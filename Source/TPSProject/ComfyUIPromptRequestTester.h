// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HttpFwd.h"
#include "UObject/SoftObjectPath.h"

class FJsonObject;
class FJsonValue;

#include "ComfyUIPromptRequestTester.generated.h"

USTRUCT(BlueprintType)
struct FComfyUIWorkflowInputTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides")
	FString NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides")
	FString InputKey;
};

UCLASS()
class TPSPROJECT_API AComfyUIPromptRequestTester : public AActor
{
	GENERATED_BODY()

public:
	// ComfyUI 요청 검증용 기본 설정을 만든다.
	AComfyUIPromptRequestTester();

	// 에디터 Details 패널에서 workflow JSON을 읽고 로컬 ComfyUI /prompt 요청을 보낸다.
	UFUNCTION(CallInEditor, Category = "ComfyUI")
	void SendPromptRequest();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Request", meta = (AllowPrivateAccess = "true"))
	FString ServerBaseUrl = TEXT("http://127.0.0.1:8188");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Request", meta = (AllowPrivateAccess = "true"))
	FFilePath WorkflowJsonFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides", meta = (AllowPrivateAccess = "true", MultiLine = "true"))
	FString PositivePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides", meta = (AllowPrivateAccess = "true"))
	FComfyUIWorkflowInputTarget PositivePromptTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 ImageWidth = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 ImageHeight = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides", meta = (AllowPrivateAccess = "true"))
	FComfyUIWorkflowInputTarget ImageWidthTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Overrides", meta = (AllowPrivateAccess = "true"))
	FComfyUIWorkflowInputTarget ImageHeightTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result", meta = (AllowPrivateAccess = "true"))
	bool bRequestInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result", meta = (AllowPrivateAccess = "true"))
	bool bLastRequestSucceeded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result", meta = (AllowPrivateAccess = "true"))
	int32 LastHttpStatusCode = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result", meta = (AllowPrivateAccess = "true"))
	FString LastStatusMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Result", meta = (AllowPrivateAccess = "true", MultiLine = "true"))
	FString LastResponseBody;

	// 파일 경로와 JSON 형식을 확인한 뒤 ComfyUI /prompt 요청 본문을 만든다.
	bool BuildPromptRequestBody(FString& OutRequestBody);

	// 요청 직전에 에디터 입력값으로 workflow의 지정 input 값을 치환한다.
	bool ApplyWorkflowInputOverrides(const TSharedPtr<FJsonObject>& RequestObject);

	// 긍정 프롬프트 치환 대상을 설정값 또는 workflow 연결 구조에서 찾는다.
	bool ResolvePositivePromptTarget(const TSharedPtr<FJsonObject>& PromptObject, FComfyUIWorkflowInputTarget& OutTarget);

	// 특정 class_type과 input key를 가진 workflow 치환 대상을 설정값 또는 자동 탐색으로 찾는다.
	bool ResolveClassInputTarget(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& ConfiguredTarget, const FString& Label, const FString& ClassType, const FString& InputKey, FComfyUIWorkflowInputTarget& OutTarget);

	// 사용자가 node id와 input key를 모두 비웠는지 확인한다.
	bool IsWorkflowInputTargetEmpty(const FComfyUIWorkflowInputTarget& Target) const;

	// 사용자가 node id와 input key를 모두 입력했는지 확인한다.
	bool IsWorkflowInputTargetComplete(const FComfyUIWorkflowInputTarget& Target) const;

	// 문자열 입력값을 지정한 workflow node/input에 반영한다.
	bool ApplyStringWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& Target, const FString& NewValue, const FString& Label);

	// 숫자 입력값을 지정한 workflow node/input에 반영한다.
	bool ApplyNumberWorkflowInputOverride(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& Target, int32 NewValue, const FString& Label);

	// 지정한 workflow node/input이 존재하는지 확인하고 inputs object를 가져온다.
	bool FindWorkflowInputsObject(const TSharedPtr<FJsonObject>& PromptObject, const FComfyUIWorkflowInputTarget& Target, const FString& Label, TSharedPtr<FJsonObject>& OutInputsObject);

	// ComfyUI 화면 저장 workflow를 /prompt API에서 받는 prompt Object로 변환한다.
	bool ConvertUiWorkflowToApiPrompt(const TSharedPtr<FJsonObject>& UiWorkflowObject, TSharedPtr<FJsonObject>& OutPromptObject) const;

	// UI workflow의 widgets_values 배열을 API prompt input 이름으로 옮긴다.
	void AddWidgetValuesToApiInputs(const FString& NodeType, const TArray<TSharedPtr<FJsonValue>>& WidgetValues, const TSharedPtr<FJsonObject>& ApiInputsObject) const;

	// 자주 쓰는 ComfyUI 기본 노드의 widgets_values 순서를 API input 이름으로 변환한다.
	TArray<FString> GetKnownWidgetInputNames(const FString& NodeType) const;

	// 에디터에 입력된 workflow JSON 경로를 실제 읽을 수 있는 전체 경로로 변환한다.
	bool ResolveWorkflowJsonFullPath(FString& OutFullPath, FString& OutTriedPaths) const;

	// 서버 주소 끝의 슬래시를 정리하고 /prompt 엔드포인트 URL을 만든다.
	FString BuildPromptEndpointUrl() const;

	// 비동기 HTTP 요청 완료 결과를 로그와 Details 패널에서 볼 수 있는 값으로 저장한다.
	void HandlePromptResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// 검증 실패 사유를 공통 형식으로 기록한다.
	void SetFailureResult(const FString& FailureMessage);
};
