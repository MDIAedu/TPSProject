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
};
