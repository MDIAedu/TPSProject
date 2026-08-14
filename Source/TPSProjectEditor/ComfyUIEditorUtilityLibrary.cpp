// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUIEditorUtilityLibrary.h"

#include "CodexFluxPromptService.h"
#include "ComfyUITextureImportService.h"
#include "ComfyUIWorkflowRequestService.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"

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

bool UComfyUIEditorUtilityLibrary::OpenComfyUIWorkflowFileDialog(
	const FString& CurrentFilePath,
	FString& SelectedFilePath)
{
	FString TrimmedCurrentFilePath = CurrentFilePath;
	TrimmedCurrentFilePath.TrimStartAndEndInline();
	SelectedFilePath = TrimmedCurrentFilePath;

	FString DefaultDirectory = FPaths::ProjectDir();
	if (!TrimmedCurrentFilePath.IsEmpty())
	{
		const FString CurrentDirectory = FPaths::GetPath(FPaths::ConvertRelativePathToFull(TrimmedCurrentFilePath));
		if (FPaths::DirectoryExists(CurrentDirectory))
		{
			DefaultDirectory = CurrentDirectory;
		}
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::TryGet();
	if (!DesktopPlatform)
	{
		UE_LOG(LogTemp, Warning, TEXT("ComfyUI workflow 파일 선택 창을 열 수 없습니다: DesktopPlatform을 사용할 수 없습니다."));
		return false;
	}

	TArray<FString> SelectedFiles;
	const bool bSelected = DesktopPlatform->OpenFileDialog(
		nullptr,
		TEXT("ComfyUI workflow JSON 파일 선택"),
		DefaultDirectory,
		TEXT(""),
		TEXT("JSON files (*.json)|*.json"),
		EFileDialogFlags::None,
		SelectedFiles);

	if (!bSelected || SelectedFiles.Num() != 1)
	{
		return false;
	}

	FString FullSelectedFilePath = FPaths::ConvertRelativePathToFull(SelectedFiles[0]);
	FPaths::NormalizeFilename(FullSelectedFilePath);
	if (!FPaths::FileExists(FullSelectedFilePath) || !FPaths::GetExtension(FullSelectedFilePath).Equals(TEXT("json"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Warning, TEXT("선택한 ComfyUI workflow 파일이 올바른 JSON 파일 경로가 아닙니다: %s"), *FullSelectedFilePath);
		return false;
	}

	SelectedFilePath = MoveTemp(FullSelectedFilePath);
	UE_LOG(LogTemp, Display, TEXT("ComfyUI workflow JSON 파일 선택 완료: %s"), *SelectedFilePath);
	return true;
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
	FString TrimmedPositivePrompt = PositivePrompt;
	TrimmedPositivePrompt.TrimStartAndEndInline();
	if (TrimmedPositivePrompt.IsEmpty())
	{
		FComfyUIWorkflowRequestResult Result;
		Result.ResponseMessage = TEXT("Codex로 변환할 긍정 프롬프트가 비어 있습니다.");
		ComfyUIEditorUtilityLibrary::ExecuteCompleted(Completed, Result);
		return;
	}

	if (ImageWidth <= 0 || ImageHeight <= 0)
	{
		FComfyUIWorkflowRequestResult Result;
		Result.ResponseMessage = TEXT("이미지 너비와 높이는 1 이상이어야 합니다.");
		ComfyUIEditorUtilityLibrary::ExecuteCompleted(Completed, Result);
		return;
	}

	FOnCodexFluxPromptComplete PromptCompletion;
	PromptCompletion.BindLambda(
		[WorkflowJsonFile, ImageWidth, ImageHeight, Completed, ServerBaseUrl, RequestTimeoutSeconds](const FCodexFluxPromptResult& PromptResult)
		{
			if (!PromptResult.bSucceeded)
			{
				FComfyUIWorkflowRequestResult FailureResult;
				FailureResult.ResponseMessage = PromptResult.ErrorMessage;
				UE_LOG(LogTemp, Warning, TEXT("Codex Flux.2 프롬프트 생성 실패: %s"), *FailureResult.ResponseMessage);
				ComfyUIEditorUtilityLibrary::ExecuteCompleted(Completed, FailureResult);
				return;
			}

			FComfyUIWorkflowRequestSettings Settings;
			Settings.ServerBaseUrl = ServerBaseUrl;
			Settings.WorkflowJsonFile = WorkflowJsonFile;
			Settings.RequestTimeoutSeconds = RequestTimeoutSeconds;
			Settings.PositivePromptText = PromptResult.GeneratedPrompt;
			Settings.ImageWidth = ImageWidth;
			Settings.ImageHeight = ImageHeight;

			FOnComfyUIWorkflowRequestComplete RequestCompletion;
			RequestCompletion.BindLambda(
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
			FComfyUIWorkflowRequestService::SendWorkflowPrompt(Settings, MoveTemp(RequestCompletion));
		});
	FCodexFluxPromptService::GeneratePrompt(TrimmedPositivePrompt, MoveTemp(PromptCompletion));
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
