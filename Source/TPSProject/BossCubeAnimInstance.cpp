// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossCubeAnimInstance.h"

#include "GameFramework/Pawn.h"

void UBossCubeAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedPawn = TryGetPawnOwner();
	UpdateBossStateValues();
}

void UBossCubeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedPawn)
	{
		CachedPawn = TryGetPawnOwner();
	}

	UpdateBossStateValues();
}

void UBossCubeAnimInstance::UpdateBossStateValues()
{
	// 삼항연산자  ?  :
	const ABossCubeAIController* BossAIController = CachedPawn ? Cast<ABossCubeAIController>(CachedPawn->GetController()) : nullptr;
	CurrentBossState = BossAIController ? BossAIController->GetCurrentState() : EBossCubeAIState::Chase;
	JumpSlamAnimState = BossAIController ? BossAIController->GetJumpSlamAnimState() : EBossJumpSlamAnimState::None;

	// 비교연산자 ( < , > , == , <=,  >= , !=)
	bIsChasing = CurrentBossState == EBossCubeAIState::Chase;
	bIsMeleeAttacking = CurrentBossState == EBossCubeAIState::MeleeAttack;
	bIsJumpSlamAttacking = CurrentBossState == EBossCubeAIState::JumpSlamAttack;
	bIsJumpSlamStarting = JumpSlamAnimState == EBossJumpSlamAnimState::Start;
	bIsJumpSlamInAir = JumpSlamAnimState == EBossJumpSlamAnimState::InAir;
	bIsJumpSlamLanding = JumpSlamAnimState == EBossJumpSlamAnimState::Land;
}
