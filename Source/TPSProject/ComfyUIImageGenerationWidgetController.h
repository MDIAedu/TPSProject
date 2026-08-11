// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "HttpFwd.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPath.h"

#include "ComfyUIImageGenerationWidgetController.generated.h"

class FJsonObject;
class FJsonValue;

UENUM(BlueprintType)
enum class EComfyUIImageGenerationState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Generating UMETA(DisplayName = "Generating"),
	Succeeded UMETA(DisplayName = "Succeeded"),
	Failed UMETA(DisplayName = "Failed")
};

USTRUCT(BlueprintType)
struct FComfyUIImageWorkflowInputTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides")
	FString NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides")
	FString InputKey;
};

UCLASS(BlueprintType, Blueprintable)
class TPSPROJECT_API UComfyUIImageGenerationWidgetController : public UObject
{
	GENERATED_BODY()

public:
	// Editor Utility Widget에서 사용할 기본 생성 설정을 준비한다.
	UComfyUIImageGenerationWidgetController();

	// 현재 입력값으로 ComfyUI 이미지 생성 요청을 시작한다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI")
	void CreateImage();

	// 현재 생성 요청 상태를 초기 대기 상태로 되돌린다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI")
	void ResetGenerationState();

	// Editor Utility Widget에서 workflow JSON 파일 선택 창을 열고 선택한 파일 경로를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI|Editor")
	bool SelectWorkflowJsonFile(FString& OutFilePath);

	// Editor Utility Widget에서 Content 하위 저장 폴더 선택 창을 열고 /Game 경로를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI|Editor")
	bool SelectContentSaveFolder(FString& OutContentFolderPath);

	// 저장된 이미지 파일을 지정한 Content 폴더에 Texture2D asset으로 import한다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture Import")
	bool ImportLastSavedImageAsTexture();

	// 지정한 이미지 파일을 지정한 Content 폴더에 Texture2D asset으로 import한다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI|Texture Import")
	bool ImportImageFileAsTexture(const FString& ImageFilePath);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Request")
	FString ServerBaseUrl = TEXT("http://127.0.0.1:8188");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Request")
	FFilePath WorkflowJsonFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Request", meta = (ContentDir))
	FDirectoryPath ContentSaveFolder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides", meta = (MultiLine = "true"))
	FString PositivePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides")
	FComfyUIImageWorkflowInputTarget PositivePromptTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides", meta = (ClampMin = "1"))
	int32 ImageWidth = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides", meta = (ClampMin = "1"))
	int32 ImageHeight = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides")
	FComfyUIImageWorkflowInputTarget ImageWidthTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Overrides")
	FComfyUIImageWorkflowInputTarget ImageHeightTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Texture Import")
	FString TextureAssetNamePrefix = TEXT("T_ComfyUI");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Texture Import")
	bool bImportedTextureSRGB = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Texture Import")
	TEnumAsByte<TextureCompressionSettings> ImportedTextureCompressionSettings = TC_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ComfyUI|Texture Import")
	TEnumAsByte<TextureMipGenSettings> ImportedTextureMipGenSettings = TMGS_FromTextureGroup;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	EComfyUIImageGenerationState GenerationState = EComfyUIImageGenerationState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	bool bRequestInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	bool bLastRequestSucceeded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	int32 LastHttpStatusCode = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	FString LastPromptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	FString LastSavedImagePath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	FString LastImportedTextureAssetPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result")
	FString LastStatusMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result", meta = (MultiLine = "true"))
	FString LastResponseBody;

private:
	// workflow JSON을 읽고 위젯 입력값을 반영한 /prompt 요청 본문을 만든다.
	bool BuildPromptRequestBody(FString& OutRequestBody);

	// workflow prompt Object에 프롬프트와 이미지 크기 입력값을 반영한다.
	bool ApplyWorkflowInputOverrides(const TSharedPtr<FJsonObject>& RequestObject);

	// 긍정 프롬프트 입력 노드를 설정값이나 workflow 연결 구조에서 찾는다.
	bool ResolvePositivePromptTarget(const TSharedPtr<FJsonObject>& PromptObject, FComfyUIImageWorkflowInputTarget& OutTarget);

	// class_type과 input key가 일치하는 workflow 입력 노드를 찾는다.
	bool ResolveClassInputTarget(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& ConfiguredTarget, const FString& Label, const FString& ClassType, const FString& InputKey, FComfyUIImageWorkflowInputTarget& OutTarget);

	// node id와 input key가 모두 비어 있는지 확인한다.
	bool IsWorkflowInputTargetEmpty(const FComfyUIImageWorkflowInputTarget& Target) const;

	// node id와 input key가 모두 입력되었는지 확인한다.
	bool IsWorkflowInputTargetComplete(const FComfyUIImageWorkflowInputTarget& Target) const;

	// 문자열 입력값을 workflow node input에 반영한다.
	bool ApplyStringWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& Target, const FString& NewValue, const FString& Label);

	// 숫자 입력값을 workflow node input에 반영한다.
	bool ApplyNumberWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& Target, int32 NewValue, const FString& Label);

	// 지정한 workflow node의 inputs Object를 가져온다.
	bool FindWorkflowInputsObject(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIImageWorkflowInputTarget& Target, const FString& Label, TSharedPtr<FJsonObject>& OutInputsObject);

	// ComfyUI 화면 저장 workflow를 /prompt API prompt Object로 변환한다.
	bool ConvertUiWorkflowToApiPrompt(const TSharedPtr<FJsonObject>& UiWorkflowObject, TSharedPtr<FJsonObject>& OutPromptObject) const;

	// UI workflow의 widgets_values 배열을 API input 이름으로 옮긴다.
	void AddWidgetValuesToApiInputs(const FString& NodeType, const TArray<TSharedPtr<FJsonValue>>& WidgetValues, const TSharedPtr<FJsonObject>& ApiInputsObject) const;

	// 자주 쓰는 ComfyUI 기본 노드의 widgets_values 순서를 반환한다.
	TArray<FString> GetKnownWidgetInputNames(const FString& NodeType) const;

	// 입력된 workflow JSON 경로를 실제 파일 경로로 변환한다.
	bool ResolveWorkflowJsonFullPath(FString& OutFullPath, FString& OutTriedPaths) const;

	// Content 저장 폴더를 실제 저장 경로로 변환한다.
	bool ResolveContentSaveFolderFullPath(FString& OutFullPath) const;

	// Content 저장 폴더를 Texture import 대상 /Game 경로로 정리한다.
	bool ResolveContentSaveFolderPackagePath(FString& OutPackagePath) const;

	// Texture asset 이름 후보를 Content Browser에서 사용할 수 있는 이름으로 정리한다.
	FString BuildTextureAssetNameBase(const FString& ImageFilePath) const;

	// import된 Texture2D에 기본 Texture 설정을 적용한다.
	void ApplyImportedTextureSettings(class UTexture2D* Texture) const;

	// 서버 주소와 API 경로를 결합한다.
	FString BuildEndpointUrl(const FString& ApiPath) const;

	// /prompt 응답을 처리하고 history 조회를 시작한다.
	void HandlePromptResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// ComfyUI history에서 생성 결과 이미지 정보를 조회한다.
	void RequestHistory();

	// history 응답에서 첫 번째 생성 이미지 정보를 찾아 다운로드를 시작한다.
	void HandleHistoryResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// 생성 이미지 파일을 ComfyUI /view에서 다운로드한다.
	void RequestImageDownload(const FString& FileName, const FString& Subfolder, const FString& ImageType);

	// 다운로드한 이미지 바이트를 Content 하위 폴더에 저장한다.
	void HandleImageDownloadResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString FileName);

	// 다음 history 조회를 지연 실행한다.
	void ScheduleNextHistoryPoll();

	// 실패 상태와 메시지를 저장한다.
	void SetFailureResult(const FString& FailureMessage);

	// 성공 상태와 메시지를 저장한다.
	void SetSuccessResult(const FString& SuccessMessage);

	int32 HistoryPollAttempts = 0;
	int32 MaxHistoryPollAttempts = 30;
	float HistoryPollIntervalSeconds = 1.0f;
};
