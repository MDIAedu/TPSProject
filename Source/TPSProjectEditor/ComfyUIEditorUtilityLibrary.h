// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"
#include "ComfyUIEditorUtilityLibrary.generated.h"

DECLARE_DYNAMIC_DELEGATE_SixParams(
	FOnComfyUIEditorRequestCompleted,
	bool, bSucceeded,
	int32, HttpStatusCode,
	const FString&, ResponseMessage,
	int32, AppliedPositivePromptNodeId,
	int32, AppliedImageSizeNodeId,
	const FString&, OverrideMessage);

UCLASS()
class TPSPROJECTEDITOR_API UComfyUIEditorUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Windows 파일 선택 창에서 ComfyUI workflow JSON 파일을 선택한다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI|Editor Utility", meta = (DisplayName = "Open ComfyUI Workflow File Dialog"))
	static bool OpenComfyUIWorkflowFileDialog(
		const FString& CurrentFilePath,
		FString& SelectedFilePath);

	// Editor Utility Widget에서 입력한 값으로 ComfyUI workflow 이미지 생성 요청을 보낸다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI|Editor Utility", meta = (DisplayName = "Send ComfyUI Workflow Prompt", AdvancedDisplay = "ServerBaseUrl,RequestTimeoutSeconds", CPP_Default_ServerBaseUrl = "http://127.0.0.1:8188", CPP_Default_RequestTimeoutSeconds = "30.0"))
	static void SendComfyUIWorkflowPrompt(
		const FFilePath& WorkflowJsonFile,
		const FString& PositivePrompt,
		int32 ImageWidth,
		int32 ImageHeight,
		FOnComfyUIEditorRequestCompleted Completed,
		const FString& ServerBaseUrl,
		float RequestTimeoutSeconds);

	// Editor Utility Widget에서 ComfyUI 서버 연결 여부를 확인한다.
	UFUNCTION(BlueprintCallable, Category = "ComfyUI|Editor Utility", meta = (DisplayName = "Check ComfyUI Server Connection", AdvancedDisplay = "ServerBaseUrl,RequestTimeoutSeconds", CPP_Default_ServerBaseUrl = "http://127.0.0.1:8188", CPP_Default_RequestTimeoutSeconds = "30.0"))
	static void CheckComfyUIServerConnection(
		FOnComfyUIEditorRequestCompleted Completed,
		const FString& ServerBaseUrl,
		float RequestTimeoutSeconds);
};
