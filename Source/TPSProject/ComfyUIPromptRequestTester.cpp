// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUIPromptRequestTester.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogComfyUIPromptRequestTester, Log, All);

AComfyUIPromptRequestTester::AComfyUIPromptRequestTester()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AComfyUIPromptRequestTester::SendPromptRequest()
{
	if (bRequestInProgress)
	{
		SetFailureResult(TEXT("ComfyUI 요청이 이미 진행 중입니다."));
		return;
	}

	FString RequestBody;
	if (!BuildPromptRequestBody(RequestBody))
	{
		return;
	}

	const FString EndpointUrl = BuildPromptEndpointUrl();
	if (EndpointUrl.IsEmpty())
	{
		SetFailureResult(TEXT("ComfyUI 서버 주소가 비어 있습니다."));
		return;
	}

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(EndpointUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->OnProcessRequestComplete().BindUObject(this, &AComfyUIPromptRequestTester::HandlePromptResponse);

	bRequestInProgress = true;
	bLastRequestSucceeded = false;
	LastHttpStatusCode = 0;
	LastStatusMessage = FString::Printf(TEXT("ComfyUI /prompt 요청 전송 중: %s"), *EndpointUrl);
	LastResponseBody.Reset();

	UE_LOG(LogComfyUIPromptRequestTester, Display, TEXT("%s"), *LastStatusMessage);

	if (!Request->ProcessRequest())
	{
		bRequestInProgress = false;
		SetFailureResult(TEXT("HTTP 요청 시작에 실패했습니다."));
	}
}

bool AComfyUIPromptRequestTester::BuildPromptRequestBody(FString& OutRequestBody)
{
	const FString RawPath = WorkflowJsonFilePath.FilePath;
	if (RawPath.IsEmpty())
	{
		SetFailureResult(TEXT("workflow JSON 파일 경로가 비어 있습니다."));
		return false;
	}

	FString FullPath;
	FString TriedPaths;
	if (!ResolveWorkflowJsonFullPath(FullPath, TriedPaths))
	{
		SetFailureResult(FString::Printf(TEXT("workflow JSON 파일을 읽을 수 없습니다. 원본 경로: %s / 시도한 경로: %s"), *RawPath, *TriedPaths));
		return false;
	}

	FString WorkflowJsonText;
	if (!FFileHelper::LoadFileToString(WorkflowJsonText, *FullPath))
	{
		SetFailureResult(FString::Printf(TEXT("workflow JSON 파일을 읽을 수 없습니다: %s"), *FullPath));
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(WorkflowJsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		SetFailureResult(FString::Printf(TEXT("workflow JSON 파싱에 실패했습니다: %s"), *FullPath));
		return false;
	}

	if (RootValue->Type != EJson::Object)
	{
		SetFailureResult(TEXT("ComfyUI workflow JSON의 최상위 값은 Object여야 합니다."));
		return false;
	}

	const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
	TSharedPtr<FJsonObject> RequestObject;
	if (RootObject.IsValid() && RootObject->HasTypedField<EJson::Array>(TEXT("nodes")) && RootObject->HasTypedField<EJson::Array>(TEXT("links")) && !RootObject->HasField(TEXT("prompt")))
	{
		TSharedPtr<FJsonObject> ConvertedPromptObject;
		if (!ConvertUiWorkflowToApiPrompt(RootObject, ConvertedPromptObject))
		{
			SetFailureResult(FString::Printf(TEXT("ComfyUI UI workflow JSON을 /prompt API 포맷으로 변환할 수 없습니다: %s"), *FullPath));
			return false;
		}

		const TSharedPtr<FJsonObject> ConvertedRequestObject = MakeShared<FJsonObject>();
		ConvertedRequestObject->SetObjectField(TEXT("prompt"), ConvertedPromptObject);
		RequestObject = ConvertedRequestObject;
		UE_LOG(LogComfyUIPromptRequestTester, Display, TEXT("ComfyUI UI workflow JSON을 /prompt API 포맷으로 변환했습니다: %s"), *FullPath);
	}
	else if (RootObject.IsValid() && RootObject->HasField(TEXT("prompt")))
	{
		RequestObject = RootObject;
	}
	else
	{
		RequestObject = MakeShared<FJsonObject>();
		RequestObject->SetField(TEXT("prompt"), RootValue);
	}

	if (!ApplyWorkflowInputOverrides(RequestObject))
	{
		return false;
	}

	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutRequestBody);
	if (!FJsonSerializer::Serialize(RequestObject.ToSharedRef(), Writer))
	{
		SetFailureResult(TEXT("ComfyUI 요청 본문 생성에 실패했습니다."));
		return false;
	}

	return true;
}

bool AComfyUIPromptRequestTester::ApplyWorkflowInputOverrides(const TSharedPtr<FJsonObject>& RequestObject)
{
	if (!RequestObject.IsValid())
	{
		SetFailureResult(TEXT("ComfyUI 요청 본문이 유효하지 않아 workflow 입력값을 치환할 수 없습니다."));
		return false;
	}

	const TSharedPtr<FJsonObject>* PromptObjectPtr = nullptr;
	if (!RequestObject->TryGetObjectField(TEXT("prompt"), PromptObjectPtr) || !PromptObjectPtr || !PromptObjectPtr->IsValid())
	{
		SetFailureResult(TEXT("ComfyUI 요청 본문의 prompt Object를 찾을 수 없어 workflow 입력값을 치환할 수 없습니다."));
		return false;
	}

	const TSharedPtr<FJsonObject>& PromptObject = *PromptObjectPtr;
	FComfyUIWorkflowInputTarget ResolvedPositivePromptTarget;
	if (!ResolvePositivePromptTarget(PromptObject, ResolvedPositivePromptTarget))
	{
		return false;
	}

	FComfyUIWorkflowInputTarget ResolvedImageWidthTarget;
	if (!ResolveClassInputTarget(PromptObject, ImageWidthTarget, TEXT("이미지 width"), TEXT("EmptyLatentImage"), TEXT("width"), ResolvedImageWidthTarget))
	{
		return false;
	}

	FComfyUIWorkflowInputTarget ResolvedImageHeightTarget;
	if (!ResolveClassInputTarget(PromptObject, ImageHeightTarget, TEXT("이미지 height"), TEXT("EmptyLatentImage"), TEXT("height"), ResolvedImageHeightTarget))
	{
		return false;
	}

	if (!ApplyStringWorkflowInputOverride(PromptObject, ResolvedPositivePromptTarget, PositivePrompt, TEXT("긍정 프롬프트")))
	{
		return false;
	}
	if (!ApplyNumberWorkflowInputOverride(PromptObject, ResolvedImageWidthTarget, ImageWidth, TEXT("이미지 width")))
	{
		return false;
	}
	if (!ApplyNumberWorkflowInputOverride(PromptObject, ResolvedImageHeightTarget, ImageHeight, TEXT("이미지 height")))
	{
		return false;
	}

	return true;
}

bool AComfyUIPromptRequestTester::ResolvePositivePromptTarget(const TSharedPtr<FJsonObject>& PromptObject, FComfyUIWorkflowInputTarget& OutTarget)
{
	if (IsWorkflowInputTargetComplete(PositivePromptTarget))
	{
		OutTarget = PositivePromptTarget;
		return true;
	}

	if (!IsWorkflowInputTargetEmpty(PositivePromptTarget))
	{
		SetFailureResult(TEXT("긍정 프롬프트 치환 대상 설정은 node id와 input key를 모두 입력하거나 모두 비워야 합니다."));
		return false;
	}

	if (!PromptObject.IsValid())
	{
		SetFailureResult(TEXT("긍정 프롬프트 치환 대상을 자동으로 찾을 수 없습니다. prompt Object가 유효하지 않습니다."));
		return false;
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& NodePair : PromptObject->Values)
	{
		const TSharedPtr<FJsonObject>* NodeObjectPtr = nullptr;
		if (!NodePair.Value.IsValid() || !NodePair.Value->TryGetObject(NodeObjectPtr) || !NodeObjectPtr || !NodeObjectPtr->IsValid())
		{
			continue;
		}

		FString ClassType;
		if (!(*NodeObjectPtr)->TryGetStringField(TEXT("class_type"), ClassType) || ClassType != TEXT("KSampler"))
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* InputsObjectPtr = nullptr;
		if (!(*NodeObjectPtr)->TryGetObjectField(TEXT("inputs"), InputsObjectPtr) || !InputsObjectPtr || !InputsObjectPtr->IsValid())
		{
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* PositiveLinkValues = nullptr;
		if (!(*InputsObjectPtr)->TryGetArrayField(TEXT("positive"), PositiveLinkValues) || !PositiveLinkValues || PositiveLinkValues->Num() == 0 || !(*PositiveLinkValues)[0].IsValid())
		{
			continue;
		}

		const FString PositiveNodeId = (*PositiveLinkValues)[0]->AsString();
		const TSharedPtr<FJsonObject>* PositiveNodeObjectPtr = nullptr;
		if (!PromptObject->TryGetObjectField(PositiveNodeId, PositiveNodeObjectPtr) || !PositiveNodeObjectPtr || !PositiveNodeObjectPtr->IsValid())
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* PositiveInputsObjectPtr = nullptr;
		if ((*PositiveNodeObjectPtr)->TryGetObjectField(TEXT("inputs"), PositiveInputsObjectPtr) && PositiveInputsObjectPtr && PositiveInputsObjectPtr->IsValid() && (*PositiveInputsObjectPtr)->HasField(TEXT("text")))
		{
			OutTarget.NodeId = PositiveNodeId;
			OutTarget.InputKey = TEXT("text");
			UE_LOG(LogComfyUIPromptRequestTester, Display, TEXT("긍정 프롬프트 치환 대상을 자동으로 찾았습니다. node id: %s / input key: text"), *PositiveNodeId);
			return true;
		}
	}

	return ResolveClassInputTarget(PromptObject, PositivePromptTarget, TEXT("긍정 프롬프트"), TEXT("CLIPTextEncode"), TEXT("text"), OutTarget);
}

bool AComfyUIPromptRequestTester::ResolveClassInputTarget(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& ConfiguredTarget, const FString& Label, const FString& ClassType, const FString& InputKey, FComfyUIWorkflowInputTarget& OutTarget)
{
	if (IsWorkflowInputTargetComplete(ConfiguredTarget))
	{
		OutTarget = ConfiguredTarget;
		return true;
	}

	if (!IsWorkflowInputTargetEmpty(ConfiguredTarget))
	{
		SetFailureResult(FString::Printf(TEXT("%s 치환 대상 설정은 node id와 input key를 모두 입력하거나 모두 비워야 합니다."), *Label));
		return false;
	}

	if (!PromptObject.IsValid())
	{
		SetFailureResult(FString::Printf(TEXT("%s 치환 대상을 자동으로 찾을 수 없습니다. prompt Object가 유효하지 않습니다."), *Label));
		return false;
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& NodePair : PromptObject->Values)
	{
		const TSharedPtr<FJsonObject>* NodeObjectPtr = nullptr;
		if (!NodePair.Value.IsValid() || !NodePair.Value->TryGetObject(NodeObjectPtr) || !NodeObjectPtr || !NodeObjectPtr->IsValid())
		{
			continue;
		}

		FString FoundClassType;
		if (!(*NodeObjectPtr)->TryGetStringField(TEXT("class_type"), FoundClassType) || FoundClassType != ClassType)
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* InputsObjectPtr = nullptr;
		if ((*NodeObjectPtr)->TryGetObjectField(TEXT("inputs"), InputsObjectPtr) && InputsObjectPtr && InputsObjectPtr->IsValid() && (*InputsObjectPtr)->HasField(InputKey))
		{
			OutTarget.NodeId = NodePair.Key;
			OutTarget.InputKey = InputKey;
			UE_LOG(LogComfyUIPromptRequestTester, Display, TEXT("%s 치환 대상을 자동으로 찾았습니다. node id: %s / input key: %s"), *Label, *NodePair.Key, *InputKey);
			return true;
		}
	}

	SetFailureResult(FString::Printf(TEXT("%s 치환 대상을 자동으로 찾을 수 없습니다. class_type: %s / input key: %s"), *Label, *ClassType, *InputKey));
	return false;
}

bool AComfyUIPromptRequestTester::IsWorkflowInputTargetEmpty(const FComfyUIWorkflowInputTarget& Target) const
{
	FString CleanNodeId = Target.NodeId;
	CleanNodeId.TrimStartAndEndInline();

	FString CleanInputKey = Target.InputKey;
	CleanInputKey.TrimStartAndEndInline();

	return CleanNodeId.IsEmpty() && CleanInputKey.IsEmpty();
}

bool AComfyUIPromptRequestTester::IsWorkflowInputTargetComplete(const FComfyUIWorkflowInputTarget& Target) const
{
	FString CleanNodeId = Target.NodeId;
	CleanNodeId.TrimStartAndEndInline();

	FString CleanInputKey = Target.InputKey;
	CleanInputKey.TrimStartAndEndInline();

	return !CleanNodeId.IsEmpty() && !CleanInputKey.IsEmpty();
}

bool AComfyUIPromptRequestTester::ApplyStringWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& Target, const FString& NewValue, const FString& Label)
{
	TSharedPtr<FJsonObject> InputsObject;
	if (!FindWorkflowInputsObject(PromptObject, Target, Label, InputsObject))
	{
		return false;
	}

	FString CleanInputKey = Target.InputKey;
	CleanInputKey.TrimStartAndEndInline();
	InputsObject->SetStringField(CleanInputKey, NewValue);
	return true;
}

bool AComfyUIPromptRequestTester::ApplyNumberWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& Target, int32 NewValue, const FString& Label)
{
	TSharedPtr<FJsonObject> InputsObject;
	if (!FindWorkflowInputsObject(PromptObject, Target, Label, InputsObject))
	{
		return false;
	}

	FString CleanInputKey = Target.InputKey;
	CleanInputKey.TrimStartAndEndInline();
	InputsObject->SetNumberField(CleanInputKey, NewValue);
	return true;
}

bool AComfyUIPromptRequestTester::FindWorkflowInputsObject(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& Target, const FString& Label, TSharedPtr<FJsonObject>& OutInputsObject)
{
	FString CleanNodeId = Target.NodeId;
	CleanNodeId.TrimStartAndEndInline();

	FString CleanInputKey = Target.InputKey;
	CleanInputKey.TrimStartAndEndInline();

	if (CleanNodeId.IsEmpty() || CleanInputKey.IsEmpty())
	{
		SetFailureResult(FString::Printf(TEXT("%s 치환 대상 설정이 비어 있습니다. node id와 input key를 모두 지정하세요."), *Label));
		return false;
	}

	if (!PromptObject.IsValid())
	{
		SetFailureResult(FString::Printf(TEXT("%s 치환 실패. prompt Object가 유효하지 않습니다. node id: %s / input key: %s"), *Label, *CleanNodeId, *CleanInputKey));
		return false;
	}

	const TSharedPtr<FJsonObject>* NodeObjectPtr = nullptr;
	if (!PromptObject->TryGetObjectField(CleanNodeId, NodeObjectPtr) || !NodeObjectPtr || !NodeObjectPtr->IsValid())
	{
		SetFailureResult(FString::Printf(TEXT("%s 치환 실패. workflow prompt에서 node id를 찾을 수 없습니다. node id: %s / input key: %s"), *Label, *CleanNodeId, *CleanInputKey));
		return false;
	}

	const TSharedPtr<FJsonObject>* InputsObjectPtr = nullptr;
	if (!(*NodeObjectPtr)->TryGetObjectField(TEXT("inputs"), InputsObjectPtr) || !InputsObjectPtr || !InputsObjectPtr->IsValid())
	{
		SetFailureResult(FString::Printf(TEXT("%s 치환 실패. workflow node에 inputs Object가 없습니다. node id: %s / input key: %s"), *Label, *CleanNodeId, *CleanInputKey));
		return false;
	}

	if (!(*InputsObjectPtr)->HasField(CleanInputKey))
	{
		SetFailureResult(FString::Printf(TEXT("%s 치환 실패. workflow node inputs에서 input key를 찾을 수 없습니다. node id: %s / input key: %s"), *Label, *CleanNodeId, *CleanInputKey));
		return false;
	}

	OutInputsObject = *InputsObjectPtr;
	return true;
}

bool AComfyUIPromptRequestTester::ConvertUiWorkflowToApiPrompt(const TSharedPtr<FJsonObject>& UiWorkflowObject, TSharedPtr<FJsonObject>& OutPromptObject) const
{
	if (!UiWorkflowObject.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* NodeValues = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* LinkValues = nullptr;
	if (!UiWorkflowObject->TryGetArrayField(TEXT("nodes"), NodeValues) || !UiWorkflowObject->TryGetArrayField(TEXT("links"), LinkValues))
	{
		return false;
	}

	TMap<int32, TPair<int32, int32>> LinkSourceById;
	for (const TSharedPtr<FJsonValue>& LinkValue : *LinkValues)
	{
		const TArray<TSharedPtr<FJsonValue>>* LinkArray = nullptr;
		if (!LinkValue.IsValid() || !LinkValue->TryGetArray(LinkArray) || !LinkArray || LinkArray->Num() < 4)
		{
			continue;
		}

		const int32 LinkId = static_cast<int32>((*LinkArray)[0]->AsNumber());
		const int32 SourceNodeId = static_cast<int32>((*LinkArray)[1]->AsNumber());
		const int32 SourceOutputIndex = static_cast<int32>((*LinkArray)[2]->AsNumber());
		LinkSourceById.Add(LinkId, TPair<int32, int32>(SourceNodeId, SourceOutputIndex));
	}

	OutPromptObject = MakeShared<FJsonObject>();

	for (const TSharedPtr<FJsonValue>& NodeValue : *NodeValues)
	{
		const TSharedPtr<FJsonObject>* NodeObjectPtr = nullptr;
		if (!NodeValue.IsValid() || !NodeValue->TryGetObject(NodeObjectPtr) || !NodeObjectPtr || !NodeObjectPtr->IsValid())
		{
			continue;
		}

		const TSharedPtr<FJsonObject>& NodeObject = *NodeObjectPtr;
		if (!NodeObject->HasField(TEXT("id")) || !NodeObject->HasTypedField<EJson::String>(TEXT("type")))
		{
			continue;
		}

		const int32 NodeId = static_cast<int32>(NodeObject->GetNumberField(TEXT("id")));
		const FString NodeType = NodeObject->GetStringField(TEXT("type"));
		const TSharedPtr<FJsonObject> ApiNodeObject = MakeShared<FJsonObject>();
		const TSharedPtr<FJsonObject> ApiInputsObject = MakeShared<FJsonObject>();

		const TArray<TSharedPtr<FJsonValue>>* InputValues = nullptr;
		if (NodeObject->TryGetArrayField(TEXT("inputs"), InputValues))
		{
			for (const TSharedPtr<FJsonValue>& InputValue : *InputValues)
			{
				const TSharedPtr<FJsonObject>* InputObjectPtr = nullptr;
				if (!InputValue.IsValid() || !InputValue->TryGetObject(InputObjectPtr) || !InputObjectPtr || !InputObjectPtr->IsValid())
				{
					continue;
				}

				const TSharedPtr<FJsonObject>& InputObject = *InputObjectPtr;
				FString InputName;
				if (!InputObject->TryGetStringField(TEXT("name"), InputName))
				{
					continue;
				}

				double LinkIdNumber = 0.0;
				if (!InputObject->TryGetNumberField(TEXT("link"), LinkIdNumber))
				{
					continue;
				}

				const TPair<int32, int32>* LinkSource = LinkSourceById.Find(static_cast<int32>(LinkIdNumber));
				if (!LinkSource)
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> LinkReference;
				LinkReference.Add(MakeShared<FJsonValueString>(FString::FromInt(LinkSource->Key)));
				LinkReference.Add(MakeShared<FJsonValueNumber>(LinkSource->Value));
				ApiInputsObject->SetArrayField(InputName, LinkReference);
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* WidgetValues = nullptr;
		if (NodeObject->TryGetArrayField(TEXT("widgets_values"), WidgetValues))
		{
			AddWidgetValuesToApiInputs(NodeType, *WidgetValues, ApiInputsObject);
		}
		else
		{
			const TSharedPtr<FJsonObject>* WidgetObjectPtr = nullptr;
			if (NodeObject->TryGetObjectField(TEXT("widgets_values"), WidgetObjectPtr) && WidgetObjectPtr && WidgetObjectPtr->IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& WidgetPair : (*WidgetObjectPtr)->Values)
				{
					ApiInputsObject->SetField(WidgetPair.Key, WidgetPair.Value);
				}
			}
		}

		ApiNodeObject->SetStringField(TEXT("class_type"), NodeType);
		ApiNodeObject->SetObjectField(TEXT("inputs"), ApiInputsObject);
		OutPromptObject->SetObjectField(FString::FromInt(NodeId), ApiNodeObject);
	}

	return OutPromptObject->Values.Num() > 0;
}

void AComfyUIPromptRequestTester::AddWidgetValuesToApiInputs(const FString& NodeType, const TArray<TSharedPtr<FJsonValue>>& WidgetValues, const TSharedPtr<FJsonObject>& ApiInputsObject) const
{
	if (!ApiInputsObject.IsValid())
	{
		return;
	}

	const TArray<FString> InputNames = GetKnownWidgetInputNames(NodeType);
	if (InputNames.Num() == 0)
	{
		UE_LOG(LogComfyUIPromptRequestTester, Verbose, TEXT("ComfyUI UI workflow 변환 중 widgets_values 매핑을 모르는 노드를 건너뜁니다: %s"), *NodeType);
		return;
	}

	const int32 Count = FMath::Min(InputNames.Num(), WidgetValues.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (InputNames[Index].IsEmpty() || !WidgetValues[Index].IsValid())
		{
			continue;
		}

		ApiInputsObject->SetField(InputNames[Index], WidgetValues[Index]);
	}
}

TArray<FString> AComfyUIPromptRequestTester::GetKnownWidgetInputNames(const FString& NodeType) const
{
	if (NodeType == TEXT("CheckpointLoaderSimple"))
	{
		return { TEXT("ckpt_name") };
	}
	if (NodeType == TEXT("UNETLoader"))
	{
		return { TEXT("unet_name"), TEXT("weight_dtype") };
	}
	if (NodeType == TEXT("CLIPLoader"))
	{
		return { TEXT("clip_name"), TEXT("type"), TEXT("device") };
	}
	if (NodeType == TEXT("DualCLIPLoader"))
	{
		return { TEXT("clip_name1"), TEXT("clip_name2"), TEXT("type"), TEXT("device") };
	}
	if (NodeType == TEXT("TripleCLIPLoader"))
	{
		return { TEXT("clip_name1"), TEXT("clip_name2"), TEXT("clip_name3") };
	}
	if (NodeType == TEXT("CLIPTextEncode"))
	{
		return { TEXT("text") };
	}
	if (NodeType == TEXT("EmptyLatentImage"))
	{
		return { TEXT("width"), TEXT("height"), TEXT("batch_size") };
	}
	if (NodeType == TEXT("KSampler"))
	{
		return { TEXT("seed"), FString(), TEXT("steps"), TEXT("cfg"), TEXT("sampler_name"), TEXT("scheduler"), TEXT("denoise") };
	}
	if (NodeType == TEXT("VAELoader"))
	{
		return { TEXT("vae_name") };
	}
	if (NodeType == TEXT("LoraLoader"))
	{
		return { TEXT("lora_name"), TEXT("strength_model"), TEXT("strength_clip") };
	}
	if (NodeType == TEXT("SaveImage"))
	{
		return { TEXT("filename_prefix") };
	}
	if (NodeType == TEXT("LoadImage"))
	{
		return { TEXT("image"), FString() };
	}
	if (NodeType == TEXT("LoadImageMask"))
	{
		return { TEXT("image"), TEXT("channel") };
	}

	return TArray<FString>();
}

bool AComfyUIPromptRequestTester::ResolveWorkflowJsonFullPath(FString& OutFullPath, FString& OutTriedPaths) const
{
	TArray<FString> CandidatePaths;

	FString RawPath = WorkflowJsonFilePath.FilePath;
	RawPath.TrimStartAndEndInline();
	FPaths::NormalizeFilename(RawPath);

	FString CleanRawPath = RawPath;
	FPaths::CollapseRelativeDirectories(CleanRawPath);
	CandidatePaths.AddUnique(CleanRawPath);

	if (FPaths::IsRelative(RawPath))
	{
		FString ProjectRelativePath = FPaths::Combine(FPaths::ProjectDir(), RawPath);
		FPaths::NormalizeFilename(ProjectRelativePath);
		FPaths::CollapseRelativeDirectories(ProjectRelativePath);
		CandidatePaths.AddUnique(ProjectRelativePath);
	}
	else
	{
		FString AbsolutePath = FPaths::ConvertRelativePathToFull(RawPath);
		FPaths::NormalizeFilename(AbsolutePath);
		FPaths::CollapseRelativeDirectories(AbsolutePath);
		CandidatePaths.AddUnique(AbsolutePath);
	}

	OutTriedPaths.Reset();
	for (const FString& CandidatePath : CandidatePaths)
	{
		if (!OutTriedPaths.IsEmpty())
		{
			OutTriedPaths += TEXT(" | ");
		}
		OutTriedPaths += CandidatePath;

		if (IFileManager::Get().FileExists(*CandidatePath))
		{
			OutFullPath = CandidatePath;
			return true;
		}
	}

	return false;
}

FString AComfyUIPromptRequestTester::BuildPromptEndpointUrl() const
{
	FString BaseUrl = ServerBaseUrl;
	BaseUrl.TrimStartAndEndInline();
	BaseUrl.RemoveFromEnd(TEXT("/"));

	if (BaseUrl.IsEmpty())
	{
		return FString();
	}

	return FString::Printf(TEXT("%s/prompt"), *BaseUrl);
}

void AComfyUIPromptRequestTester::HandlePromptResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	bRequestInProgress = false;
	LastHttpStatusCode = Response.IsValid() ? Response->GetResponseCode() : 0;
	LastResponseBody = Response.IsValid() ? Response->GetContentAsString() : FString();

	const bool bHttpSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(LastHttpStatusCode);
	bLastRequestSucceeded = bHttpSuccess;

	if (bHttpSuccess)
	{
		LastStatusMessage = FString::Printf(TEXT("ComfyUI /prompt 요청 성공. HTTP %d"), LastHttpStatusCode);
		UE_LOG(LogComfyUIPromptRequestTester, Display, TEXT("%s Response: %s"), *LastStatusMessage, *LastResponseBody);
		return;
	}

	if (LastHttpStatusCode == 500)
	{
		LastStatusMessage = TEXT("ComfyUI /prompt 요청 실패. HTTP 500. workflow JSON이 /prompt API 포맷인지 확인하세요.");
	}
	else
	{
		LastStatusMessage = FString::Printf(TEXT("ComfyUI /prompt 요청 실패. HTTP %d"), LastHttpStatusCode);
	}
	UE_LOG(LogComfyUIPromptRequestTester, Warning, TEXT("%s Response: %s"), *LastStatusMessage, *LastResponseBody);
}

void AComfyUIPromptRequestTester::SetFailureResult(const FString& FailureMessage)
{
	bRequestInProgress = false;
	bLastRequestSucceeded = false;
	LastHttpStatusCode = 0;
	LastStatusMessage = FailureMessage;
	LastResponseBody.Reset();

	UE_LOG(LogComfyUIPromptRequestTester, Warning, TEXT("%s"), *LastStatusMessage);
}
