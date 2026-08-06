// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossCubeCharacter.generated.h"

class UStaticMeshComponent;

UCLASS()
class TPSPROJECT_API ABossCubeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// 길찾기 추적 검증용 보스 Character의 기본 컴포넌트와 이동 값을 만든다.
	ABossCubeCharacter();

	// 보스 일반 공격이 닿는 거리 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Melee Attack")
	float GetMeleeAttackRange() const;

	// 보스 일반 공격의 피해량 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Melee Attack")
	float GetMeleeAttackDamage() const;

	// 보스 일반 공격 후 다시 공격할 수 있기까지의 시간을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Melee Attack")
	float GetMeleeAttackCooldown() const;

	// 보스 일반 공격 상태가 유지되는 시간을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Melee Attack")
	float GetMeleeAttackDuration() const;

	// 점프 내려찍기 공격 후보가 되는 최소 거리를 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamTriggerDistance() const;

	// 점프 내려찍기 공격 발동 확률을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamChance() const;

	// 점프 내려찍기 공격 이동 시간을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamDuration() const;

	// 점프 내려찍기 시작 애니메이션을 재생할 시간을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamStartAnimDuration() const;

	// 점프 내려찍기 착지 애니메이션을 재생할 시간을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamLandAnimDuration() const;

	// 점프 내려찍기 공격 원형 판정 범위를 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamDamageRadius() const;

	// 점프 내려찍기 공격 피해량을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamDamage() const;

	// 점프 내려찍기 공격 재사용 대기시간을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamCooldown() const;

	// 점프 내려찍기 이동 중 임시로 들어올리는 높이를 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamArcHeight() const;

	// 점프 내려찍기 중 플레이어와 겹쳤을 때 밀어내는 힘을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Boss|Jump Slam")
	float GetJumpSlamOverlapLaunchStrength() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BossVisualMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Melee Attack", meta = (AllowPrivateAccess = "true"))
	float MeleeAttackRange = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Melee Attack", meta = (AllowPrivateAccess = "true"))
	float MeleeAttackDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Melee Attack", meta = (AllowPrivateAccess = "true"))
	float MeleeAttackCooldown = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Melee Attack", meta = (AllowPrivateAccess = "true"))
	float MeleeAttackDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamTriggerDistance = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float JumpSlamChance = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamStartAnimDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamLandAnimDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamDamageRadius = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamCooldown = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamArcHeight = 280.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Jump Slam", meta = (AllowPrivateAccess = "true"))
	float JumpSlamOverlapLaunchStrength = 900.0f;
};
