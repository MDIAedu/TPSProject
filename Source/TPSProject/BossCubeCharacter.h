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

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BossVisualMesh;
};
