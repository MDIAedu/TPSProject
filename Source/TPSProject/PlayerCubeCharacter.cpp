// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerCubeCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

APlayerCubeCharacter::APlayerCubeCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(50.0f, 50.0f);

	CubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	CubeMesh->SetupAttachment(GetRootComponent());

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		CubeMesh->SetStaticMesh(CubeMeshAsset.Object);
	}

	CubeMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	CubeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 450.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 80.0f, 70.0f);
	CameraBoom->bUsePawnControlRotation = true;

	ShoulderCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ShoulderCamera"));
	ShoulderCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ShoulderCamera->bUsePawnControlRotation = false;

	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	CharacterMovementComponent->MaxWalkSpeed = 600.0f;
	CharacterMovementComponent->MaxAcceleration = 2400.0f;
	CharacterMovementComponent->BrakingDecelerationWalking = 2400.0f;
	CharacterMovementComponent->bOrientRotationToMovement = true;
}

void APlayerCubeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 이 Character가 조작되기 시작할 때, 플레이어에게 이동 입력 매핑 컨텍스트를 등록한다.
	// 여기서 등록해야 IA_Move 같은 Input Action이 실제 키 입력과 연결된다.
	RegisterMovementMappingContext();

	// Enhanced Input을 사용하려면 기본 UInputComponent를 UEnhancedInputComponent로 변환해야 한다.
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		// Enhanced Input 컴포넌트가 아니면 입력을 묶을 대상이 없으므로 안전하게 함수를 끝낸다.
		return;
	}

	if (MoveAction)
	{
		// MoveAction 입력이 계속 들어오는 동안 Move 함수를 호출해서 큐브 Character를 이동시킨다.
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCubeCharacter::Move);
	}

	if (LookAction)
	{
		// LookAction 입력이 들어오는 동안 Look 함수를 호출해서 카메라 방향을 회전시킨다.
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCubeCharacter::Look);
	}
}

void APlayerCubeCharacter::RegisterMovementMappingContext() const
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

void APlayerCubeCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementInput = Value.Get<FVector2D>();
	if (MovementInput.IsNearlyZero())
	{
		return;
	}
	
	const FRotator ControlRotation = GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	// 카메라의 수평 회전값만 사용해서 바라보는 방향 기준으로 앞뒤/좌우 이동을 계산한다.
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementInput.Y);
	AddMovementInput(RightDirection, MovementInput.X);
}

void APlayerCubeCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	if (LookInput.IsNearlyZero())
	{
		return;
	}

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}
