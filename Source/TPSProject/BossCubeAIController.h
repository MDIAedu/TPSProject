// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "BossCubeAIController.generated.h"

UENUM(BlueprintType)
enum class EBossCubeAIState : uint8
{
	Chase,
	MeleeAttack
};

UCLASS()
class TPSPROJECT_API ABossCubeAIController : public AAIController
{
	GENERATED_BODY()

public:
	// 보스가 플레이어를 길찾기 대상으로 추적할 때 사용할 기본 값을 만든다.
	ABossCubeAIController();

	// Blueprint나 디버그 표시에서 현재 보스 AI 상태를 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|State")
	EBossCubeAIState GetCurrentState() const;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Boss|State", meta = (AllowPrivateAccess = "true"))
	EBossCubeAIState CurrentState = EBossCubeAIState::Chase;

	FTimerHandle ChaseTimerHandle;
	FTimerHandle MeleeAttackTimerHandle;
	float LastMeleeAttackTime = -1000.0f;

	// 현재 플레이어 Pawn을 찾아 길찾기 이동 대상으로 다시 지정한다.
	void UpdateChaseTarget();

	// 추적 중 근접 사거리와 쿨타임을 확인해 공격 상태로 전환할 수 있는지 판단한다.
	bool CanStartMeleeAttack(const APawn* ControlledPawn, const APawn* PlayerPawn) const;

	// 보스 이동을 멈추고 일반 공격 피해 판정을 실행한다.
	void StartMeleeAttack(APawn* PlayerPawn);

	// 일반 공격 상태를 끝내고 다시 추적 상태로 되돌린다.
	void FinishMeleeAttack();
};
