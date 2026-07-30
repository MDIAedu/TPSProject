// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerWraithAnimInstance.h"

#include "GameFramework/Pawn.h"

void UPlayerWraithAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedPawn = TryGetPawnOwner();
	UpdateLocomotionValues();
}

void UPlayerWraithAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedPawn)
	{
		CachedPawn = TryGetPawnOwner();
	}

	UpdateLocomotionValues();
}

void UPlayerWraithAnimInstance::UpdateLocomotionValues()
{
	if (!CachedPawn)
	{
		Speed = 0.0f;
		Direction = 0.0f;
		bIsMoving = false;
		return;
	}

	const FVector HorizontalVelocity(CachedPawn->GetVelocity().X, CachedPawn->GetVelocity().Y, 0.0f);
	Speed = HorizontalVelocity.Size();
	bIsMoving = Speed > 3.0f;

	if (!bIsMoving)
	{
		Direction = 0.0f;
		return;
	}

	const FVector LocalVelocity = CachedPawn->GetActorTransform().InverseTransformVectorNoScale(HorizontalVelocity);
	Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}
