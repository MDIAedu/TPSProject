// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerCubeCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
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
	CameraBoom->TargetArmLength = DefaultCameraArmLength;
	CameraBoom->SocketOffset = FVector(0.0f, 80.0f, 70.0f);
	CameraBoom->bUsePawnControlRotation = true;

	ShoulderCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ShoulderCamera"));
	ShoulderCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ShoulderCamera->bUsePawnControlRotation = false;
	ShoulderCamera->FieldOfView = DefaultCameraFov;

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

	if (AimAction)
	{
		// 우클릭을 누르고 있는 동안 확대 조준 카메라 상태로 둔다.
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &APlayerCubeCharacter::StartAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &APlayerCubeCharacter::StopAim);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &APlayerCubeCharacter::StopAim);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: AimAction is not assigned."));
	}

	if (FireAction)
	{
		// 좌클릭이 눌리는 순간 한 번만 기본 사격 판정을 확인한다.
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCubeCharacter::Fire);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: FireAction is not assigned."));
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

void APlayerCubeCharacter::StartAim()
{
	bIsAiming = true;
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = AimingCameraArmLength;
	}
	if (ShoulderCamera)
	{
		ShoulderCamera->SetFieldOfView(AimingCameraFov);
	}
	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Aim started."));
}

void APlayerCubeCharacter::StopAim()
{
	bIsAiming = false;
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = DefaultCameraArmLength;
	}
	if (ShoulderCamera)
	{
		ShoulderCamera->SetFieldOfView(DefaultCameraFov);
	}
	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Aim stopped."));
}

void APlayerCubeCharacter::Fire()
{
	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Fire input received. IsAiming=%s."), bIsAiming ? TEXT("true") : TEXT("false"));

	if (!ShoulderCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: Fire ignored because ShoulderCamera is missing."));
		return;
	}

	const FVector TraceStart = ShoulderCamera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + ShoulderCamera->GetForwardVector() * FireTraceDistance;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerCubeFireTrace), false, this);

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: Fire ignored because World is missing."));
		return;
	}

	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	const FVector DebugEnd = bHit ? HitResult.ImpactPoint : TraceEnd;
	const FColor DebugColor = bHit ? FColor::Green : FColor::Red;
	DrawDebugLine(World, TraceStart, DebugEnd, DebugColor, false, FireDebugDrawTime, 0, 2.0f);

	if (bHit)
	{
		DrawDebugPoint(World, HitResult.ImpactPoint, 12.0f, FColor::Yellow, false, FireDebugDrawTime);
		UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Fire hit %s."), *GetNameSafe(HitResult.GetActor()));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Fire missed."));
}
