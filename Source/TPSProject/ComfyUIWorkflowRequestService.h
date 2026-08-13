// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

struct FComfyUIWorkflowRequestSettings
{
	FString ServerBaseUrl = TEXT("http://127.0.0.1:8188");
	FFilePath WorkflowJsonFile;
	float RequestTimeoutSeconds = 30.0f;
	FString PositivePromptText;
	FString NegativePromptText;
	int32 ImageWidth = 0;
	int32 ImageHeight = 0;
	int32 PositivePromptNodeId = 0;
	int32 NegativePromptNodeId = 0;
	int32 ImageSizeNodeId = 0;
};

struct FComfyUIWorkflowRequestResult
{
	bool bSucceeded = false;
	int32 HttpStatusCode = 0;
	FString ResponseMessage;
	int32 AppliedPositivePromptNodeId = 0;
	int32 AppliedNegativePromptNodeId = 0;
	int32 AppliedImageSizeNodeId = 0;
	FString OverrideMessage;
};

DECLARE_DELEGATE_OneParam(FOnComfyUIWorkflowRequestComplete, const FComfyUIWorkflowRequestResult&);

class TPSPROJECT_API FComfyUIWorkflowRequestService
{
public:
	// workflow 파일에 override 값을 적용하고 ComfyUI /prompt 요청을 보낸다.
	static void SendWorkflowPrompt(const FComfyUIWorkflowRequestSettings& Settings, FOnComfyUIWorkflowRequestComplete Completion);

	// ComfyUI /system_stats 엔드포인트로 서버 연결을 확인한다.
	static void CheckServerConnection(const FComfyUIWorkflowRequestSettings& Settings, FOnComfyUIWorkflowRequestComplete Completion);
};
