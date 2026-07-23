// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerCubePawn.h"

#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

APlayerCubePawn::APlayerCubePawn()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	SetRootComponent(CubeMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		CubeMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	CubeMesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
	CubeMesh->SetCollisionProfileName(TEXT("Pawn"));

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = 600.0f;
	MovementComponent->Acceleration = 2400.0f;
	MovementComponent->Deceleration = 2400.0f;
}

void APlayerCubePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	RegisterMovementMappingContext();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent || !MoveAction)
	{
		return;
	}

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCubePawn::Move);
}

void APlayerCubePawn::RegisterMovementMappingContext() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !MovementMappingContext)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		InputSubsystem->AddMappingContext(MovementMappingContext, MovementMappingPriority);
	}
}

void APlayerCubePawn::Move(const FInputActionValue& Value)
{
	const FVector2D MovementInput = Value.Get<FVector2D>();
	if (MovementInput.IsNearlyZero())
	{
		return;
	}

	AddMovementInput(FVector::ForwardVector, MovementInput.Y);
	AddMovementInput(FVector::RightVector, MovementInput.X);
}
