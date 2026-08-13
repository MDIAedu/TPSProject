// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUIWorkflowRequestActor.h"

#include "ComfyUIWorkflowRequestService.h"

AComfyUIWorkflowRequestActor::AComfyUIWorkflowRequestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AComfyUIWorkflowRequestActor::SendWorkflowPrompt()
{
	FComfyUIWorkflowRequestSettings Settings;
	Settings.ServerBaseUrl = ServerBaseUrl;
	Settings.WorkflowJsonFile = WorkflowJsonFile;
	Settings.RequestTimeoutSeconds = RequestTimeoutSeconds;
	Settings.PositivePromptText = PositivePromptText;
	Settings.NegativePromptText = NegativePromptText;
	Settings.ImageWidth = ImageWidth;
	Settings.ImageHeight = ImageHeight;
	Settings.PositivePromptNodeId = PositivePromptNodeId;
	Settings.NegativePromptNodeId = NegativePromptNodeId;
	Settings.ImageSizeNodeId = ImageSizeNodeId;

	bLastRequestSucceeded = false;
	LastHttpStatusCode = 0;
	LastResponseMessage = TEXT("ComfyUI workflow 요청을 준비하고 있습니다.");

	FOnComfyUIWorkflowRequestComplete Completion;
	Completion.BindUObject(this, &AComfyUIWorkflowRequestActor::ApplyRequestResult);
	FComfyUIWorkflowRequestService::SendWorkflowPrompt(Settings, MoveTemp(Completion));
}

void AComfyUIWorkflowRequestActor::CheckServerConnection()
{
	FComfyUIWorkflowRequestSettings Settings;
	Settings.ServerBaseUrl = ServerBaseUrl;
	Settings.RequestTimeoutSeconds = RequestTimeoutSeconds;

	bLastRequestSucceeded = false;
	LastHttpStatusCode = 0;
	LastResponseMessage = TEXT("ComfyUI 서버 연결 확인을 준비하고 있습니다.");

	FOnComfyUIWorkflowRequestComplete Completion;
	Completion.BindUObject(this, &AComfyUIWorkflowRequestActor::ApplyRequestResult);
	FComfyUIWorkflowRequestService::CheckServerConnection(Settings, MoveTemp(Completion));
}

void AComfyUIWorkflowRequestActor::ApplyRequestResult(const FComfyUIWorkflowRequestResult& Result)
{
	bLastRequestSucceeded = Result.bSucceeded;
	LastHttpStatusCode = Result.HttpStatusCode;
	LastResponseMessage = Result.ResponseMessage;
	LastAppliedPositivePromptNodeId = Result.AppliedPositivePromptNodeId;
	LastAppliedNegativePromptNodeId = Result.AppliedNegativePromptNodeId;
	LastAppliedImageSizeNodeId = Result.AppliedImageSizeNodeId;
	LastOverrideMessage = Result.OverrideMessage;
}
