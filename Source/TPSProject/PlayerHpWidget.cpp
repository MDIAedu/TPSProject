// Copyright Epic Games, Inc. All Rights Reserved.

#include "PlayerHpWidget.h"

#include "Kismet/GameplayStatics.h"
#include "PlayerCubeCharacter.h"

void UPlayerHpWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CachedPlayerCharacter = Cast<APlayerCubeCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

float UPlayerHpWidget::GetPlayerHealthPercent() const
{
	const APlayerCubeCharacter* PlayerCharacter = GetPlayerCharacter();
	return PlayerCharacter ? PlayerCharacter->GetHealthPercent() : 0.0f;
}

float UPlayerHpWidget::GetPlayerCurrentHealth() const
{
	const APlayerCubeCharacter* PlayerCharacter = GetPlayerCharacter();
	return PlayerCharacter ? PlayerCharacter->GetCurrentHealth() : 0.0f;
}

float UPlayerHpWidget::GetPlayerMaxHealth() const
{
	const APlayerCubeCharacter* PlayerCharacter = GetPlayerCharacter();
	return PlayerCharacter ? PlayerCharacter->GetMaxHealth() : 0.0f;
}

APlayerCubeCharacter* UPlayerHpWidget::GetPlayerCharacter() const
{
	if (CachedPlayerCharacter)
	{
		return CachedPlayerCharacter.Get();
	}

	return Cast<APlayerCubeCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}
