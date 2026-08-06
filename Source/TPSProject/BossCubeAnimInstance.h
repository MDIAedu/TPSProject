// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BossCubeAIController.h"
#include "BossCubeAnimInstance.generated.h"

class APawn;

UCLASS(Blueprintable, BlueprintType)
class TPSPROJECT_API UBossCubeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// 애니메이션 인스턴스가 시작될 때 소유 Pawn과 보스 AI 상태를 초기화한다.
	virtual void NativeInitializeAnimation() override;

	// 매 프레임 보스 AI 상태를 애니메이션 Blueprint에서 읽을 값으로 갱신한다.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	EBossCubeAIState CurrentBossState = EBossCubeAIState::Chase;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsChasing = true;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsMeleeAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsJumpSlamAttacking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	EBossJumpSlamAnimState JumpSlamAnimState = EBossJumpSlamAnimState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsJumpSlamStarting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsJumpSlamInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsJumpSlamLanding = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedPawn;

	// 소유 Pawn의 AIController에서 현재 보스 FSM 상태를 읽어 Blueprint 노출 값을 동기화한다.
	void UpdateBossStateValues();
};
