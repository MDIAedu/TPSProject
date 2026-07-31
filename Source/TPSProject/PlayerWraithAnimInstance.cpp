// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerWraithAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "PlayerCubeCharacter.h"

void UPlayerWraithAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedPawn = TryGetPawnOwner();
	UpdateLocomotionValues();
	UpdateAimValues();
}

void UPlayerWraithAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedPawn)
	{
		CachedPawn = TryGetPawnOwner();
	}

	UpdateLocomotionValues();
	UpdateAimValues();
}

void UPlayerWraithAnimInstance::UpdateLocomotionValues()
{
	if (!CachedPawn)
	{
		Speed = 0.0f;
		Direction = 0.0f;
		bIsMoving = false;
		bIsInAir = false;
		return;
	}

	const FVector HorizontalVelocity(CachedPawn->GetVelocity().X, CachedPawn->GetVelocity().Y, 0.0f);
	Speed = HorizontalVelocity.Size();
	bIsMoving = Speed > 3.0f;
	bIsInAir = false;

	const ACharacter* CharacterOwner = Cast<ACharacter>(CachedPawn);
	if (CharacterOwner && CharacterOwner->GetCharacterMovement())
	{
		bIsInAir = CharacterOwner->GetCharacterMovement()->IsFalling();
	}

	if (!bIsMoving)
	{
		Direction = 0.0f;
		return;
	}

	const FVector LocalVelocity = CachedPawn->GetActorTransform().InverseTransformVectorNoScale(HorizontalVelocity);
	Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}

void UPlayerWraithAnimInstance::UpdateAimValues()
{
	const APlayerCubeCharacter* PlayerCharacter = Cast<APlayerCubeCharacter>(CachedPawn);
	if (!PlayerCharacter)
	{
		AimPitch = 0.0f;
		return;
	}

	AimPitch = PlayerCharacter->GetAimPitch();
}
