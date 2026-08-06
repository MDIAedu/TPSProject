// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCubeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class UAnimMontage;
class UPlayerHpWidget;
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

	// Animation Blueprint에서 Aim Offset 세로 축으로 사용할 카메라 Pitch 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Aim")
	float GetAimPitch() const;

	// Widget Blueprint에서 플레이어 현재 HP 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetCurrentHealth() const;

	// Widget Blueprint에서 플레이어 최대 HP 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetMaxHealth() const;

	// Widget Blueprint에서 플레이어 HP 비율을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetHealthPercent() const;

	// Anim Notify에서 다음 콤보 입력을 받을 수 있는 구간을 연다.
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void OpenFireComboInputWindow();

	// Anim Notify에서 다음 콤보 입력을 받을 수 있는 구간을 닫는다.
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void CloseFireComboInputWindow();

	// Anim Notify 또는 몽타주 종료 시점에서 현재 사격 콤보를 끝낸다.
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void EndFireComboAttack();

protected:
	// 게임이 시작될 때 HP 초기값과 선택된 HP Widget을 준비한다.
	virtual void BeginPlay() override;

	// 보스 공격의 ApplyDamage 호출을 받아 플레이어 HP를 줄인다.
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

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
	TObjectPtr<UInputAction> JumpAction;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Health", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPlayerHpWidget> PlayerHpWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerHpWidget> PlayerHpWidgetInstance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboStep1Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboStep2Damage = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboStep3Damage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
	float FireComboResetTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> FireComboStep1Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> FireComboStep2Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> FireComboStep3Montage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	FName FireComboStep1SectionName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	FName FireComboStep2SectionName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	FName FireComboStep3SectionName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	float FireComboMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo|Animation", meta = (AllowPrivateAccess = "true"))
	float FireComboMontageStopBlendOutTime = 0.15f;

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
	bool bIsFireComboAttackActive = false;
	bool bIsFireComboInputWindowOpen = false;
	float LastFireComboInputTime = -1000.0f;

	// 현재 소유 플레이어에 이동 입력 매핑 컨텍스트를 등록한다.
	void RegisterMovementMappingContext() const;

	// WASD 입력으로 받은 2D 값을 월드 X/Y 방향 이동으로 바꾼다.
	void Move(const FInputActionValue& Value);

	// 마우스 입력으로 받은 2D 값을 카메라 회전 입력으로 바꾼다.
	void Look(const FInputActionValue& Value);

	// 점프 입력이 눌렸을 때 지상 상태라면 점프를 시작한다.
	void StartJump();

	// 점프 입력이 끝났을 때 점프 입력 유지 상태를 해제한다.
	void StopJump();

	// 조준 입력이 눌렸을 때 사격 가능한 조준 상태로 바꾼다.
	void StartAim();

	// 조준 입력이 끝났을 때 조준 상태를 해제한다.
	void StopAim();

	// 발사 입력이 들어오면 카메라 정면으로 명중 판정을 확인한다.
	void Fire();

	// 좌클릭 입력이 현재 Anim Notify 입력 허용 상태에서 유효한지 판단하고 콤보 단계를 갱신한다.
	bool TryAdvanceFireComboFromInput();

	// 현재 사격 콤보 단계에 맞는 공격 몽타주 또는 섹션을 재생한다.
	void PlayFireComboAnimation();

	// 현재 사격 콤보 단계에 맞는 몽타주 자산을 찾는다.
	UAnimMontage* GetFireComboMontageForStep(int32 ComboStep) const;

	// 현재 사격 콤보 단계에 맞는 몽타주 섹션 이름을 찾는다.
	FName GetFireComboSectionNameForStep(int32 ComboStep) const;

	// 카메라 정면으로 기존 사격 명중 판정을 실행한다.
	void PerformFireTrace(float CurrentComboDamage);
};
