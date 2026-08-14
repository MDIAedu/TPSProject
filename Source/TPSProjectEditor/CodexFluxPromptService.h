// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FCodexFluxPromptResult
{
	bool bSucceeded = false;
	FString GeneratedPrompt;
	FString ErrorMessage;
};

DECLARE_DELEGATE_OneParam(FOnCodexFluxPromptComplete, const FCodexFluxPromptResult&);

class FCodexFluxPromptService
{
public:
	// 사용자의 이미지 설명을 codex exec로 전달해 Flux.2용 영문 긍정 프롬프트를 생성한다.
	static void GeneratePrompt(const FString& SourcePrompt, FOnCodexFluxPromptComplete Completion);
};
