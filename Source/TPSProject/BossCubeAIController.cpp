// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossCubeAIController.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABossCubeAIController::ABossCubeAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABossCubeAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UpdateChaseTarget();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	//일정시간마다 특정 함수를 호출한다.	
	World->GetTimerManager().SetTimer(
		ChaseTimerHandle,
		this,
		&ABossCubeAIController::UpdateChaseTarget,
		ChaseRefreshInterval,
		true
	);
}

void ABossCubeAIController::OnUnPossess()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChaseTimerHandle);
	}

	Super::OnUnPossess();
}

void ABossCubeAIController::UpdateChaseTarget()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("BossCubeAIController: Player pawn is not found."));
		return;
	}

	MoveToActor(PlayerPawn, AcceptanceRadius, true, true, true, nullptr, true);
}
