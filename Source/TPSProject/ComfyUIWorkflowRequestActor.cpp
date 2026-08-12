// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUIWorkflowRequestActor.h"

#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include <initializer_list>

AComfyUIWorkflowRequestActor::AComfyUIWorkflowRequestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AComfyUIWorkflowRequestActor::SendWorkflowPrompt()
{
	FString WorkflowJson;
	if (!TryLoadWorkflowJson(WorkflowJson))
	{
		return;
	}

	FString RequestBody;
	if (!TryBuildPromptRequestBody(WorkflowJson, RequestBody))
	{
		return;
	}

	const FString PromptUrl = BuildComfyUrl(TEXT("/prompt"));
	if (PromptUrl.IsEmpty())
	{
		SetFailureMessage(TEXT("ComfyUI 서버 주소가 비어 있습니다."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> PromptRequest = FHttpModule::Get().CreateRequest();
	PromptRequest->SetURL(PromptUrl);
	PromptRequest->SetVerb(TEXT("POST"));
	PromptRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	PromptRequest->SetContentAsString(RequestBody);
	PromptRequest->SetTimeout(RequestTimeoutSeconds);
	PromptRequest->OnProcessRequestComplete().BindUObject(this, &AComfyUIWorkflowRequestActor::HandlePromptResponse);

	bLastRequestSucceeded = false;
	LastHttpStatusCode = 0;
	LastResponseMessage = FString::Printf(TEXT("ComfyUI 요청 전송 중: %s, 본문 크기: %d chars"), *PromptUrl, RequestBody.Len());
	UE_LOG(LogTemp, Display, TEXT("%s"), *LastResponseMessage);

	if (!PromptRequest->ProcessRequest())
	{
		SetFailureMessage(TEXT("ComfyUI HTTP 요청 시작에 실패했습니다."));
	}
}

void AComfyUIWorkflowRequestActor::CheckServerConnection()
{
	const FString CheckUrl = BuildComfyUrl(TEXT("/system_stats"));
	if (CheckUrl.IsEmpty())
	{
		SetFailureMessage(TEXT("ComfyUI 서버 주소가 비어 있습니다."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CheckRequest = FHttpModule::Get().CreateRequest();
	CheckRequest->SetURL(CheckUrl);
	CheckRequest->SetVerb(TEXT("GET"));
	CheckRequest->SetTimeout(RequestTimeoutSeconds);
	CheckRequest->OnProcessRequestComplete().BindUObject(this, &AComfyUIWorkflowRequestActor::HandleServerCheckResponse);

	bLastRequestSucceeded = false;
	LastHttpStatusCode = 0;
	LastResponseMessage = FString::Printf(TEXT("ComfyUI 서버 연결 확인 중: %s"), *CheckUrl);
	UE_LOG(LogTemp, Display, TEXT("%s"), *LastResponseMessage);

	if (!CheckRequest->ProcessRequest())
	{
		SetFailureMessage(TEXT("ComfyUI 서버 연결 확인 요청 시작에 실패했습니다."));
	}
}

FString AComfyUIWorkflowRequestActor::BuildComfyUrl(const FString& EndpointPath) const
{
	FString TrimmedBaseUrl = ServerBaseUrl;
	TrimmedBaseUrl.TrimStartAndEndInline();
	TrimmedBaseUrl.RemoveFromEnd(TEXT("/"));

	if (TrimmedBaseUrl.IsEmpty())
	{
		return FString();
	}

	return FString::Printf(TEXT("%s%s"), *TrimmedBaseUrl, *EndpointPath);
}

bool AComfyUIWorkflowRequestActor::TryLoadWorkflowJson(FString& OutRequestBody)
{
	const FString RawPath = WorkflowJsonFile.FilePath;
	if (RawPath.IsEmpty())
	{
		SetFailureMessage(TEXT("workflow JSON 파일 경로가 비어 있습니다."));
		return false;
	}

	const FString FullPath = FPaths::ConvertRelativePathToFull(RawPath);
	if (!FPaths::FileExists(FullPath))
	{
		SetFailureMessage(FString::Printf(TEXT("workflow JSON 파일을 찾을 수 없습니다: %s"), *FullPath));
		return false;
	}

	if (!FFileHelper::LoadFileToString(OutRequestBody, *FullPath))
	{
		SetFailureMessage(FString::Printf(TEXT("workflow JSON 파일을 읽지 못했습니다: %s"), *FullPath));
		return false;
	}

	TSharedPtr<FJsonObject> ParsedJson;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(OutRequestBody);
	if (!FJsonSerializer::Deserialize(JsonReader, ParsedJson) || !ParsedJson.IsValid())
	{
		SetFailureMessage(FString::Printf(TEXT("workflow JSON 파일이 올바른 JSON 객체가 아닙니다: %s"), *FullPath));
		return false;
	}

	return true;
}

bool AComfyUIWorkflowRequestActor::TryBuildPromptRequestBody(const FString& WorkflowJson, FString& OutRequestBody)
{
	TSharedPtr<FJsonObject> WorkflowObject;
	TSharedRef<TJsonReader<>> WorkflowReader = TJsonReaderFactory<>::Create(WorkflowJson);
	if (!FJsonSerializer::Deserialize(WorkflowReader, WorkflowObject) || !WorkflowObject.IsValid())
	{
		SetFailureMessage(TEXT("workflow JSON을 ComfyUI 요청 본문으로 변환하지 못했습니다."));
		return false;
	}

	if (WorkflowObject->HasField(TEXT("prompt")))
	{
		OutRequestBody = WorkflowJson;
		return true;
	}

	if (WorkflowObject->HasField(TEXT("nodes")) && WorkflowObject->HasField(TEXT("links")))
	{
		TSharedPtr<FJsonObject> PromptObject;
		if (!TryConvertUiWorkflowToApiPrompt(WorkflowObject, PromptObject))
		{
			return false;
		}

		TSharedRef<FJsonObject> RequestObject = MakeShared<FJsonObject>();
		RequestObject->SetObjectField(TEXT("prompt"), PromptObject);
		RequestObject->SetStringField(TEXT("client_id"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));

		TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutRequestBody);
		if (!FJsonSerializer::Serialize(RequestObject, JsonWriter))
		{
			SetFailureMessage(TEXT("ComfyUI UI workflow를 /prompt 요청 본문으로 변환하지 못했습니다."));
			return false;
		}

		return true;
	}

	TSharedRef<FJsonObject> RequestObject = MakeShared<FJsonObject>();
	RequestObject->SetObjectField(TEXT("prompt"), WorkflowObject);
	RequestObject->SetStringField(TEXT("client_id"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));

	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutRequestBody);
	if (!FJsonSerializer::Serialize(RequestObject, JsonWriter))
	{
		SetFailureMessage(TEXT("ComfyUI /prompt 요청 본문 생성에 실패했습니다."));
		return false;
	}

	return true;
}

bool AComfyUIWorkflowRequestActor::TryConvertUiWorkflowToApiPrompt(const TSharedPtr<FJsonObject>& WorkflowObject, TSharedPtr<FJsonObject>& OutPromptObject)
{
	LastAppliedPositivePromptNodeId = 0;
	LastAppliedNegativePromptNodeId = 0;
	LastAppliedImageSizeNodeId = 0;
	LastOverrideMessage.Empty();
	ActivePositivePromptNodeId = 0;
	ActiveImageSizeNodeId = 0;

	const TArray<TSharedPtr<FJsonValue>>* UiNodes = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* UiLinks = nullptr;
	if (!WorkflowObject->TryGetArrayField(TEXT("nodes"), UiNodes) || !WorkflowObject->TryGetArrayField(TEXT("links"), UiLinks))
	{
		SetFailureMessage(TEXT("ComfyUI UI workflow의 nodes 또는 links 배열을 읽지 못했습니다."));
		return false;
	}

	struct FComfyLinkInfo
	{
		int32 SourceNodeId = 0;
		int32 SourceOutputIndex = 0;
	};

	TMap<int32, FString> NodeTypeById;
	for (const TSharedPtr<FJsonValue>& NodeValue : *UiNodes)
	{
		const TSharedPtr<FJsonObject> UiNode = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
		if (!UiNode.IsValid())
		{
			continue;
		}

		double NodeIdNumber = 0.0;
		FString ClassType;
		if (UiNode->TryGetNumberField(TEXT("id"), NodeIdNumber) && UiNode->TryGetStringField(TEXT("type"), ClassType))
		{
			NodeTypeById.Add(static_cast<int32>(NodeIdNumber), ClassType);
		}
	}

	TMap<int32, FComfyLinkInfo> LinkById;
	for (const TSharedPtr<FJsonValue>& LinkValue : *UiLinks)
	{
		const TArray<TSharedPtr<FJsonValue>>* LinkArray = nullptr;
		if (!LinkValue.IsValid() || !LinkValue->TryGetArray(LinkArray) || LinkArray->Num() < 4)
		{
			continue;
		}

		const int32 LinkId = static_cast<int32>((*LinkArray)[0]->AsNumber());
		FComfyLinkInfo LinkInfo;
		LinkInfo.SourceNodeId = static_cast<int32>((*LinkArray)[1]->AsNumber());
		LinkInfo.SourceOutputIndex = static_cast<int32>((*LinkArray)[2]->AsNumber());
		LinkById.Add(LinkId, LinkInfo);
	}

	if (PositivePromptNodeId > 0)
	{
		ActivePositivePromptNodeId = PositivePromptNodeId;
	}
	else
	{
		int32 PositiveCandidateCount = 0;
		int32 PositiveCandidateNodeId = 0;

		for (const TSharedPtr<FJsonValue>& NodeValue : *UiNodes)
		{
			const TSharedPtr<FJsonObject> UiNode = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
			if (!UiNode.IsValid())
			{
				continue;
			}

			FString ClassType;
			if (!UiNode->TryGetStringField(TEXT("type"), ClassType) || ClassType != TEXT("KSampler"))
			{
				continue;
			}

			const TArray<TSharedPtr<FJsonValue>>* UiInputs = nullptr;
			if (!UiNode->TryGetArrayField(TEXT("inputs"), UiInputs))
			{
				continue;
			}

			for (const TSharedPtr<FJsonValue>& InputValue : *UiInputs)
			{
				const TSharedPtr<FJsonObject> UiInput = InputValue.IsValid() ? InputValue->AsObject() : nullptr;
				if (!UiInput.IsValid())
				{
					continue;
				}

				FString InputName;
				double LinkIdNumber = 0.0;
				if (!UiInput->TryGetStringField(TEXT("name"), InputName) || InputName != TEXT("positive") || !UiInput->TryGetNumberField(TEXT("link"), LinkIdNumber))
				{
					continue;
				}

				const FComfyLinkInfo* LinkInfo = LinkById.Find(static_cast<int32>(LinkIdNumber));
				const FString* SourceNodeType = LinkInfo ? NodeTypeById.Find(LinkInfo->SourceNodeId) : nullptr;
				if (SourceNodeType && *SourceNodeType == TEXT("CLIPTextEncode"))
				{
					++PositiveCandidateCount;
					PositiveCandidateNodeId = LinkInfo->SourceNodeId;
				}
			}
		}

		if (PositiveCandidateCount == 1)
		{
			ActivePositivePromptNodeId = PositiveCandidateNodeId;
		}
		else if (PositiveCandidateCount == 0 && !TryFindSingleUiNodeIdByType(*UiNodes, TEXT("CLIPTextEncode"), TEXT("긍정 프롬프트"), ActivePositivePromptNodeId))
		{
			return false;
		}
		else if (PositiveCandidateCount > 1)
		{
			SetFailureMessage(FString::Printf(TEXT("workflow에서 KSampler positive에 연결된 긍정 프롬프트 후보가 %d개 발견되어 자동 판단할 수 없습니다."), PositiveCandidateCount));
			return false;
		}
	}

	if (ImageSizeNodeId > 0)
	{
		ActiveImageSizeNodeId = ImageSizeNodeId;
	}
	else if (!TryFindSingleUiNodeIdByType(*UiNodes, TEXT("EmptyLatentImage"), TEXT("이미지 크기"), ActiveImageSizeNodeId))
	{
		return false;
	}

	TSharedRef<FJsonObject> PromptObject = MakeShared<FJsonObject>();
	int32 ClipTextEncodeIndex = 0;
	int32 EmptyLatentImageIndex = 0;

	for (const TSharedPtr<FJsonValue>& NodeValue : *UiNodes)
	{
		const TSharedPtr<FJsonObject> UiNode = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
		if (!UiNode.IsValid())
		{
			continue;
		}

		int32 NodeId = 0;
		double NodeIdNumber = 0.0;
		if (!UiNode->TryGetNumberField(TEXT("id"), NodeIdNumber))
		{
			continue;
		}
		NodeId = static_cast<int32>(NodeIdNumber);

		FString ClassType;
		if (!UiNode->TryGetStringField(TEXT("type"), ClassType) || ClassType.IsEmpty())
		{
			continue;
		}

		const int32 CurrentClipTextEncodeIndex = ClassType == TEXT("CLIPTextEncode") ? ClipTextEncodeIndex++ : -1;
		const int32 CurrentEmptyLatentImageIndex = ClassType == TEXT("EmptyLatentImage") ? EmptyLatentImageIndex++ : -1;

		TSharedRef<FJsonObject> ApiNode = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> ApiInputs = MakeShared<FJsonObject>();
		ApiNode->SetStringField(TEXT("class_type"), ClassType);
		ApiNode->SetObjectField(TEXT("inputs"), ApiInputs);

		const TArray<TSharedPtr<FJsonValue>>* UiInputs = nullptr;
		if (UiNode->TryGetArrayField(TEXT("inputs"), UiInputs))
		{
			for (const TSharedPtr<FJsonValue>& InputValue : *UiInputs)
			{
				const TSharedPtr<FJsonObject> UiInput = InputValue.IsValid() ? InputValue->AsObject() : nullptr;
				if (!UiInput.IsValid())
				{
					continue;
				}

				FString InputName;
				double LinkIdNumber = 0.0;
				if (!UiInput->TryGetStringField(TEXT("name"), InputName) || !UiInput->TryGetNumberField(TEXT("link"), LinkIdNumber))
				{
					continue;
				}

				const FComfyLinkInfo* LinkInfo = LinkById.Find(static_cast<int32>(LinkIdNumber));
				if (!LinkInfo)
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> LinkInput;
				LinkInput.Add(MakeShared<FJsonValueString>(FString::FromInt(LinkInfo->SourceNodeId)));
				LinkInput.Add(MakeShared<FJsonValueNumber>(LinkInfo->SourceOutputIndex));
				ApiInputs->SetArrayField(InputName, LinkInput);
			}
		}

		ApplyKnownWidgetInputs(UiNode, ApiNode, CurrentClipTextEncodeIndex, CurrentEmptyLatentImageIndex);
		PromptObject->SetObjectField(FString::FromInt(NodeId), ApiNode);
	}

	if (PromptObject->Values.IsEmpty())
	{
		SetFailureMessage(TEXT("ComfyUI UI workflow에서 변환 가능한 노드를 찾지 못했습니다."));
		return false;
	}

	OutPromptObject = PromptObject;

	LastOverrideMessage = FString::Printf(
		TEXT("Override 적용 결과 - Positive Node: %d, Negative Node: %d, Image Size Node: %d"),
		LastAppliedPositivePromptNodeId,
		LastAppliedNegativePromptNodeId,
		LastAppliedImageSizeNodeId
	);
	UE_LOG(LogTemp, Display, TEXT("%s"), *LastOverrideMessage);

	return true;
}

bool AComfyUIWorkflowRequestActor::TryFindSingleUiNodeIdByType(const TArray<TSharedPtr<FJsonValue>>& UiNodes, const FString& NodeType, const FString& RoleName, int32& OutNodeId)
{
	int32 FoundNodeCount = 0;
	int32 FoundNodeId = 0;

	for (const TSharedPtr<FJsonValue>& NodeValue : UiNodes)
	{
		const TSharedPtr<FJsonObject> UiNode = NodeValue.IsValid() ? NodeValue->AsObject() : nullptr;
		if (!UiNode.IsValid())
		{
			continue;
		}

		FString CurrentNodeType;
		if (!UiNode->TryGetStringField(TEXT("type"), CurrentNodeType) || CurrentNodeType != NodeType)
		{
			continue;
		}

		double NodeIdNumber = 0.0;
		if (!UiNode->TryGetNumberField(TEXT("id"), NodeIdNumber))
		{
			continue;
		}

		++FoundNodeCount;
		FoundNodeId = static_cast<int32>(NodeIdNumber);
	}

	if (FoundNodeCount == 1)
	{
		OutNodeId = FoundNodeId;
		return true;
	}

	if (FoundNodeCount == 0)
	{
		SetFailureMessage(FString::Printf(TEXT("workflow에서 %s 노드(%s)를 찾지 못했습니다."), *RoleName, *NodeType));
		return false;
	}

	SetFailureMessage(FString::Printf(TEXT("workflow에서 %s 후보 노드(%s)가 %d개 발견되어 자동 판단할 수 없습니다."), *RoleName, *NodeType, FoundNodeCount));
	return false;
}

void AComfyUIWorkflowRequestActor::ApplyKnownWidgetInputs(const TSharedPtr<FJsonObject>& UiNode, const TSharedRef<FJsonObject>& ApiNode, int32 ClipTextEncodeIndex, int32 EmptyLatentImageIndex)
{
	if (!UiNode.IsValid())
	{
		return;
	}

	FString ClassType;
	if (!UiNode->TryGetStringField(TEXT("type"), ClassType))
	{
		return;
	}

	double NodeIdNumber = 0.0;
	UiNode->TryGetNumberField(TEXT("id"), NodeIdNumber);
	const int32 NodeId = static_cast<int32>(NodeIdNumber);

	const TArray<TSharedPtr<FJsonValue>>* WidgetValues = nullptr;
	if (!UiNode->TryGetArrayField(TEXT("widgets_values"), WidgetValues))
	{
		return;
	}

	const TSharedPtr<FJsonObject> ApiInputs = ApiNode->GetObjectField(TEXT("inputs"));
	auto CopyWidget = [WidgetValues, ApiInputs](int32 WidgetIndex, const TCHAR* InputName)
	{
		if (WidgetValues->IsValidIndex(WidgetIndex) && (*WidgetValues)[WidgetIndex].IsValid())
		{
			ApiInputs->SetField(InputName, (*WidgetValues)[WidgetIndex]);
		}
	};

	auto CopyNamedWidgets = [&CopyWidget](std::initializer_list<const TCHAR*> InputNames)
	{
		int32 Index = 0;
		for (const TCHAR* InputName : InputNames)
		{
			CopyWidget(Index, InputName);
			++Index;
		}
	};

	if (ClassType == TEXT("CLIPTextEncode"))
	{
		FString TextValue;
		if (WidgetValues->IsValidIndex(0) && (*WidgetValues)[0].IsValid())
		{
			TextValue = (*WidgetValues)[0]->AsString();
		}

		const bool bUseAsPositive = ActivePositivePromptNodeId == NodeId;
		const bool bUseAsNegative = NegativePromptNodeId == NodeId;
		if (bUseAsPositive && !PositivePromptText.IsEmpty())
		{
			TextValue = PositivePromptText;
			LastAppliedPositivePromptNodeId = NodeId;
		}
		else if (bUseAsNegative && !NegativePromptText.IsEmpty())
		{
			TextValue = NegativePromptText;
			LastAppliedNegativePromptNodeId = NodeId;
		}

		ApiInputs->SetStringField(TEXT("text"), TextValue);
		return;
	}

	if (ClassType == TEXT("EmptyLatentImage"))
	{
		const bool bUseAsSizeNode = ActiveImageSizeNodeId == NodeId;
		double WidthValue = WidgetValues->IsValidIndex(0) && (*WidgetValues)[0].IsValid() ? (*WidgetValues)[0]->AsNumber() : 512.0;
		double HeightValue = WidgetValues->IsValidIndex(1) && (*WidgetValues)[1].IsValid() ? (*WidgetValues)[1]->AsNumber() : 512.0;
		const double BatchSizeValue = WidgetValues->IsValidIndex(2) && (*WidgetValues)[2].IsValid() ? (*WidgetValues)[2]->AsNumber() : 1.0;

		if (bUseAsSizeNode && ImageWidth > 0)
		{
			WidthValue = ImageWidth;
			LastAppliedImageSizeNodeId = NodeId;
		}
		if (bUseAsSizeNode && ImageHeight > 0)
		{
			HeightValue = ImageHeight;
			LastAppliedImageSizeNodeId = NodeId;
		}

		ApiInputs->SetNumberField(TEXT("width"), WidthValue);
		ApiInputs->SetNumberField(TEXT("height"), HeightValue);
		ApiInputs->SetNumberField(TEXT("batch_size"), BatchSizeValue);
		return;
	}

	if (ClassType == TEXT("CheckpointLoaderSimple"))
	{
		CopyNamedWidgets({ TEXT("ckpt_name") });
		return;
	}

	if (ClassType == TEXT("KSampler"))
	{
		CopyNamedWidgets({ TEXT("seed"), TEXT("control_after_generate"), TEXT("steps"), TEXT("cfg"), TEXT("sampler_name"), TEXT("scheduler"), TEXT("denoise") });
		return;
	}

	if (ClassType == TEXT("SaveImage"))
	{
		CopyNamedWidgets({ TEXT("filename_prefix") });
		return;
	}

	if (ClassType == TEXT("LoadImage"))
	{
		CopyNamedWidgets({ TEXT("image"), TEXT("upload") });
		return;
	}

	if (ClassType == TEXT("LoraLoader"))
	{
		CopyNamedWidgets({ TEXT("lora_name"), TEXT("strength_model"), TEXT("strength_clip") });
		return;
	}

	if (ClassType == TEXT("VAELoader"))
	{
		CopyNamedWidgets({ TEXT("vae_name") });
		return;
	}

	if (ClassType == TEXT("UNETLoader"))
	{
		CopyNamedWidgets({ TEXT("unet_name"), TEXT("weight_dtype") });
		return;
	}

	if (ClassType == TEXT("CLIPLoader"))
	{
		CopyNamedWidgets({ TEXT("clip_name"), TEXT("type") });
		return;
	}

	if (ClassType == TEXT("InspyrenetRembg"))
	{
		CopyNamedWidgets({ TEXT("torchscript_jit") });
	}
}

void AComfyUIWorkflowRequestActor::HandlePromptResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetFailureMessage(TEXT("ComfyUI /prompt 응답을 받지 못했습니다. 먼저 Check Server Connection으로 서버 연결을 확인하세요."));
		return;
	}

	LastHttpStatusCode = Response->GetResponseCode();
	LastResponseMessage = Response->GetContentAsString();
	bLastRequestSucceeded = LastHttpStatusCode >= 200 && LastHttpStatusCode < 300;

	const TCHAR* ResultText = bLastRequestSucceeded ? TEXT("성공") : TEXT("실패");
	UE_LOG(LogTemp, Display, TEXT("ComfyUI /prompt 응답 %s - HTTP %d: %s"), ResultText, LastHttpStatusCode, *LastResponseMessage);
}

void AComfyUIWorkflowRequestActor::HandleServerCheckResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetFailureMessage(TEXT("ComfyUI 서버 연결 확인 응답을 받지 못했습니다. 브라우저에서 http://127.0.0.1:8188/system_stats 접속 여부를 먼저 확인하세요."));
		return;
	}

	LastHttpStatusCode = Response->GetResponseCode();
	LastResponseMessage = Response->GetContentAsString();
	bLastRequestSucceeded = LastHttpStatusCode >= 200 && LastHttpStatusCode < 300;

	const TCHAR* ResultText = bLastRequestSucceeded ? TEXT("성공") : TEXT("실패");
	UE_LOG(LogTemp, Display, TEXT("ComfyUI 서버 연결 확인 %s - HTTP %d: %s"), ResultText, LastHttpStatusCode, *LastResponseMessage);
}

void AComfyUIWorkflowRequestActor::SetFailureMessage(const FString& FailureMessage)
{
	bLastRequestSucceeded = false;
	LastHttpStatusCode = 0;
	LastResponseMessage = FailureMessage;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *LastResponseMessage);
}
