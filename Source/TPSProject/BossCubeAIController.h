// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "BossCubeAIController.generated.h"

UCLASS()
class TPSPROJECT_API ABossCubeAIController : public AAIController
{
	GENERATED_BODY()

public:
	// 보스가 플레이어를 길찾기 대상으로 추적할 때 사용할 기본 값을 만든다.
	ABossCubeAIController();

protected:
	// 보스 Pawn을 조종하기 시작하면 플레이어 추적 갱신을 시작한다.
	virtual void OnPossess(APawn* InPawn) override;

	// 보스 Pawn 조종이 끝나면 추적 갱신을 멈춘다.
	virtual void OnUnPossess() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Chase")
	float ChaseRefreshInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Chase")
	float AcceptanceRadius = 120.0f;

	FTimerHandle ChaseTimerHandle;

	// 현재 플레이어 Pawn을 찾아 길찾기 이동 대상으로 다시 지정한다.
	void UpdateChaseTarget();
};
