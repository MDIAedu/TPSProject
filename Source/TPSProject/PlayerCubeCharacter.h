// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCubeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USpringArmComponent;
struct FInputActionValue;

UCLASS()
class TPSPROJECT_API APlayerCubeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 플레이어 Character의 기본 컴포넌트와 이동 값을 만든다.
	APlayerCubeCharacter();

	// HUD나 Blueprint에서 현재 사격 콤보 단계를 읽는다.
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	int32 GetCurrentFireComboStep() const;

	// HUD나 Blueprint에서 현재 사격 콤보 단계의 피해량 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	float GetCurrentFireComboDamage() const;

protected:
	// 이동 Input Action을 Enhanced Input 컴포넌트에 연결한다.
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> ShoulderCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> MovementMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 MovementMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireTraceDistance = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireDebugDrawTime = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboStep1Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboStep2Damage = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboStep3Damage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboResetTime = 0.8f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	int32 CurrentFireComboStep = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Aim Camera")
	float DefaultCameraArmLength = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim Camera")
	float AimingCameraArmLength = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim Camera")
	float DefaultCameraFov = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim Camera")
	float AimingCameraFov = 65.0f;

	bool bIsAiming = false;
	float LastFireComboInputTime = -1000.0f;

	// 현재 소유 플레이어에 이동 입력 매핑 컨텍스트를 등록한다.
	void RegisterMovementMappingContext() const;

	// WASD 입력으로 받은 2D 값을 월드 X/Y 방향 이동으로 바꾼다.
	void Move(const FInputActionValue& Value);

	// 마우스 입력으로 받은 2D 값을 카메라 회전 입력으로 바꾼다.
	void Look(const FInputActionValue& Value);

	// 조준 입력이 눌렸을 때 사격 가능한 조준 상태로 바꾼다.
	void StartAim();

	// 조준 입력이 끝났을 때 조준 상태를 해제한다.
	void StopAim();

	// 발사 입력이 들어오면 카메라 정면으로 명중 판정을 확인한다.
	void Fire();

	// 좌클릭 입력 시간에 따라 현재 사격 콤보 단계를 갱신한다.
	void AdvanceFireCombo();
};
