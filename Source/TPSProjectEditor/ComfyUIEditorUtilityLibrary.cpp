// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUIEditorUtilityLibrary.h"

#include "ComfyUITextureImportService.h"
#include "ComfyUIWorkflowRequestService.h"

namespace ComfyUIEditorUtilityLibrary
{
	// 공통 서비스 결과를 EUW의 Blueprint 완료 이벤트 형식으로 전달한다.
	void ExecuteCompleted(const FOnComfyUIEditorRequestCompleted& Completed, const FComfyUIWorkflowRequestResult& Result)
	{
		Completed.ExecuteIfBound(
			Result.bSucceeded,
			Result.HttpStatusCode,
			Result.ResponseMessage,
			Result.AppliedPositivePromptNodeId,
			Result.AppliedImageSizeNodeId,
			Result.OverrideMessage);
	}
}

void UComfyUIEditorUtilityLibrary::SendComfyUIWorkflowPrompt(
	const FFilePath& WorkflowJsonFile,
	const FString& PositivePrompt,
	int32 ImageWidth,
	int32 ImageHeight,
	FOnComfyUIEditorRequestCompleted Completed,
	const FString& ServerBaseUrl,
	float RequestTimeoutSeconds)
{
	if (ImageWidth <= 0 || ImageHeight <= 0)
	{
		FComfyUIWorkflowRequestResult Result;
		Result.ResponseMessage = TEXT("이미지 너비와 높이는 1 이상이어야 합니다.");
		ComfyUIEditorUtilityLibrary::ExecuteCompleted(Completed, Result);
		return;
	}

	FComfyUIWorkflowRequestSettings Settings;
	Settings.ServerBaseUrl = ServerBaseUrl;
	Settings.WorkflowJsonFile = WorkflowJsonFile;
	Settings.RequestTimeoutSeconds = RequestTimeoutSeconds;
	Settings.PositivePromptText = PositivePrompt;
	Settings.ImageWidth = ImageWidth;
	Settings.ImageHeight = ImageHeight;

	FOnComfyUIWorkflowRequestComplete Completion;
	Completion.BindLambda(
		[Completed, ServerBaseUrl, RequestTimeoutSeconds](const FComfyUIWorkflowRequestResult& Result)
		{
			if (!Result.bSucceeded)
			{
				ComfyUIEditorUtilityLibrary::ExecuteCompleted(Completed, Result);
				return;
			}

			FOnComfyUITextureImportComplete ImportCompletion;
			ImportCompletion.BindLambda(
				[Completed, RequestResult = Result](const FComfyUITextureImportResult& ImportResult)
				{
					FComfyUIWorkflowRequestResult FinalResult = RequestResult;
					FinalResult.bSucceeded = ImportResult.bSucceeded;
					FinalResult.HttpStatusCode = ImportResult.HttpStatusCode;
					FinalResult.ResponseMessage = ImportResult.ResponseMessage;
					ComfyUIEditorUtilityLibrary::ExecuteCompleted(Completed, FinalResult);
				});
			FComfyUITextureImportService::ImportGeneratedTextures(
				ServerBaseUrl,
				Result.PromptId,
				RequestTimeoutSeconds,
				MoveTemp(ImportCompletion));
		});
	FComfyUIWorkflowRequestService::SendWorkflowPrompt(Settings, MoveTemp(Completion));
}

void UComfyUIEditorUtilityLibrary::CheckComfyUIServerConnection(
	FOnComfyUIEditorRequestCompleted Completed,
	const FString& ServerBaseUrl,
	float RequestTimeoutSeconds)
{
	FComfyUIWorkflowRequestSettings Settings;
	Settings.ServerBaseUrl = ServerBaseUrl;
	Settings.RequestTimeoutSeconds = RequestTimeoutSeconds;

	FOnComfyUIWorkflowRequestComplete Completion;
	Completion.BindLambda(
		[Completed](const FComfyUIWorkflowRequestResult& Result)
		{
			ComfyUIEditorUtilityLibrary::ExecuteCompleted(Completed, Result);
		});
	FComfyUIWorkflowRequestService::CheckServerConnection(Settings, MoveTemp(Completion));
}
