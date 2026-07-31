// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerCubeCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"

APlayerCubeCharacter::APlayerCubeCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	CharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	CharacterMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

int32 APlayerCubeCharacter::GetCurrentFireComboStep() const
{
	return CurrentFireComboStep;
}

float APlayerCubeCharacter::GetCurrentFireComboDamage() const
{
	switch (CurrentFireComboStep)
	{
	case 1:
		return FireComboStep1Damage;
	case 2:
		return FireComboStep2Damage;
	case 3:
		return FireComboStep3Damage;
	default:
		return 0.0f;
	}
}

float APlayerCubeCharacter::GetAimPitch() const
{
	const FRotator ControlRotation = GetControlRotation();
	return FMath::Clamp(FRotator::NormalizeAxis(ControlRotation.Pitch), -89.0f, 89.0f);
}

void APlayerCubeCharacter::OpenFireComboInputWindow()
{
	if (!bIsFireComboAttackActive || CurrentFireComboStep >= 3)
	{
		return;
	}

	bIsFireComboInputWindowOpen = true;
	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Fire combo input window opened. ComboStep=%d."), CurrentFireComboStep);
}

void APlayerCubeCharacter::CloseFireComboInputWindow()
{
	bIsFireComboInputWindowOpen = false;
	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Fire combo input window closed. ComboStep=%d."), CurrentFireComboStep);
}

void APlayerCubeCharacter::EndFireComboAttack()
{
	if (!bIsFireComboAttackActive && CurrentFireComboStep == 0)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Fire combo attack ended. ComboStep=%d."), CurrentFireComboStep);
	bIsFireComboAttackActive = false;
	bIsFireComboInputWindowOpen = false;
	CurrentFireComboStep = 0;
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
		// MoveAction 입력이 계속 들어오는 동안 Move 함수를 호출해서 플레이어 Character를 이동시킨다.
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCubeCharacter::Move);
	}

	if (LookAction)
	{
		// LookAction 입력이 들어오는 동안 Look 함수를 호출해서 카메라 방향을 회전시킨다.
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCubeCharacter::Look);
	}

	if (JumpAction)
	{
		// 스페이스바 같은 점프 입력이 눌리는 순간 CharacterMovement의 점프 흐름을 시작한다.
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCubeCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCubeCharacter::StopJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &APlayerCubeCharacter::StopJump);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: JumpAction is not assigned."));
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

void APlayerCubeCharacter::StartJump()
{
	if (!CanJump())
	{
		UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Jump input ignored because character cannot jump now."));
		return;
	}

	Jump();
	UE_LOG(LogTemp, Log, TEXT("PlayerCubeCharacter: Jump started."));
}

void APlayerCubeCharacter::StopJump()
{
	StopJumping();
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
	if (!TryAdvanceFireComboFromInput())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("PlayerCubeCharacter: Fire input ignored by Anim Notify combo window. ComboStep=%d. WindowOpen=%s."),
			CurrentFireComboStep,
			bIsFireComboInputWindowOpen ? TEXT("true") : TEXT("false")
		);
		return;
	}

	const float CurrentComboDamage = GetCurrentFireComboDamage();
	PlayFireComboAnimation();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerCubeCharacter: Fire input received. IsAiming=%s. ComboStep=%d. ComboDamage=%.1f."),
		bIsAiming ? TEXT("true") : TEXT("false"),
		CurrentFireComboStep,
		CurrentComboDamage
	);

	PerformFireTrace(CurrentComboDamage);
}

bool APlayerCubeCharacter::TryAdvanceFireComboFromInput()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		CurrentFireComboStep = 1;
		LastFireComboInputTime = -1000.0f;
		bIsFireComboAttackActive = true;
		bIsFireComboInputWindowOpen = false;
		return true;
	}

	if (!bIsFireComboAttackActive || CurrentFireComboStep == 0)
	{
		CurrentFireComboStep = 1;
		LastFireComboInputTime = World->GetTimeSeconds();
		bIsFireComboAttackActive = true;
		bIsFireComboInputWindowOpen = false;
		return true;
	}

	if (!bIsFireComboInputWindowOpen || CurrentFireComboStep >= 3)
	{
		return false;
	}

	++CurrentFireComboStep;
	LastFireComboInputTime = World->GetTimeSeconds();
	bIsFireComboInputWindowOpen = false;
	return true;
}

void APlayerCubeCharacter::PlayFireComboAnimation()
{
	UAnimMontage* FireComboMontage = GetFireComboMontageForStep(CurrentFireComboStep);
	if (!FireComboMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: Fire combo montage is not assigned. ComboStep=%d."), CurrentFireComboStep);
		return;
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: Fire combo montage ignored because CharacterMesh is missing."));
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerCubeCharacter: Fire combo montage ignored because AnimInstance is missing."));
		return;
	}

	AnimInstance->Montage_Stop(FireComboMontageStopBlendOutTime);
	AnimInstance->Montage_Play(FireComboMontage, FireComboMontagePlayRate);

	const FName SectionName = GetFireComboSectionNameForStep(CurrentFireComboStep);
	if (!SectionName.IsNone())
	{
		AnimInstance->Montage_JumpToSection(SectionName, FireComboMontage);
	}
}

UAnimMontage* APlayerCubeCharacter::GetFireComboMontageForStep(int32 ComboStep) const
{
	switch (ComboStep)
	{
	case 1:
		return FireComboStep1Montage.Get();
	case 2:
		return FireComboStep2Montage ? FireComboStep2Montage.Get() : FireComboStep1Montage.Get();
	case 3:
		if (FireComboStep3Montage)
		{
			return FireComboStep3Montage.Get();
		}
		return FireComboStep2Montage ? FireComboStep2Montage.Get() : FireComboStep1Montage.Get();
	default:
		return nullptr;
	}
}

FName APlayerCubeCharacter::GetFireComboSectionNameForStep(int32 ComboStep) const
{
	switch (ComboStep)
	{
	case 1:
		return FireComboStep1SectionName;
	case 2:
		return FireComboStep2SectionName;
	case 3:
		return FireComboStep3SectionName;
	default:
		return NAME_None;
	}
}

void APlayerCubeCharacter::PerformFireTrace(float CurrentComboDamage)
{
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
		UE_LOG(
			LogTemp,
			Log,
			TEXT("PlayerCubeCharacter: Fire hit %s. ComboStep=%d. ComboDamage=%.1f."),
			*GetNameSafe(HitResult.GetActor()),
			CurrentFireComboStep,
			CurrentComboDamage
		);
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerCubeCharacter: Fire missed. ComboStep=%d. ComboDamage=%.1f."),
		CurrentFireComboStep,
		CurrentComboDamage
	);
}
