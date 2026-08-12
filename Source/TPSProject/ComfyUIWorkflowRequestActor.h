// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "UObject/NoExportTypes.h"
#include "ComfyUIWorkflowRequestActor.generated.h"

UCLASS()
class TPSPROJECT_API AComfyUIWorkflowRequestActor : public AActor
{
	GENERATED_BODY()

public:
	// ComfyUI workflow 요청 검증에 필요한 기본 설정값을 만든다.
	AComfyUIWorkflowRequestActor();

	// Unreal Editor Details 패널에서 workflow JSON 파일을 읽고 ComfyUI /prompt 요청을 보낸다.
	UFUNCTION(CallInEditor, Category = "ComfyUI")
	void SendWorkflowPrompt();

	// Unreal Editor Details 패널에서 ComfyUI 서버 기본 연결만 확인한다.
	UFUNCTION(CallInEditor, Category = "ComfyUI")
	void CheckServerConnection();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Request", meta = (AllowPrivateAccess = "true"))
	FString ServerBaseUrl = TEXT("http://.127.0.0.1:8188");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Request", meta = (AllowPrivateAccess = "true", FilePathFilter = "JSON files (*.json)|*.json"))
	FFilePath WorkflowJsonFile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Request", meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float RequestTimeoutSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Override", meta = (AllowPrivateAccess = "true", MultiLine = "true"))
	FString PositivePromptText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Override", meta = (AllowPrivateAccess = "true", MultiLine = "true"))
	FString NegativePromptText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Override", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	int32 ImageWidth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Override", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	int32 ImageHeight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Override|Advanced", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	int32 PositivePromptNodeId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Override|Advanced", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	int32 NegativePromptNodeId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ComfyUI|Override|Advanced", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	int32 ImageSizeNodeId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Last Response", meta = (AllowPrivateAccess = "true"))
	bool bLastRequestSucceeded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Last Response", meta = (AllowPrivateAccess = "true"))
	int32 LastHttpStatusCode = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Last Response", meta = (AllowPrivateAccess = "true", MultiLine = "true"))
	FString LastResponseMessage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Last Override", meta = (AllowPrivateAccess = "true"))
	int32 LastAppliedPositivePromptNodeId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Last Override", meta = (AllowPrivateAccess = "true"))
	int32 LastAppliedNegativePromptNodeId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Last Override", meta = (AllowPrivateAccess = "true"))
	int32 LastAppliedImageSizeNodeId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ComfyUI|Last Override", meta = (AllowPrivateAccess = "true", MultiLine = "true"))
	FString LastOverrideMessage;

	// 서버 기본 주소와 엔드포인트 경로를 합쳐 실제 요청 URL을 만든다.
	FString BuildComfyUrl(const FString& EndpointPath) const;

	// workflow JSON 파일을 읽고 요청 본문으로 사용할 문자열을 돌려준다.
	bool TryLoadWorkflowJson(FString& OutRequestBody);

	// ComfyUI /prompt API가 기대하는 {"prompt": ...} 요청 본문을 만든다.
	bool TryBuildPromptRequestBody(const FString& WorkflowJson, FString& OutRequestBody);

	// ComfyUI UI workflow 저장본을 /prompt API가 받을 수 있는 prompt 객체로 변환한다.
	bool TryConvertUiWorkflowToApiPrompt(const TSharedPtr<FJsonObject>& WorkflowObject, TSharedPtr<FJsonObject>& OutPromptObject);

	// UI workflow 노드의 widget 값을 API prompt 입력값에 추가한다.
	void ApplyKnownWidgetInputs(const TSharedPtr<FJsonObject>& UiNode, const TSharedRef<FJsonObject>& ApiNode, int32 ClipTextEncodeIndex, int32 EmptyLatentImageIndex);

	// 요청 완료 후 성공 여부와 응답 내용을 에디터에서 확인할 수 있는 값으로 저장한다.
	void HandlePromptResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// 서버 연결 확인 요청 완료 후 성공 여부와 응답 내용을 에디터에서 확인할 수 있는 값으로 저장한다.
	void HandleServerCheckResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// 실패 사유를 Last Response 값과 로그에 남긴다.
	void SetFailureMessage(const FString& FailureMessage);
};
