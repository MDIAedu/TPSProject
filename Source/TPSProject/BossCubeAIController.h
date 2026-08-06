// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "BossCubeAIController.generated.h"

// 열거형 ( enum 형) : 상태를 나열
UENUM(BlueprintType)
enum class EBossCubeAIState : uint8
{
	Chase,
	MeleeAttack,
	JumpSlamAttack
};

UENUM(BlueprintType)
enum class EBossJumpSlamAnimState : uint8
{
	None,
	Start,
	InAir,
	Land
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

	// Blueprint나 AnimBP에서 점프 내려찍기의 세부 애니메이션 구간을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|State")
	EBossJumpSlamAnimState GetJumpSlamAnimState() const;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Boss|State", meta = (AllowPrivateAccess = "true"))
	EBossJumpSlamAnimState JumpSlamAnimState = EBossJumpSlamAnimState::None;

	FTimerHandle ChaseTimerHandle;
	FTimerHandle MeleeAttackTimerHandle;
	FTimerHandle JumpSlamStartTimerHandle;
	FTimerHandle JumpSlamMoveTimerHandle;
	FTimerHandle JumpSlamLandTimerHandle;
	FTimerHandle JumpSlamFinishTimerHandle;
	float LastMeleeAttackTime = -1000.0f;
	float LastJumpSlamAttackTime = -1000.0f;
	float JumpSlamElapsedTime = 0.0f;
	FVector JumpSlamStartLocation = FVector::ZeroVector;
	FVector LockedJumpSlamLandingLocation = FVector::ZeroVector;
	TEnumAsByte<ECollisionResponse> PreviousPawnCollisionResponse = ECR_Block;
	bool bJumpSlamCollisionOverrideActive = false;
	bool bJumpSlamOverlapLaunchApplied = false;
	bool bJumpSlamDamageApplied = false;

	// 현재 플레이어 Pawn을 찾아 길찾기 이동 대상으로 다시 지정한다.
	void UpdateChaseTarget();

	// 추적 중 근접 사거리와 쿨타임을 확인해 공격 상태로 전환할 수 있는지 판단한다.
	bool CanStartMeleeAttack(const APawn* ControlledPawn, const APawn* PlayerPawn) const;

	// 보스 이동을 멈추고 일반 공격 피해 판정을 실행한다.
	void StartMeleeAttack(APawn* PlayerPawn);

	// 일반 공격 상태를 끝내고 다시 추적 상태로 되돌린다.
	void FinishMeleeAttack();

	// 추적 중 거리, 확률, 쿨타임을 확인해 점프 내려찍기 상태로 전환할 수 있는지 판단한다.
	bool CanStartJumpSlamAttack(const APawn* ControlledPawn, const APawn* PlayerPawn) const;

	// 발동 시점의 플레이어 위치를 고정하고 점프 내려찍기 이동을 시작한다.
	void StartJumpSlamAttack(const APawn* PlayerPawn);

	// 점프 시작 애니메이션 구간이 끝나면 공중 이동 구간으로 전환한다.
	void BeginJumpSlamInAirMovement();

	// 점프 중 보스와 플레이어가 겹칠 수 있게 Pawn 채널 충돌 응답을 Overlap으로 바꾼다.
	void EnableJumpSlamOverlapCollision();

	// 점프 중 바꾼 Pawn 채널 충돌 응답을 원래 값으로 되돌린다.
	void RestoreJumpSlamCollision();

	// 점프 공격 피해를 받은 플레이어를 착지 지점 바깥쪽으로 밀어낸다.
	void LaunchPlayerFromJumpSlam(ACharacter* PlayerCharacter);

	// 점프 중 보스와 플레이어가 겹쳤다면 즉시 피해를 주고 밀어낸다.
	void ApplyJumpSlamDamageFromOverlap();

	// 고정된 착지 지점까지 보스 위치를 보간 이동한다.
	void UpdateJumpSlamMovement();

	// 착지 지점을 중심으로 원형 범위 피해를 판정한다.
	void ResolveJumpSlamAttack();

	// 점프 내려찍기 상태를 끝내고 다시 추적 상태로 되돌린다.
	void FinishJumpSlamAttack();
};
