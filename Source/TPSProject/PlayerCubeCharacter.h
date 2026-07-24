// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCubeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS()
class TPSPROJECT_API APlayerCubeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 플레이어 이동 검증용 큐브 Character의 기본 컴포넌트와 이동 값을 만든다.
	APlayerCubeCharacter();

protected:
	// 이동 Input Action을 Enhanced Input 컴포넌트에 연결한다.
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Cube", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CubeMesh;

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

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 MovementMappingPriority = 0;

	// 현재 소유 플레이어에 이동 입력 매핑 컨텍스트를 등록한다.
	void RegisterMovementMappingContext() const;

	// WASD 입력으로 받은 2D 값을 월드 X/Y 방향 이동으로 바꾼다.
	void Move(const FInputActionValue& Value);

	// 마우스 입력으로 받은 2D 값을 카메라 회전 입력으로 바꾼다.
	void Look(const FInputActionValue& Value);
};
