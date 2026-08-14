// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FComfyUITextureImportResult
{
	bool bSucceeded = false;
	int32 HttpStatusCode = 0;
	FString ResponseMessage;
	TArray<FString> ImportedTexturePaths;
};

DECLARE_DELEGATE_OneParam(FOnComfyUITextureImportComplete, const FComfyUITextureImportResult&);

class FComfyUITextureImportService
{
public:
	// ComfyUI 생성 완료를 기다린 뒤 결과 이미지를 Texture2D 자산으로 임포트한다.
	static void ImportGeneratedTextures(
		const FString& ServerBaseUrl,
		const FString& PromptId,
		float RequestTimeoutSeconds,
		FOnComfyUITextureImportComplete Completion);
};
