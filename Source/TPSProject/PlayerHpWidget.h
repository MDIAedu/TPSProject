// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHpWidget.generated.h"

class APlayerCubeCharacter;

UCLASS()
class TPSPROJECT_API UPlayerHpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Widget Blueprint에서 플레이어 현재 HP 비율을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetPlayerHealthPercent() const;

	// Widget Blueprint에서 플레이어 현재 HP 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetPlayerCurrentHealth() const;

	// Widget Blueprint에서 플레이어 최대 HP 값을 읽는다.
	UFUNCTION(BlueprintPure, Category = "Player|Health")
	float GetPlayerMaxHealth() const;

protected:
	// Widget이 화면에 생성될 때 로컬 플레이어 Character를 찾는다.
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<APlayerCubeCharacter> CachedPlayerCharacter;

	// 캐시된 플레이어 Character가 없으면 다시 찾는다.
	APlayerCubeCharacter* GetPlayerCharacter() const;
};
