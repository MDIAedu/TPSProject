// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/NoExportTypes.h"
#include "ComfyUIWorkflowRequestActor.generated.h"

struct FComfyUIWorkflowRequestResult;

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
	FString ServerBaseUrl = TEXT("http://127.0.0.1:8188");

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

	// 공통 요청 서비스의 완료 결과를 Details 패널 확인 값에 반영한다.
	void ApplyRequestResult(const FComfyUIWorkflowRequestResult& Result);
};
