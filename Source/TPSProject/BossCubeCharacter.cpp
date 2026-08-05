// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossCubeCharacter.h"

#include "BossCubeAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABossCubeCharacter::ABossCubeCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = ABossCubeAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(60.0f, 90.0f);

	BossVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BossVisualMesh"));
	BossVisualMesh->SetupAttachment(GetRootComponent());
	BossVisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -30.0f));
	BossVisualMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
	BossVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	CharacterMovementComponent->MaxWalkSpeed = 350.0f;
	CharacterMovementComponent->MaxAcceleration = 1200.0f;
	CharacterMovementComponent->BrakingDecelerationWalking = 1200.0f;
	CharacterMovementComponent->bOrientRotationToMovement = true;
	CharacterMovementComponent->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
}

float ABossCubeCharacter::GetMeleeAttackRange() const
{
	return MeleeAttackRange;
}

float ABossCubeCharacter::GetMeleeAttackDamage() const
{
	return MeleeAttackDamage;
}

float ABossCubeCharacter::GetMeleeAttackCooldown() const
{
	return MeleeAttackCooldown;
}

float ABossCubeCharacter::GetMeleeAttackDuration() const
{
	return MeleeAttackDuration;
}
