// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossCubeAIController.h"

#include "BossCubeCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABossCubeAIController::ABossCubeAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

EBossCubeAIState ABossCubeAIController::GetCurrentState() const
{
	return CurrentState;
}

void ABossCubeAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UpdateChaseTarget();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	//일정시간마다 특정 함수를 호출한다.	
	World->GetTimerManager().SetTimer(
		ChaseTimerHandle,
		this,
		&ABossCubeAIController::UpdateChaseTarget,
		ChaseRefreshInterval,
		true
	);
}

void ABossCubeAIController::OnUnPossess()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChaseTimerHandle);
		World->GetTimerManager().ClearTimer(MeleeAttackTimerHandle);
	}

	Super::OnUnPossess();
}

void ABossCubeAIController::UpdateChaseTarget()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (CurrentState == EBossCubeAIState::MeleeAttack)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("BossCubeAIController: Player pawn is not found."));
		return;
	}

	if (CanStartMeleeAttack(ControlledPawn, PlayerPawn))
	{
		StartMeleeAttack(PlayerPawn);
		return;
	}

	MoveToActor(PlayerPawn, AcceptanceRadius, true, true, true, nullptr, true);
}

bool ABossCubeAIController::CanStartMeleeAttack(const APawn* ControlledPawn, const APawn* PlayerPawn) const
{
	if (!ControlledPawn || !PlayerPawn)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(ControlledPawn);
	const float AttackCooldown = BossCharacter ? BossCharacter->GetMeleeAttackCooldown() : 1.5f;
	if (CurrentTime - LastMeleeAttackTime < AttackCooldown)
	{
		return false;
	}

	const float AttackRange = BossCharacter ? BossCharacter->GetMeleeAttackRange() : 180.0f;
	const float DistanceToPlayer = FVector::Dist2D(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
	return DistanceToPlayer <= AttackRange;
}

void ABossCubeAIController::StartMeleeAttack(APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	CurrentState = EBossCubeAIState::MeleeAttack;
	LastMeleeAttackTime = World->GetTimeSeconds();
	StopMovement();

	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(GetPawn());
	const float AttackDamage = BossCharacter ? BossCharacter->GetMeleeAttackDamage() : 20.0f;
	const float AttackRange = BossCharacter ? BossCharacter->GetMeleeAttackRange() : 180.0f;
	const float AttackCooldown = BossCharacter ? BossCharacter->GetMeleeAttackCooldown() : 1.5f;
	const float AttackDuration = BossCharacter ? BossCharacter->GetMeleeAttackDuration() : 0.5f;

	UGameplayStatics::ApplyDamage(PlayerPawn, AttackDamage, this, GetPawn(), nullptr);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("BossCubeAIController: Melee attack hit %s. Damage=%.1f. Range=%.1f. Cooldown=%.1f."),
		*GetNameSafe(PlayerPawn),
		AttackDamage,
		AttackRange,
		AttackCooldown
	);

	World->GetTimerManager().SetTimer(
		MeleeAttackTimerHandle,
		this,
		&ABossCubeAIController::FinishMeleeAttack,
		AttackDuration,
		false
	);
}

void ABossCubeAIController::FinishMeleeAttack()
{
	CurrentState = EBossCubeAIState::Chase;
	UE_LOG(LogTemp, Log, TEXT("BossCubeAIController: Melee attack finished. Return to chase."));
	UpdateChaseTarget();
}
