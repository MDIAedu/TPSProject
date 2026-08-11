// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUIImageGenerationWidgetController.h"

#include "Containers/Ticker.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_EDITOR
#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "Factories/TextureFactory.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogComfyUIImageGenerationWidgetController, Log, All);

UComfyUIImageGenerationWidgetController::UComfyUIImageGenerationWidgetController()
{
	ContentSaveFolder.Path = TEXT("/Game/GeneratedTextures");
}

void UComfyUIImageGenerationWidgetController::CreateImage()
{
	if (bRequestInProgress)
	{
		SetFailureResult(TEXT("ComfyUI 이미지 생성 요청이 이미 진행 중입니다."));
		return;
	}

	if (ImageWidth <= 0 || ImageHeight <= 0)
	{
		SetFailureResult(TEXT("이미지 width와 height는 1 이상이어야 합니다."));
		return;
	}

	FString TextureDestinationPath;
	if (!ResolveContentSaveFolderPackagePath(TextureDestinationPath))
	{
		SetFailureResult(TEXT("Texture import 폴더는 /Game 하위 경로여야 합니다."));
		return;
	}

	FString RequestBody;
	if (!BuildPromptRequestBody(RequestBody))
	{
		return;
	}

	const FString EndpointUrl = BuildEndpointUrl(TEXT("prompt"));
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
	Request->OnProcessRequestComplete().BindUObject(this, &UComfyUIImageGenerationWidgetController::HandlePromptResponse);

	AddToRoot();
	bRequestInProgress = true;
	bLastRequestSucceeded = false;
	GenerationState = EComfyUIImageGenerationState::Generating;
	LastHttpStatusCode = 0;
	LastPromptId.Reset();
	LastSavedImagePath.Reset();
	LastImportedTextureAssetPath.Reset();
	LastResponseBody.Reset();
	HistoryPollAttempts = 0;
	LastStatusMessage = FString::Printf(TEXT("ComfyUI 이미지 생성 요청 전송 중: %s"), *EndpointUrl);

	UE_LOG(LogComfyUIImageGenerationWidgetController, Display, TEXT("%s"), *LastStatusMessage);

	if (!Request->ProcessRequest())
	{
		SetFailureResult(TEXT("HTTP 요청 시작에 실패했습니다."));
	}
}

void UComfyUIImageGenerationWidgetController::ResetGenerationState()
{
	if (bRequestInProgress)
	{
		SetFailureResult(TEXT("진행 중인 요청이 있어 상태를 초기화할 수 없습니다."));
		return;
	}

	bLastRequestSucceeded = false;
	GenerationState = EComfyUIImageGenerationState::Idle;
	LastHttpStatusCode = 0;
	LastPromptId.Reset();
	LastSavedImagePath.Reset();
	LastImportedTextureAssetPath.Reset();
	LastStatusMessage.Reset();
	LastResponseBody.Reset();
	HistoryPollAttempts = 0;
}

bool UComfyUIImageGenerationWidgetController::ImportLastSavedImageAsTexture()
{
	if (LastSavedImagePath.IsEmpty())
	{
		SetFailureResult(TEXT("Texture로 import할 저장 이미지 파일 경로가 없습니다."));
		return false;
	}

	return ImportImageFileAsTexture(LastSavedImagePath);
}

bool UComfyUIImageGenerationWidgetController::ImportImageFileAsTexture(const FString& ImageFilePath)
{
	LastImportedTextureAssetPath.Reset();

#if WITH_EDITOR
	FString CleanImageFilePath = ImageFilePath;
	CleanImageFilePath.TrimStartAndEndInline();
	FPaths::NormalizeFilename(CleanImageFilePath);

	if (CleanImageFilePath.IsEmpty())
	{
		SetFailureResult(TEXT("Texture로 import할 이미지 파일 경로가 비어 있습니다."));
		return false;
	}

	if (!IFileManager::Get().FileExists(*CleanImageFilePath))
	{
		SetFailureResult(FString::Printf(TEXT("Texture로 import할 이미지 파일을 찾을 수 없습니다: %s"), *CleanImageFilePath));
		return false;
	}

	FString DestinationPath;
	if (!ResolveContentSaveFolderPackagePath(DestinationPath))
	{
		SetFailureResult(TEXT("Texture import 폴더는 /Game 하위 경로여야 합니다."));
		return false;
	}

	const FString AssetNameBase = BuildTextureAssetNameBase(CleanImageFilePath);
	const FString BasePackageName = DestinationPath / AssetNameBase;

	FString UniquePackageName;
	FString UniqueAssetName;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, TEXT(""), UniquePackageName, UniqueAssetName);

	UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
	TextureFactory->AddToRoot();

	UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
	ImportTask->Filename = CleanImageFilePath;
	ImportTask->DestinationPath = DestinationPath;
	ImportTask->DestinationName = UniqueAssetName;
	ImportTask->Factory = TextureFactory;
	ImportTask->bAutomated = true;
	ImportTask->bSave = false;
	ImportTask->bReplaceExisting = false;

	TArray<UAssetImportTask*> ImportTasks;
	ImportTasks.Add(ImportTask);
	AssetToolsModule.Get().ImportAssetTasks(ImportTasks);

	TextureFactory->RemoveFromRoot();

	if (ImportTask->ImportedObjectPaths.Num() == 0)
	{
		SetFailureResult(FString::Printf(TEXT("Texture2D asset import에 실패했습니다: %s"), *CleanImageFilePath));
		return false;
	}

	UTexture2D* ImportedTexture = nullptr;
	for (const FString& ImportedObjectPath : ImportTask->ImportedObjectPaths)
	{
		ImportedTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *ImportedObjectPath));
		if (ImportedTexture)
		{
			LastImportedTextureAssetPath = ImportedObjectPath;
			break;
		}
	}

	if (!ImportedTexture)
	{
		SetFailureResult(TEXT("import 결과에서 Texture2D asset을 찾을 수 없습니다."));
		return false;
	}

	ApplyImportedTextureSettings(ImportedTexture);
	ImportedTexture->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(ImportedTexture);

	UPackage* Package = ImportedTexture->GetOutermost();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (!UPackage::SavePackage(Package, ImportedTexture, *PackageFileName, SaveArgs))
	{
		SetFailureResult(FString::Printf(TEXT("Texture2D asset 저장에 실패했습니다: %s"), *LastImportedTextureAssetPath));
		return false;
	}

	SetSuccessResult(FString::Printf(TEXT("Texture2D asset import 완료: %s"), *LastImportedTextureAssetPath));
	return true;
#else
	SetFailureResult(TEXT("Texture2D import는 Unreal Editor에서만 사용할 수 있습니다."));
	return false;
#endif
}

bool UComfyUIImageGenerationWidgetController::SelectWorkflowJsonFile(FString& OutFilePath)
{
	OutFilePath.Reset();

#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return false;
	}

	TArray<FString> SelectedFiles;
	const FString DefaultPath = FPaths::ProjectDir();
	const void* ParentWindowHandle = FSlateApplication::IsInitialized() && FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr)
		? FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr)
		: nullptr;

	const bool bSelected = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Select ComfyUI Workflow JSON"),
		DefaultPath,
		TEXT(""),
		TEXT("JSON files (*.json)|*.json|All files (*.*)|*.*"),
		EFileDialogFlags::None,
		SelectedFiles);

	if (!bSelected || SelectedFiles.Num() == 0)
	{
		return false;
	}

	OutFilePath = SelectedFiles[0];
	FPaths::NormalizeFilename(OutFilePath);
	return true;
#else
	return false;
#endif
}

bool UComfyUIImageGenerationWidgetController::SelectContentSaveFolder(FString& OutContentFolderPath)
{
	OutContentFolderPath.Reset();

#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return false;
	}

	FString SelectedFolder;
	const FString ContentDir = FPaths::ProjectContentDir();
	const void* ParentWindowHandle = FSlateApplication::IsInitialized() && FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr)
		? FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr)
		: nullptr;

	const bool bSelected = DesktopPlatform->OpenDirectoryDialog(
		ParentWindowHandle,
		TEXT("Select Unreal Content Save Folder"),
		ContentDir,
		SelectedFolder);

	if (!bSelected || SelectedFolder.IsEmpty())
	{
		return false;
	}

	FPaths::NormalizeDirectoryName(SelectedFolder);
	FString NormalizedContentDir = ContentDir;
	FPaths::NormalizeDirectoryName(NormalizedContentDir);

	if (SelectedFolder == NormalizedContentDir)
	{
		OutContentFolderPath = TEXT("/Game");
		return true;
	}

	FString RelativeFolder = SelectedFolder;
	if (!FPaths::MakePathRelativeTo(RelativeFolder, *NormalizedContentDir))
	{
		SetFailureResult(TEXT("선택한 저장 폴더는 프로젝트 Content 폴더 안에 있어야 합니다."));
		return false;
	}

	FPaths::NormalizeFilename(RelativeFolder);
	FPaths::CollapseRelativeDirectories(RelativeFolder);
	RelativeFolder.RemoveFromStart(TEXT("Content/"));
	if (RelativeFolder.IsEmpty() || RelativeFolder.StartsWith(TEXT("..")))
	{
		SetFailureResult(TEXT("선택한 저장 폴더는 프로젝트 Content 폴더 안에 있어야 합니다."));
		return false;
	}

	OutContentFolderPath = FString::Printf(TEXT("/Game/%s"), *RelativeFolder);
	return true;
#else
	return false;
#endif
}

bool UComfyUIImageGenerationWidgetController::BuildPromptRequestBody(FString& OutRequestBody)
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

		RequestObject = MakeShared<FJsonObject>();
		RequestObject->SetObjectField(TEXT("prompt"), ConvertedPromptObject);
		UE_LOG(LogComfyUIImageGenerationWidgetController, Display, TEXT("ComfyUI UI workflow JSON을 /prompt API 포맷으로 변환했습니다: %s"), *FullPath);
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

bool UComfyUIImageGenerationWidgetController::ApplyWorkflowInputOverrides(const TSharedPtr<FJsonObject>& RequestObject)
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
	FComfyUIImageWorkflowInputTarget ResolvedPositivePromptTarget;
	if (!ResolvePositivePromptTarget(PromptObject, ResolvedPositivePromptTarget))
	{
		return false;
	}

	FComfyUIImageWorkflowInputTarget ResolvedImageWidthTarget;
	if (!ResolveClassInputTarget(PromptObject, ImageWidthTarget, TEXT("이미지 width"), TEXT("EmptyLatentImage"), TEXT("width"), ResolvedImageWidthTarget))
	{
		return false;
	}

	FComfyUIImageWorkflowInputTarget ResolvedImageHeightTarget;
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

bool UComfyUIImageGenerationWidgetController::ResolvePositivePromptTarget(const TSharedPtr<FJsonObject>& PromptObject, FComfyUIImageWorkflowInputTarget& OutTarget)
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
			UE_LOG(LogComfyUIImageGenerationWidgetController, Display, TEXT("긍정 프롬프트 치환 대상을 자동으로 찾았습니다. node id: %s / input key: text"), *PositiveNodeId);
			return true;
		}
	}

	return ResolveClassInputTarget(PromptObject, PositivePromptTarget, TEXT("긍정 프롬프트"), TEXT("CLIPTextEncode"), TEXT("text"), OutTarget);
}

bool UComfyUIImageGenerationWidgetController::ResolveClassInputTarget(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& ConfiguredTarget, const FString& Label, const FString& ClassType, const FString& InputKey, FComfyUIImageWorkflowInputTarget& OutTarget)
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
			UE_LOG(LogComfyUIImageGenerationWidgetController, Display, TEXT("%s 치환 대상을 자동으로 찾았습니다. node id: %s / input key: %s"), *Label, *NodePair.Key, *InputKey);
			return true;
		}
	}

	SetFailureResult(FString::Printf(TEXT("%s 치환 대상을 자동으로 찾을 수 없습니다. class_type: %s / input key: %s"), *Label, *ClassType, *InputKey));
	return false;
}

bool UComfyUIImageGenerationWidgetController::IsWorkflowInputTargetEmpty(const FComfyUIImageWorkflowInputTarget& Target) const
{
	FString CleanNodeId = Target.NodeId;
	CleanNodeId.TrimStartAndEndInline();

	FString CleanInputKey = Target.InputKey;
	CleanInputKey.TrimStartAndEndInline();

	return CleanNodeId.IsEmpty() && CleanInputKey.IsEmpty();
}

bool UComfyUIImageGenerationWidgetController::IsWorkflowInputTargetComplete(const FComfyUIImageWorkflowInputTarget& Target) const
{
	FString CleanNodeId = Target.NodeId;
	CleanNodeId.TrimStartAndEndInline();

	FString CleanInputKey = Target.InputKey;
	CleanInputKey.TrimStartAndEndInline();

	return !CleanNodeId.IsEmpty() && !CleanInputKey.IsEmpty();
}

bool UComfyUIImageGenerationWidgetController::ApplyStringWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& Target, const FString& NewValue, const FString& Label)
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

bool UComfyUIImageGenerationWidgetController::ApplyNumberWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& Target, int32 NewValue, const FString& Label)
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

bool UComfyUIImageGenerationWidgetController::FindWorkflowInputsObject(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& Target, const FString& Label, TSharedPtr<FJsonObject>& OutInputsObject)
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

bool UComfyUIImageGenerationWidgetController::ConvertUiWorkflowToApiPrompt(const TSharedPtr<FJsonObject>& UiWorkflowObject, TSharedPtr<FJsonObject>& OutPromptObject) const
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

void UComfyUIImageGenerationWidgetController::AddWidgetValuesToApiInputs(const FString& NodeType, const TArray<TSharedPtr<FJsonValue>>& WidgetValues, const TSharedPtr<FJsonObject>& ApiInputsObject) const
{
	if (!ApiInputsObject.IsValid())
	{
		return;
	}

	const TArray<FString> InputNames = GetKnownWidgetInputNames(NodeType);
	if (InputNames.Num() == 0)
	{
		UE_LOG(LogComfyUIImageGenerationWidgetController, Verbose, TEXT("ComfyUI UI workflow 변환 중 widgets_values 매핑을 모르는 노드를 건너뜁니다: %s"), *NodeType);
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

TArray<FString> UComfyUIImageGenerationWidgetController::GetKnownWidgetInputNames(const FString& NodeType) const
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

bool UComfyUIImageGenerationWidgetController::ResolveWorkflowJsonFullPath(FString& OutFullPath, FString& OutTriedPaths) const
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

bool UComfyUIImageGenerationWidgetController::ResolveContentSaveFolderFullPath(FString& OutFullPath) const
{
	FString CleanFolder = ContentSaveFolder.Path;
	CleanFolder.TrimStartAndEndInline();
	FPaths::NormalizeFilename(CleanFolder);

	if (CleanFolder.IsEmpty() || CleanFolder == TEXT("/Game"))
	{
		OutFullPath = FPaths::ProjectContentDir();
		return true;
	}

	if (CleanFolder.StartsWith(TEXT("/Game/")))
	{
		CleanFolder.RightChopInline(6);
		CleanFolder.RemoveFromStart(TEXT("Content/"));
	}
	else if (CleanFolder.StartsWith(TEXT("Content/")))
	{
		CleanFolder.RightChopInline(8);
	}
	else
	{
		return false;
	}

	FPaths::CollapseRelativeDirectories(CleanFolder);
	if (CleanFolder.Contains(TEXT("..")))
	{
		return false;
	}

	OutFullPath = FPaths::Combine(FPaths::ProjectContentDir(), CleanFolder);
	FPaths::NormalizeFilename(OutFullPath);
	return true;
}

bool UComfyUIImageGenerationWidgetController::ResolveContentSaveFolderPackagePath(FString& OutPackagePath) const
{
	FString CleanFolder = ContentSaveFolder.Path;
	CleanFolder.TrimStartAndEndInline();
	FPaths::NormalizeFilename(CleanFolder);

	if (CleanFolder.IsEmpty())
	{
		CleanFolder = TEXT("/Game/GeneratedTextures");
	}

	if (CleanFolder == TEXT("/Game"))
	{
		OutPackagePath = CleanFolder;
		return true;
	}

	if (CleanFolder.StartsWith(TEXT("Content/")))
	{
		CleanFolder.RightChopInline(8);
		CleanFolder = FString::Printf(TEXT("/Game/%s"), *CleanFolder);
	}

	if (!CleanFolder.StartsWith(TEXT("/Game/")))
	{
		return false;
	}

	FPaths::CollapseRelativeDirectories(CleanFolder);
	if (CleanFolder.Contains(TEXT("..")))
	{
		return false;
	}

	CleanFolder.RemoveFromEnd(TEXT("/"));
	OutPackagePath = CleanFolder;
	return FPackageName::IsValidLongPackageName(OutPackagePath);
}

FString UComfyUIImageGenerationWidgetController::BuildTextureAssetNameBase(const FString& ImageFilePath) const
{
	FString Prefix = TextureAssetNamePrefix;
	Prefix.TrimStartAndEndInline();
	if (Prefix.IsEmpty())
	{
		Prefix = TEXT("T_ComfyUI");
	}

	FString SourceName = FPaths::GetBaseFilename(ImageFilePath);
	SourceName.TrimStartAndEndInline();
	if (SourceName.IsEmpty())
	{
		SourceName = TEXT("Image");
	}

	FString AssetName = FString::Printf(TEXT("%s_%s"), *Prefix, *SourceName);
	for (TCHAR& Character : AssetName)
	{
		if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
		{
			Character = TEXT('_');
		}
	}

	if (AssetName.IsEmpty())
	{
		AssetName = TEXT("T_ComfyUI_Image");
	}

	return AssetName;
}

void UComfyUIImageGenerationWidgetController::ApplyImportedTextureSettings(UTexture2D* Texture) const
{
	if (!Texture)
	{
		return;
	}

	Texture->SRGB = bImportedTextureSRGB;
	Texture->CompressionSettings = ImportedTextureCompressionSettings;
	Texture->MipGenSettings = ImportedTextureMipGenSettings;
#if WITH_EDITOR
	Texture->PostEditChange();
#endif
}

FString UComfyUIImageGenerationWidgetController::BuildEndpointUrl(const FString& ApiPath) const
{
	FString BaseUrl = ServerBaseUrl;
	BaseUrl.TrimStartAndEndInline();
	BaseUrl.RemoveFromEnd(TEXT("/"));

	if (BaseUrl.IsEmpty())
	{
		return FString();
	}

	FString CleanApiPath = ApiPath;
	CleanApiPath.TrimStartAndEndInline();
	CleanApiPath.RemoveFromStart(TEXT("/"));

	return FString::Printf(TEXT("%s/%s"), *BaseUrl, *CleanApiPath);
}

void UComfyUIImageGenerationWidgetController::HandlePromptResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	LastHttpStatusCode = Response.IsValid() ? Response->GetResponseCode() : 0;
	LastResponseBody = Response.IsValid() ? Response->GetContentAsString() : FString();

	const bool bHttpSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(LastHttpStatusCode);
	if (!bHttpSuccess)
	{
		SetFailureResult(FString::Printf(TEXT("ComfyUI /prompt 요청 실패. HTTP %d"), LastHttpStatusCode));
		return;
	}

	TSharedPtr<FJsonObject> ResponseObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(LastResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, ResponseObject) || !ResponseObject.IsValid() || !ResponseObject->TryGetStringField(TEXT("prompt_id"), LastPromptId) || LastPromptId.IsEmpty())
	{
		SetFailureResult(TEXT("ComfyUI /prompt 응답에서 prompt_id를 찾을 수 없습니다."));
		return;
	}

	LastStatusMessage = FString::Printf(TEXT("ComfyUI 생성 대기 중. prompt_id: %s"), *LastPromptId);
	RequestHistory();
}

void UComfyUIImageGenerationWidgetController::RequestHistory()
{
	if (LastPromptId.IsEmpty())
	{
		SetFailureResult(TEXT("ComfyUI history 조회에 사용할 prompt_id가 없습니다."));
		return;
	}

	++HistoryPollAttempts;
	const FString EndpointUrl = BuildEndpointUrl(FString::Printf(TEXT("history/%s"), *LastPromptId));
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(EndpointUrl);
	Request->SetVerb(TEXT("GET"));
	Request->OnProcessRequestComplete().BindUObject(this, &UComfyUIImageGenerationWidgetController::HandleHistoryResponse);

	LastStatusMessage = FString::Printf(TEXT("ComfyUI 생성 결과 확인 중: %d/%d"), HistoryPollAttempts, MaxHistoryPollAttempts);

	if (!Request->ProcessRequest())
	{
		SetFailureResult(TEXT("ComfyUI history 조회 요청 시작에 실패했습니다."));
	}
}

void UComfyUIImageGenerationWidgetController::HandleHistoryResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	LastHttpStatusCode = Response.IsValid() ? Response->GetResponseCode() : 0;
	LastResponseBody = Response.IsValid() ? Response->GetContentAsString() : FString();

	const bool bHttpSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(LastHttpStatusCode);
	if (!bHttpSuccess)
	{
		SetFailureResult(FString::Printf(TEXT("ComfyUI history 조회 실패. HTTP %d"), LastHttpStatusCode));
		return;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(LastResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		SetFailureResult(TEXT("ComfyUI history 응답 JSON 파싱에 실패했습니다."));
		return;
	}

	const TSharedPtr<FJsonObject>* PromptHistoryObjectPtr = nullptr;
	if (!RootObject->TryGetObjectField(LastPromptId, PromptHistoryObjectPtr) || !PromptHistoryObjectPtr || !PromptHistoryObjectPtr->IsValid())
	{
		ScheduleNextHistoryPoll();
		return;
	}

	const TSharedPtr<FJsonObject>* OutputsObjectPtr = nullptr;
	if (!(*PromptHistoryObjectPtr)->TryGetObjectField(TEXT("outputs"), OutputsObjectPtr) || !OutputsObjectPtr || !OutputsObjectPtr->IsValid())
	{
		ScheduleNextHistoryPoll();
		return;
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& OutputPair : (*OutputsObjectPtr)->Values)
	{
		const TSharedPtr<FJsonObject>* OutputNodeObjectPtr = nullptr;
		if (!OutputPair.Value.IsValid() || !OutputPair.Value->TryGetObject(OutputNodeObjectPtr) || !OutputNodeObjectPtr || !OutputNodeObjectPtr->IsValid())
		{
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* ImageValues = nullptr;
		if (!(*OutputNodeObjectPtr)->TryGetArrayField(TEXT("images"), ImageValues) || !ImageValues)
		{
			continue;
		}

		for (const TSharedPtr<FJsonValue>& ImageValue : *ImageValues)
		{
			const TSharedPtr<FJsonObject>* ImageObjectPtr = nullptr;
			if (!ImageValue.IsValid() || !ImageValue->TryGetObject(ImageObjectPtr) || !ImageObjectPtr || !ImageObjectPtr->IsValid())
			{
				continue;
			}

			FString FileName;
			if (!(*ImageObjectPtr)->TryGetStringField(TEXT("filename"), FileName) || FileName.IsEmpty())
			{
				continue;
			}

			FString Subfolder;
			(*ImageObjectPtr)->TryGetStringField(TEXT("subfolder"), Subfolder);

			FString ImageType = TEXT("output");
			(*ImageObjectPtr)->TryGetStringField(TEXT("type"), ImageType);

			RequestImageDownload(FileName, Subfolder, ImageType);
			return;
		}
	}

	ScheduleNextHistoryPoll();
}

void UComfyUIImageGenerationWidgetController::RequestImageDownload(const FString& FileName, const FString& Subfolder, const FString& ImageType)
{
	FString EndpointUrl = BuildEndpointUrl(TEXT("view"));
	EndpointUrl += FString::Printf(
		TEXT("?filename=%s&subfolder=%s&type=%s"),
		*FGenericPlatformHttp::UrlEncode(FileName),
		*FGenericPlatformHttp::UrlEncode(Subfolder),
		*FGenericPlatformHttp::UrlEncode(ImageType));

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(EndpointUrl);
	Request->SetVerb(TEXT("GET"));
	Request->OnProcessRequestComplete().BindUObject(this, &UComfyUIImageGenerationWidgetController::HandleImageDownloadResponse, FileName);

	LastStatusMessage = FString::Printf(TEXT("ComfyUI 생성 이미지 다운로드 중: %s"), *FileName);

	if (!Request->ProcessRequest())
	{
		SetFailureResult(TEXT("ComfyUI 생성 이미지 다운로드 요청 시작에 실패했습니다."));
	}
}

void UComfyUIImageGenerationWidgetController::HandleImageDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString FileName)
{
	LastHttpStatusCode = Response.IsValid() ? Response->GetResponseCode() : 0;
	LastResponseBody.Reset();

	const bool bHttpSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(LastHttpStatusCode);
	if (!bHttpSuccess)
	{
		SetFailureResult(FString::Printf(TEXT("ComfyUI 생성 이미지 다운로드 실패. HTTP %d"), LastHttpStatusCode));
		return;
	}

	const FString CleanFileName = FPaths::GetCleanFilename(FileName);
	const FString TempFolderFullPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ComfyUITextureImport"));
	IFileManager::Get().MakeDirectory(*TempFolderFullPath, true);

	LastSavedImagePath = FPaths::Combine(TempFolderFullPath, CleanFileName);
	FPaths::NormalizeFilename(LastSavedImagePath);

	if (!FFileHelper::SaveArrayToFile(Response->GetContent(), *LastSavedImagePath))
	{
		SetFailureResult(FString::Printf(TEXT("Texture import용 임시 이미지 파일 저장에 실패했습니다: %s"), *LastSavedImagePath));
		return;
	}

	const FString TempImagePath = LastSavedImagePath;
	const bool bImported = ImportImageFileAsTexture(TempImagePath);
	IFileManager::Get().Delete(*TempImagePath, false, true);

	if (!bImported)
	{
		LastSavedImagePath.Reset();
		return;
	}

	LastSavedImagePath.Reset();
}

void UComfyUIImageGenerationWidgetController::ScheduleNextHistoryPoll()
{
	if (HistoryPollAttempts >= MaxHistoryPollAttempts)
	{
		SetFailureResult(TEXT("ComfyUI 생성 결과 대기 시간이 초과되었습니다."));
		return;
	}

	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
	{
		RequestHistory();
		return false;
	}), HistoryPollIntervalSeconds);
}

void UComfyUIImageGenerationWidgetController::SetFailureResult(const FString& FailureMessage)
{
	bRequestInProgress = false;
	bLastRequestSucceeded = false;
	GenerationState = EComfyUIImageGenerationState::Failed;
	LastStatusMessage = FailureMessage;

	if (IsRooted())
	{
		RemoveFromRoot();
	}

	UE_LOG(LogComfyUIImageGenerationWidgetController, Warning, TEXT("%s"), *LastStatusMessage);
}

void UComfyUIImageGenerationWidgetController::SetSuccessResult(const FString& SuccessMessage)
{
	bRequestInProgress = false;
	bLastRequestSucceeded = true;
	GenerationState = EComfyUIImageGenerationState::Succeeded;
	LastStatusMessage = SuccessMessage;

	if (IsRooted())
	{
		RemoveFromRoot();
	}

	UE_LOG(LogComfyUIImageGenerationWidgetController, Display, TEXT("%s"), *LastStatusMessage);
}
