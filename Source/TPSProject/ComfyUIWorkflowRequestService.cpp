// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUIWorkflowRequestService.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <initializer_list>

namespace ComfyUIWorkflowRequestService
{
	struct FLinkInfo
	{
		int32 SourceNodeId = 0;
		int32 SourceOutputIndex = 0;
	};

	// 요청 설정과 변환 결과를 비동기 HTTP 완료 시점까지 유지한다.
	class FRequestState : public TSharedFromThis<FRequestState, ESPMode::ThreadSafe>
	{
	public:
		FRequestState(const FComfyUIWorkflowRequestSettings& InSettings, FOnComfyUIWorkflowRequestComplete&& InCompletion)
			: Settings(InSettings)
			, Completion(MoveTemp(InCompletion))
		{
		}

		// workflow 파일을 읽고 /prompt 요청을 시작한다.
		void SendWorkflowPrompt()
		{
			FString WorkflowJson;
			if (!TryLoadWorkflowJson(WorkflowJson))
			{
				Complete();
				return;
			}

			FString RequestBody;
			if (!TryBuildPromptRequestBody(WorkflowJson, RequestBody))
			{
				Complete();
				return;
			}

			const FString PromptUrl = BuildComfyUrl(TEXT("/prompt"));
			if (PromptUrl.IsEmpty())
			{
				Fail(TEXT("ComfyUI 서버 주소가 비어 있습니다."));
				Complete();
				return;
			}

			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
			Request->SetURL(PromptUrl);
			Request->SetVerb(TEXT("POST"));
			Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
			Request->SetContentAsString(RequestBody);
			Request->SetTimeout(Settings.RequestTimeoutSeconds);
			Request->OnProcessRequestComplete().BindLambda(
				[Self = AsShared()](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
				{
					Self->HandleHttpResponse(TEXT("/prompt"), Response, bWasSuccessful);
				});

			UE_LOG(LogTemp, Display, TEXT("ComfyUI 최종 긍정 프롬프트: %s"), *Settings.PositivePromptText);
			UE_LOG(LogTemp, Display, TEXT("ComfyUI 요청 전송 중: %s, 본문 크기: %d chars"), *PromptUrl, RequestBody.Len());
			if (!Request->ProcessRequest())
			{
				Fail(TEXT("ComfyUI HTTP 요청 시작에 실패했습니다."));
				Complete();
			}
		}

		// /system_stats 요청을 시작해 서버 연결을 확인한다.
		void CheckServerConnection()
		{
			const FString CheckUrl = BuildComfyUrl(TEXT("/system_stats"));
			if (CheckUrl.IsEmpty())
			{
				Fail(TEXT("ComfyUI 서버 주소가 비어 있습니다."));
				Complete();
				return;
			}

			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
			Request->SetURL(CheckUrl);
			Request->SetVerb(TEXT("GET"));
			Request->SetTimeout(Settings.RequestTimeoutSeconds);
			Request->OnProcessRequestComplete().BindLambda(
				[Self = AsShared()](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
				{
					Self->HandleHttpResponse(TEXT("/system_stats"), Response, bWasSuccessful);
				});

			UE_LOG(LogTemp, Display, TEXT("ComfyUI 서버 연결 확인 중: %s"), *CheckUrl);
			if (!Request->ProcessRequest())
			{
				Fail(TEXT("ComfyUI 서버 연결 확인 요청 시작에 실패했습니다."));
				Complete();
			}
		}

	private:
		// 서버 기본 주소와 엔드포인트를 결합한다.
		FString BuildComfyUrl(const FString& EndpointPath) const
		{
			FString BaseUrl = Settings.ServerBaseUrl;
			BaseUrl.TrimStartAndEndInline();
			BaseUrl.RemoveFromEnd(TEXT("/"));
			return BaseUrl.IsEmpty() ? FString() : BaseUrl + EndpointPath;
		}

		// 설정된 workflow JSON 파일을 문자열로 읽고 기본 형식을 검증한다.
		bool TryLoadWorkflowJson(FString& OutWorkflowJson)
		{
			const FString RawPath = Settings.WorkflowJsonFile.FilePath;
			if (RawPath.IsEmpty())
			{
				Fail(TEXT("workflow JSON 파일 경로가 비어 있습니다."));
				return false;
			}

			const FString FullPath = FPaths::ConvertRelativePathToFull(RawPath);
			if (!FPaths::FileExists(FullPath))
			{
				Fail(FString::Printf(TEXT("workflow JSON 파일을 찾을 수 없습니다: %s"), *FullPath));
				return false;
			}

			if (!FFileHelper::LoadFileToString(OutWorkflowJson, *FullPath))
			{
				Fail(FString::Printf(TEXT("workflow JSON 파일을 읽지 못했습니다: %s"), *FullPath));
				return false;
			}

			TSharedPtr<FJsonObject> ParsedJson;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(OutWorkflowJson);
			if (!FJsonSerializer::Deserialize(Reader, ParsedJson) || !ParsedJson.IsValid())
			{
				Fail(FString::Printf(TEXT("workflow JSON 파일이 올바른 JSON 객체가 아닙니다: %s"), *FullPath));
				return false;
			}

			return true;
		}

		// UI workflow 또는 API prompt JSON을 /prompt 요청 본문으로 변환한다.
		bool TryBuildPromptRequestBody(const FString& WorkflowJson, FString& OutRequestBody)
		{
			TSharedPtr<FJsonObject> WorkflowObject;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(WorkflowJson);
			if (!FJsonSerializer::Deserialize(Reader, WorkflowObject) || !WorkflowObject.IsValid())
			{
				Fail(TEXT("workflow JSON을 ComfyUI 요청 본문으로 변환하지 못했습니다."));
				return false;
			}

			TSharedPtr<FJsonObject> PromptObject;
			if (WorkflowObject->HasTypedField<EJson::Object>(TEXT("prompt")))
			{
				PromptObject = WorkflowObject->GetObjectField(TEXT("prompt"));
				if (!ApplyApiPromptOverrides(PromptObject))
				{
					return false;
				}
			}
			else if (WorkflowObject->HasField(TEXT("nodes")) && WorkflowObject->HasField(TEXT("links")))
			{
				if (!TryConvertUiWorkflowToApiPrompt(WorkflowObject, PromptObject))
				{
					return false;
				}
			}
			else
			{
				PromptObject = WorkflowObject;
				if (!ApplyApiPromptOverrides(PromptObject))
				{
					return false;
				}
			}

			TSharedRef<FJsonObject> RequestObject = MakeShared<FJsonObject>();
			RequestObject->SetObjectField(TEXT("prompt"), PromptObject);
			RequestObject->SetStringField(TEXT("client_id"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));

			const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutRequestBody);
			if (!FJsonSerializer::Serialize(RequestObject, Writer))
			{
				Fail(TEXT("ComfyUI /prompt 요청 본문 생성에 실패했습니다."));
				return false;
			}

			return true;
		}

		// API prompt에서 override 대상 노드를 판별해 입력값을 변경한다.
		bool ApplyApiPromptOverrides(const TSharedPtr<FJsonObject>& PromptObject)
		{
			if (!PromptObject.IsValid())
			{
				Fail(TEXT("ComfyUI API prompt 객체가 비어 있습니다."));
				return false;
			}

			int32 PositiveNodeId = Settings.PositivePromptNodeId;
			int32 ImageNodeId = Settings.ImageSizeNodeId;
			TArray<int32> ClipNodeIds;
			TArray<int32> ImageNodeIds;

			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : PromptObject->Values)
			{
				const TSharedPtr<FJsonObject> Node = Pair.Value.IsValid() ? Pair.Value->AsObject() : nullptr;
				FString ClassType;
				if (!Node.IsValid() || !Node->TryGetStringField(TEXT("class_type"), ClassType))
				{
					continue;
				}

				const int32 NodeId = FCString::Atoi(*Pair.Key);
				if (ClassType == TEXT("CLIPTextEncode"))
				{
					ClipNodeIds.Add(NodeId);
				}
				else if (ClassType == TEXT("EmptyLatentImage"))
				{
					ImageNodeIds.Add(NodeId);
				}
				else if (ClassType == TEXT("KSampler") && PositiveNodeId <= 0 && Node->HasTypedField<EJson::Object>(TEXT("inputs")))
				{
					const TSharedPtr<FJsonObject> Inputs = Node->GetObjectField(TEXT("inputs"));
					const TArray<TSharedPtr<FJsonValue>>* PositiveLink = nullptr;
					if (Inputs->TryGetArrayField(TEXT("positive"), PositiveLink) && PositiveLink->Num() > 0)
					{
						PositiveNodeId = FCString::Atoi(*(*PositiveLink)[0]->AsString());
					}
				}
			}

			if (PositiveNodeId <= 0 && !TrySelectSingleNode(ClipNodeIds, TEXT("긍정 프롬프트"), PositiveNodeId))
			{
				return false;
			}
			if (ImageNodeId <= 0 && !TrySelectSingleNode(ImageNodeIds, TEXT("이미지 크기"), ImageNodeId))
			{
				return false;
			}

			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : PromptObject->Values)
			{
				const int32 NodeId = FCString::Atoi(*Pair.Key);
				const TSharedPtr<FJsonObject> Node = Pair.Value.IsValid() ? Pair.Value->AsObject() : nullptr;
				if (!Node.IsValid() || !Node->HasTypedField<EJson::Object>(TEXT("inputs")))
				{
					continue;
				}

				const TSharedPtr<FJsonObject> Inputs = Node->GetObjectField(TEXT("inputs"));
				if (NodeId == PositiveNodeId && !Settings.PositivePromptText.IsEmpty())
				{
					Inputs->SetStringField(TEXT("text"), Settings.PositivePromptText);
					Result.AppliedPositivePromptNodeId = NodeId;
				}
				if (NodeId == Settings.NegativePromptNodeId && !Settings.NegativePromptText.IsEmpty())
				{
					Inputs->SetStringField(TEXT("text"), Settings.NegativePromptText);
					Result.AppliedNegativePromptNodeId = NodeId;
				}
				if (NodeId == ImageNodeId)
				{
					if (Settings.ImageWidth > 0)
					{
						Inputs->SetNumberField(TEXT("width"), Settings.ImageWidth);
						Result.AppliedImageSizeNodeId = NodeId;
					}
					if (Settings.ImageHeight > 0)
					{
						Inputs->SetNumberField(TEXT("height"), Settings.ImageHeight);
						Result.AppliedImageSizeNodeId = NodeId;
					}
				}
			}

			SetOverrideMessage();
			return true;
		}

		// UI workflow 노드와 링크를 API prompt 객체로 변환한다.
		bool TryConvertUiWorkflowToApiPrompt(const TSharedPtr<FJsonObject>& WorkflowObject, TSharedPtr<FJsonObject>& OutPromptObject)
		{
			const TArray<TSharedPtr<FJsonValue>>* UiNodes = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* UiLinks = nullptr;
			if (!WorkflowObject->TryGetArrayField(TEXT("nodes"), UiNodes) || !WorkflowObject->TryGetArrayField(TEXT("links"), UiLinks))
			{
				Fail(TEXT("ComfyUI UI workflow의 nodes 또는 links 배열을 읽지 못했습니다."));
				return false;
			}

			TMap<int32, FString> NodeTypeById;
			for (const TSharedPtr<FJsonValue>& NodeValue : *UiNodes)
			{
				const TSharedPtr<FJsonObject> Node = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
				double NodeIdNumber = 0.0;
				FString ClassType;
				if (Node.IsValid() && Node->TryGetNumberField(TEXT("id"), NodeIdNumber) && Node->TryGetStringField(TEXT("type"), ClassType))
				{
					NodeTypeById.Add(static_cast<int32>(NodeIdNumber), ClassType);
				}
			}

			TMap<int32, FLinkInfo> LinkById;
			for (const TSharedPtr<FJsonValue>& LinkValue : *UiLinks)
			{
				const TArray<TSharedPtr<FJsonValue>>* LinkArray = nullptr;
				if (LinkValue.IsValid() && LinkValue->TryGetArray(LinkArray) && LinkArray->Num() >= 4)
				{
					LinkById.Add(static_cast<int32>((*LinkArray)[0]->AsNumber()), { static_cast<int32>((*LinkArray)[1]->AsNumber()), static_cast<int32>((*LinkArray)[2]->AsNumber()) });
				}
			}

			int32 PositiveNodeId = Settings.PositivePromptNodeId;
			if (PositiveNodeId <= 0 && !TryFindPositiveUiNode(*UiNodes, LinkById, NodeTypeById, PositiveNodeId))
			{
				return false;
			}

			int32 ImageNodeId = Settings.ImageSizeNodeId;
			if (ImageNodeId <= 0 && !TryFindSingleUiNodeIdByType(*UiNodes, TEXT("EmptyLatentImage"), TEXT("이미지 크기"), ImageNodeId))
			{
				return false;
			}

			TSharedRef<FJsonObject> PromptObject = MakeShared<FJsonObject>();
			for (const TSharedPtr<FJsonValue>& NodeValue : *UiNodes)
			{
				const TSharedPtr<FJsonObject> UiNode = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
				double NodeIdNumber = 0.0;
				FString ClassType;
				if (!UiNode.IsValid() || !UiNode->TryGetNumberField(TEXT("id"), NodeIdNumber) || !UiNode->TryGetStringField(TEXT("type"), ClassType))
				{
					continue;
				}

				const int32 NodeId = static_cast<int32>(NodeIdNumber);
				TSharedRef<FJsonObject> ApiNode = MakeShared<FJsonObject>();
				TSharedRef<FJsonObject> ApiInputs = MakeShared<FJsonObject>();
				ApiNode->SetStringField(TEXT("class_type"), ClassType);
				ApiNode->SetObjectField(TEXT("inputs"), ApiInputs);

				const TArray<TSharedPtr<FJsonValue>>* UiInputs = nullptr;
				if (UiNode->TryGetArrayField(TEXT("inputs"), UiInputs))
				{
					for (const TSharedPtr<FJsonValue>& InputValue : *UiInputs)
					{
						const TSharedPtr<FJsonObject> Input = InputValue.IsValid() ? InputValue->AsObject() : nullptr;
						FString InputName;
						double LinkIdNumber = 0.0;
						if (!Input.IsValid() || !Input->TryGetStringField(TEXT("name"), InputName) || !Input->TryGetNumberField(TEXT("link"), LinkIdNumber))
						{
							continue;
						}

						const FLinkInfo* LinkInfo = LinkById.Find(static_cast<int32>(LinkIdNumber));
						if (LinkInfo)
						{
							TArray<TSharedPtr<FJsonValue>> LinkInput;
							LinkInput.Add(MakeShared<FJsonValueString>(FString::FromInt(LinkInfo->SourceNodeId)));
							LinkInput.Add(MakeShared<FJsonValueNumber>(LinkInfo->SourceOutputIndex));
							ApiInputs->SetArrayField(InputName, LinkInput);
						}
					}
				}

				ApplyKnownWidgetInputs(UiNode, ApiInputs, NodeId, ClassType, PositiveNodeId, ImageNodeId);
				PromptObject->SetObjectField(FString::FromInt(NodeId), ApiNode);
			}

			if (PromptObject->Values.IsEmpty())
			{
				Fail(TEXT("ComfyUI UI workflow에서 변환 가능한 노드를 찾지 못했습니다."));
				return false;
			}

			OutPromptObject = PromptObject;
			SetOverrideMessage();
			return true;
		}

		// KSampler positive 연결을 우선 사용하고 없으면 단일 CLIPTextEncode를 선택한다.
		bool TryFindPositiveUiNode(const TArray<TSharedPtr<FJsonValue>>& UiNodes, const TMap<int32, FLinkInfo>& LinkById, const TMap<int32, FString>& NodeTypeById, int32& OutNodeId)
		{
			TArray<int32> Candidates;
			for (const TSharedPtr<FJsonValue>& NodeValue : UiNodes)
			{
				const TSharedPtr<FJsonObject> Node = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
				FString ClassType;
				const TArray<TSharedPtr<FJsonValue>>* Inputs = nullptr;
				if (!Node.IsValid() || !Node->TryGetStringField(TEXT("type"), ClassType) || ClassType != TEXT("KSampler") || !Node->TryGetArrayField(TEXT("inputs"), Inputs))
				{
					continue;
				}

				for (const TSharedPtr<FJsonValue>& InputValue : *Inputs)
				{
					const TSharedPtr<FJsonObject> Input = InputValue.IsValid() ? InputValue->AsObject() : nullptr;
					FString InputName;
					double LinkIdNumber = 0.0;
					if (!Input.IsValid() || !Input->TryGetStringField(TEXT("name"), InputName) || InputName != TEXT("positive") || !Input->TryGetNumberField(TEXT("link"), LinkIdNumber))
					{
						continue;
					}

					const FLinkInfo* LinkInfo = LinkById.Find(static_cast<int32>(LinkIdNumber));
					const FString* SourceType = LinkInfo ? NodeTypeById.Find(LinkInfo->SourceNodeId) : nullptr;
					if (SourceType && *SourceType == TEXT("CLIPTextEncode"))
					{
						Candidates.AddUnique(LinkInfo->SourceNodeId);
					}
				}
			}

			if (Candidates.Num() == 1)
			{
				OutNodeId = Candidates[0];
				return true;
			}
			if (Candidates.Num() > 1)
			{
				Fail(FString::Printf(TEXT("workflow에서 KSampler positive에 연결된 긍정 프롬프트 후보가 %d개 발견되어 자동 판단할 수 없습니다."), Candidates.Num()));
				return false;
			}

			return TryFindSingleUiNodeIdByType(UiNodes, TEXT("CLIPTextEncode"), TEXT("긍정 프롬프트"), OutNodeId);
		}

		// UI workflow에서 지정 타입의 노드가 정확히 하나인지 확인한다.
		bool TryFindSingleUiNodeIdByType(const TArray<TSharedPtr<FJsonValue>>& UiNodes, const FString& NodeType, const FString& RoleName, int32& OutNodeId)
		{
			TArray<int32> NodeIds;
			for (const TSharedPtr<FJsonValue>& NodeValue : UiNodes)
			{
				const TSharedPtr<FJsonObject> Node = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
				FString ClassType;
				double NodeIdNumber = 0.0;
				if (Node.IsValid() && Node->TryGetStringField(TEXT("type"), ClassType) && ClassType == NodeType && Node->TryGetNumberField(TEXT("id"), NodeIdNumber))
				{
					NodeIds.Add(static_cast<int32>(NodeIdNumber));
				}
			}

			return TrySelectSingleNode(NodeIds, RoleName, OutNodeId);
		}

		// 후보가 하나일 때만 자동 선택하고 아니면 실패 사유를 기록한다.
		bool TrySelectSingleNode(const TArray<int32>& NodeIds, const FString& RoleName, int32& OutNodeId)
		{
			if (NodeIds.Num() == 1)
			{
				OutNodeId = NodeIds[0];
				return true;
			}

			Fail(NodeIds.IsEmpty()
				? FString::Printf(TEXT("workflow에서 %s 노드를 찾지 못했습니다."), *RoleName)
				: FString::Printf(TEXT("workflow에서 %s 후보 노드가 %d개 발견되어 자동 판단할 수 없습니다."), *RoleName, NodeIds.Num()));
			return false;
		}

		// UI 노드 widget 값을 API 입력으로 복사하고 지정된 override를 적용한다.
		void ApplyKnownWidgetInputs(const TSharedPtr<FJsonObject>& UiNode, const TSharedRef<FJsonObject>& ApiInputs, int32 NodeId, const FString& ClassType, int32 PositiveNodeId, int32 ImageNodeId)
		{
			const TArray<TSharedPtr<FJsonValue>>* WidgetValues = nullptr;
			if (!UiNode->TryGetArrayField(TEXT("widgets_values"), WidgetValues))
			{
				return;
			}

			auto CopyWidget = [WidgetValues, ApiInputs](int32 Index, const TCHAR* InputName)
			{
				if (WidgetValues->IsValidIndex(Index) && (*WidgetValues)[Index].IsValid())
				{
					ApiInputs->SetField(InputName, (*WidgetValues)[Index]);
				}
			};
			auto CopyNamedWidgets = [&CopyWidget](std::initializer_list<const TCHAR*> Names)
			{
				int32 Index = 0;
				for (const TCHAR* Name : Names)
				{
					CopyWidget(Index++, Name);
				}
			};

			if (ClassType == TEXT("CLIPTextEncode"))
			{
				FString TextValue = WidgetValues->IsValidIndex(0) && (*WidgetValues)[0].IsValid() ? (*WidgetValues)[0]->AsString() : FString();
				if (NodeId == PositiveNodeId && !Settings.PositivePromptText.IsEmpty())
				{
					TextValue = Settings.PositivePromptText;
					Result.AppliedPositivePromptNodeId = NodeId;
				}
				else if (NodeId == Settings.NegativePromptNodeId && !Settings.NegativePromptText.IsEmpty())
				{
					TextValue = Settings.NegativePromptText;
					Result.AppliedNegativePromptNodeId = NodeId;
				}
				ApiInputs->SetStringField(TEXT("text"), TextValue);
				return;
			}

			if (ClassType == TEXT("EmptyLatentImage"))
			{
				double Width = WidgetValues->IsValidIndex(0) && (*WidgetValues)[0].IsValid() ? (*WidgetValues)[0]->AsNumber() : 512.0;
				double Height = WidgetValues->IsValidIndex(1) && (*WidgetValues)[1].IsValid() ? (*WidgetValues)[1]->AsNumber() : 512.0;
				const double BatchSize = WidgetValues->IsValidIndex(2) && (*WidgetValues)[2].IsValid() ? (*WidgetValues)[2]->AsNumber() : 1.0;
				if (NodeId == ImageNodeId && Settings.ImageWidth > 0)
				{
					Width = Settings.ImageWidth;
					Result.AppliedImageSizeNodeId = NodeId;
				}
				if (NodeId == ImageNodeId && Settings.ImageHeight > 0)
				{
					Height = Settings.ImageHeight;
					Result.AppliedImageSizeNodeId = NodeId;
				}
				ApiInputs->SetNumberField(TEXT("width"), Width);
				ApiInputs->SetNumberField(TEXT("height"), Height);
				ApiInputs->SetNumberField(TEXT("batch_size"), BatchSize);
				return;
			}

			if (ClassType == TEXT("CheckpointLoaderSimple")) CopyNamedWidgets({ TEXT("ckpt_name") });
			else if (ClassType == TEXT("KSampler")) CopyNamedWidgets({ TEXT("seed"), TEXT("control_after_generate"), TEXT("steps"), TEXT("cfg"), TEXT("sampler_name"), TEXT("scheduler"), TEXT("denoise") });
			else if (ClassType == TEXT("SaveImage")) CopyNamedWidgets({ TEXT("filename_prefix") });
			else if (ClassType == TEXT("LoadImage")) CopyNamedWidgets({ TEXT("image"), TEXT("upload") });
			else if (ClassType == TEXT("LoraLoader")) CopyNamedWidgets({ TEXT("lora_name"), TEXT("strength_model"), TEXT("strength_clip") });
			else if (ClassType == TEXT("VAELoader")) CopyNamedWidgets({ TEXT("vae_name") });
			else if (ClassType == TEXT("UNETLoader")) CopyNamedWidgets({ TEXT("unet_name"), TEXT("weight_dtype") });
			else if (ClassType == TEXT("CLIPLoader")) CopyNamedWidgets({ TEXT("clip_name"), TEXT("type") });
			else if (ClassType == TEXT("InspyrenetRembg")) CopyNamedWidgets({ TEXT("torchscript_jit") });
		}

		// 적용된 노드 ID를 호출자와 Output Log에서 확인할 수 있게 기록한다.
		void SetOverrideMessage()
		{
			Result.OverrideMessage = FString::Printf(
				TEXT("Override 적용 결과 - Positive Node: %d, Negative Node: %d, Image Size Node: %d"),
				Result.AppliedPositivePromptNodeId,
				Result.AppliedNegativePromptNodeId,
				Result.AppliedImageSizeNodeId);
			UE_LOG(LogTemp, Display, TEXT("%s"), *Result.OverrideMessage);
		}

		// HTTP 완료 결과를 공통 결과 구조에 기록하고 호출자에게 전달한다.
		void HandleHttpResponse(const TCHAR* Endpoint, const FHttpResponsePtr& Response, bool bWasSuccessful)
		{
			if (!bWasSuccessful || !Response.IsValid())
			{
				Fail(FString::Printf(TEXT("ComfyUI %s 응답을 받지 못했습니다. 서버 연결과 주소를 확인하세요."), Endpoint));
				Complete();
				return;
			}

			Result.HttpStatusCode = Response->GetResponseCode();
			Result.ResponseMessage = Response->GetContentAsString();
			Result.bSucceeded = Result.HttpStatusCode >= 200 && Result.HttpStatusCode < 300;
			if (Result.bSucceeded && FCString::Strcmp(Endpoint, TEXT("/prompt")) == 0)
			{
				TSharedPtr<FJsonObject> ResponseObject;
				const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result.ResponseMessage);
				if (!FJsonSerializer::Deserialize(Reader, ResponseObject)
					|| !ResponseObject.IsValid()
					|| !ResponseObject->TryGetStringField(TEXT("prompt_id"), Result.PromptId)
					|| Result.PromptId.IsEmpty())
				{
					Result.bSucceeded = false;
					Result.ResponseMessage = TEXT("ComfyUI /prompt 응답에서 prompt_id를 찾지 못했습니다.");
				}
			}
			UE_LOG(LogTemp, Display, TEXT("ComfyUI %s 응답 %s - HTTP %d: %s"), Endpoint, Result.bSucceeded ? TEXT("성공") : TEXT("실패"), Result.HttpStatusCode, *Result.ResponseMessage);
			Complete();
		}

		// 실패 상태와 사유를 공통 결과에 기록한다.
		void Fail(const FString& Message)
		{
			Result.bSucceeded = false;
			Result.HttpStatusCode = 0;
			Result.ResponseMessage = Message;
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		}

		// 완료 델리게이트를 한 번만 실행한다.
		void Complete()
		{
			if (Completion.IsBound())
			{
				FOnComfyUIWorkflowRequestComplete LocalCompletion = MoveTemp(Completion);
				LocalCompletion.Execute(Result);
			}
		}

		FComfyUIWorkflowRequestSettings Settings;
		FComfyUIWorkflowRequestResult Result;
		FOnComfyUIWorkflowRequestComplete Completion;
	};
}

void FComfyUIWorkflowRequestService::SendWorkflowPrompt(const FComfyUIWorkflowRequestSettings& Settings, FOnComfyUIWorkflowRequestComplete Completion)
{
	MakeShared<ComfyUIWorkflowRequestService::FRequestState, ESPMode::ThreadSafe>(Settings, MoveTemp(Completion))->SendWorkflowPrompt();
}

void FComfyUIWorkflowRequestService::CheckServerConnection(const FComfyUIWorkflowRequestSettings& Settings, FOnComfyUIWorkflowRequestComplete Completion)
{
	MakeShared<ComfyUIWorkflowRequestService::FRequestState, ESPMode::ThreadSafe>(Settings, MoveTemp(Completion))->CheckServerConnection();
}
