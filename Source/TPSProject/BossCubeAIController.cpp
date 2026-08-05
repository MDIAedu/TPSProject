// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossCubeAIController.h"

#include "BossCubeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
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
	RestoreJumpSlamCollision();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChaseTimerHandle);
		World->GetTimerManager().ClearTimer(MeleeAttackTimerHandle);
		World->GetTimerManager().ClearTimer(JumpSlamMoveTimerHandle);
		World->GetTimerManager().ClearTimer(JumpSlamLandTimerHandle);
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

	if (CurrentState != EBossCubeAIState::Chase)
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

	if (CanStartJumpSlamAttack(ControlledPawn, PlayerPawn))
	{
		StartJumpSlamAttack(PlayerPawn);
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

bool ABossCubeAIController::CanStartJumpSlamAttack(const APawn* ControlledPawn, const APawn* PlayerPawn) const
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

	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(ControlledPawn);
	if (!BossCharacter)
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastJumpSlamAttackTime < BossCharacter->GetJumpSlamCooldown())
	{
		return false;
	}

	const float DistanceToPlayer = FVector::Dist2D(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation());
	if (DistanceToPlayer < BossCharacter->GetJumpSlamTriggerDistance())
	{
		return false;
	}

	return FMath::FRand() <= BossCharacter->GetJumpSlamChance();
}

void ABossCubeAIController::StartJumpSlamAttack(const APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return;
	}

	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();
	if (!World || !ControlledPawn)
	{
		return;
	}

	CurrentState = EBossCubeAIState::JumpSlamAttack;
	LastJumpSlamAttackTime = World->GetTimeSeconds();
	JumpSlamElapsedTime = 0.0f;
	JumpSlamStartLocation = ControlledPawn->GetActorLocation();
	LockedJumpSlamLandingLocation = PlayerPawn->GetActorLocation();
	LockedJumpSlamLandingLocation.Z = JumpSlamStartLocation.Z;
	bJumpSlamOverlapLaunchApplied = false;
	bJumpSlamDamageApplied = false;
	StopMovement();
	EnableJumpSlamOverlapCollision();

	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(ControlledPawn);
	const float JumpDuration = BossCharacter ? BossCharacter->GetJumpSlamDuration() : 0.8f;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("BossCubeAIController: Jump slam started. LockedLanding=%s. Duration=%.2f."),
		*LockedJumpSlamLandingLocation.ToCompactString(),
		JumpDuration
	);

	World->GetTimerManager().SetTimer(
		JumpSlamMoveTimerHandle,
		this,
		&ABossCubeAIController::UpdateJumpSlamMovement,
		0.02f,
		true
	);

	World->GetTimerManager().SetTimer(
		JumpSlamLandTimerHandle,
		this,
		&ABossCubeAIController::ResolveJumpSlamAttack,
		JumpDuration,
		false
	);
}

void ABossCubeAIController::EnableJumpSlamOverlapCollision()
{
	ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(GetPawn());
	if (!BossCharacter)
	{
		return;
	}

	UCapsuleComponent* BossCapsule = BossCharacter->GetCapsuleComponent();
	if (!BossCapsule)
	{
		return;
	}

	PreviousPawnCollisionResponse = BossCapsule->GetCollisionResponseToChannel(ECC_Pawn);
	BossCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	bJumpSlamCollisionOverrideActive = true;

	UE_LOG(LogTemp, Log, TEXT("BossCubeAIController: Jump slam collision changed to overlap for Pawn channel."));
}

void ABossCubeAIController::RestoreJumpSlamCollision()
{
	if (!bJumpSlamCollisionOverrideActive)
	{
		return;
	}

	ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(GetPawn());
	if (BossCharacter)
	{
		if (UCapsuleComponent* BossCapsule = BossCharacter->GetCapsuleComponent())
		{
			BossCapsule->SetCollisionResponseToChannel(ECC_Pawn, PreviousPawnCollisionResponse);
		}
	}

	bJumpSlamCollisionOverrideActive = false;
	UE_LOG(LogTemp, Log, TEXT("BossCubeAIController: Jump slam collision restored for Pawn channel."));
}

void ABossCubeAIController::LaunchPlayerFromJumpSlam(ACharacter* PlayerCharacter)
{
	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(GetPawn());
	if (!BossCharacter || !PlayerCharacter)
	{
		return;
	}

	if (bJumpSlamOverlapLaunchApplied)
	{
		return;
	}

	FVector LaunchDirection = PlayerCharacter->GetActorLocation() - LockedJumpSlamLandingLocation;
	LaunchDirection.Z = 0.0f;
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = PlayerCharacter->GetActorLocation() - BossCharacter->GetActorLocation();
		LaunchDirection.Z = 0.0f;
	}
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = BossCharacter->GetActorForwardVector();
		LaunchDirection.Z = 0.0f;
	}
	LaunchDirection.Normalize();

	const float LaunchStrength = BossCharacter->GetJumpSlamOverlapLaunchStrength();
	PlayerCharacter->LaunchCharacter(LaunchDirection * LaunchStrength, true, false);
	bJumpSlamOverlapLaunchApplied = true;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("BossCubeAIController: Jump slam launched player. Strength=%.1f."),
		LaunchStrength
	);
}

void ABossCubeAIController::ApplyJumpSlamDamageFromOverlap()
{
	if (bJumpSlamDamageApplied)
	{
		return;
	}

	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(GetPawn());
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!BossCharacter || !PlayerCharacter)
	{
		return;
	}

	const UCapsuleComponent* BossCapsule = BossCharacter->GetCapsuleComponent();
	const UCapsuleComponent* PlayerCapsule = PlayerCharacter->GetCapsuleComponent();
	if (!BossCapsule || !PlayerCapsule || !BossCapsule->IsOverlappingComponent(PlayerCapsule))
	{
		return;
	}

	const float Damage = BossCharacter->GetJumpSlamDamage();
	UGameplayStatics::ApplyDamage(PlayerCharacter, Damage, this, GetPawn(), nullptr);
	bJumpSlamDamageApplied = true;
	LaunchPlayerFromJumpSlam(PlayerCharacter);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("BossCubeAIController: Jump slam overlap hit player. Damage=%.1f."),
		Damage
	);
}

void ABossCubeAIController::UpdateJumpSlamMovement()
{
	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();
	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(ControlledPawn);
	if (!World || !ControlledPawn || !BossCharacter)
	{
		return;
	}

	const float JumpDuration = FMath::Max(BossCharacter->GetJumpSlamDuration(), 0.01f);
	JumpSlamElapsedTime += 0.02f;
	const float Alpha = FMath::Clamp(JumpSlamElapsedTime / JumpDuration, 0.0f, 1.0f);

	FVector NewLocation = FMath::Lerp(JumpSlamStartLocation, LockedJumpSlamLandingLocation, Alpha);
	const float ArcHeight = BossCharacter->GetJumpSlamArcHeight();
	NewLocation.Z += FMath::Sin(Alpha * UE_PI) * ArcHeight;
	ControlledPawn->SetActorLocation(NewLocation, false);
	ApplyJumpSlamDamageFromOverlap();
}

void ABossCubeAIController::ResolveJumpSlamAttack()
{
	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();
	const ABossCubeCharacter* BossCharacter = Cast<ABossCubeCharacter>(ControlledPawn);
	if (!World || !ControlledPawn || !BossCharacter)
	{
		FinishJumpSlamAttack();
		return;
	}

	World->GetTimerManager().ClearTimer(JumpSlamMoveTimerHandle);
	ControlledPawn->SetActorLocation(LockedJumpSlamLandingLocation, false);
	ApplyJumpSlamDamageFromOverlap();

	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	const float Radius = BossCharacter->GetJumpSlamDamageRadius();
	const float Damage = BossCharacter->GetJumpSlamDamage();
	const bool bPlayerInRange = PlayerCharacter && FVector::Dist2D(PlayerCharacter->GetActorLocation(), LockedJumpSlamLandingLocation) <= Radius;

	if (bPlayerInRange && !bJumpSlamDamageApplied)
	{
		UGameplayStatics::ApplyDamage(PlayerCharacter, Damage, this, ControlledPawn, nullptr);
		bJumpSlamDamageApplied = true;
	}
	if (bPlayerInRange)
	{
		LaunchPlayerFromJumpSlam(PlayerCharacter);
	}

	const bool bPlayerHitByJumpSlam = bJumpSlamDamageApplied;

	DrawDebugCircle(
		World,
		LockedJumpSlamLandingLocation,
		Radius,
		48,
		bPlayerHitByJumpSlam ? FColor::Red : FColor::Cyan,
		false,
		2.0f,
		0,
		3.0f,
		FVector::ForwardVector,
		FVector::RightVector,
		false
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("BossCubeAIController: Jump slam landed. PlayerHit=%s. PlayerInLandingRange=%s. Damage=%.1f. Radius=%.1f. LockedLanding=%s."),
		bPlayerHitByJumpSlam ? TEXT("true") : TEXT("false"),
		bPlayerInRange ? TEXT("true") : TEXT("false"),
		Damage,
		Radius,
		*LockedJumpSlamLandingLocation.ToCompactString()
	);

	FinishJumpSlamAttack();
}

void ABossCubeAIController::FinishJumpSlamAttack()
{
	RestoreJumpSlamCollision();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JumpSlamMoveTimerHandle);
		World->GetTimerManager().ClearTimer(JumpSlamLandTimerHandle);
	}

	CurrentState = EBossCubeAIState::Chase;
	UE_LOG(LogTemp, Log, TEXT("BossCubeAIController: Jump slam finished. Return to chase."));
	UpdateChaseTarget();
}
