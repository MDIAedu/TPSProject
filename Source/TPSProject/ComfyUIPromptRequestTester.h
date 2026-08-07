// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HttpFwd.h"
#include "UObject/SoftObjectPath.h"
#include "ComfyUIPromptRequestTester.generated.h"

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
