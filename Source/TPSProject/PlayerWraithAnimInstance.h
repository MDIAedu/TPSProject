// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PlayerWraithAnimInstance.generated.h"

class APawn;

UCLASS(Blueprintable, BlueprintType)
class TPSPROJECT_API UPlayerWraithAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// 애니메이션 인스턴스가 시작될 때 소유 Pawn을 캐싱한다.
	virtual void NativeInitializeAnimation() override;

	// 매 프레임 이동 속도와 방향 값을 갱신한다.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsMoving = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedPawn;

	// 소유 Pawn의 월드 속도를 애니메이션 Blueprint에서 사용할 로컬 이동 값으로 바꾼다.
	void UpdateLocomotionValues();
};
