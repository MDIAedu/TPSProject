// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossBattleArenaActor.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABossBattleArenaActor::ABossBattleArenaActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ArenaRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot"));
	SetRootComponent(ArenaRoot);

	FloorMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMeshComponent->SetupAttachment(ArenaRoot);
	FloorMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FloorMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

	BoundaryMeshInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BoundaryMeshInstances"));
	BoundaryMeshInstances->SetupAttachment(ArenaRoot);
	BoundaryMeshInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BasicCubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BasicCubeMesh.Succeeded())
	{
		FloorMesh = BasicCubeMesh.Object;
		BoundaryMesh = BasicCubeMesh.Object;
	}
}

void ABossBattleArenaActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RebuildArena();
}

void ABossBattleArenaActor::RebuildArena()
{
	const int32 SafeSegmentCount = FMath::Max(BoundarySegmentCount, 8);
	const float SafeRadius = FMath::Max(ArenaRadius, 500.0f);
	const float SegmentLength = (2.0f * UE_PI * SafeRadius / static_cast<float>(SafeSegmentCount)) * BoundarySegmentOverlapScale;

	UpdateFloorMesh();
	UpdateBoundaryMeshes(SafeSegmentCount, SegmentLength);
	RebuildBoundaryCollision(SafeSegmentCount, SegmentLength);
}

void ABossBattleArenaActor::UpdateFloorMesh()
{
	FloorMeshComponent->SetStaticMesh(FloorMesh);

	const float SafeSourceSize = FMath::Max(FloorMeshSourceSize, 1.0f);
	const float SafeRadius = FMath::Max(ArenaRadius, 500.0f);
	const float Diameter = FMath::Max(SafeRadius * 2.0f, 1.0f);
	FloorMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	FloorMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
	FloorMeshComponent->SetRelativeScale3D(FVector(Diameter / SafeSourceSize, Diameter / SafeSourceSize, 0.1f));
}

void ABossBattleArenaActor::UpdateBoundaryMeshes(int32 SafeSegmentCount, float SegmentLength)
{
	BoundaryMeshInstances->ClearInstances();
	BoundaryMeshInstances->SetStaticMesh(BoundaryMesh);

	if (!BoundaryMesh)
	{
		return;
	}

	const float SafeSourceSize = FMath::Max(BoundaryMeshSourceSize, 1.0f);
	const float BaseBoundaryCenterRadius = FMath::Max(ArenaRadius, 500.0f) + BoundaryThickness * 0.5f;

	for (int32 SegmentIndex = 0; SegmentIndex < SafeSegmentCount; ++SegmentIndex)
	{
		const float AngleRadians = 2.0f * UE_PI * static_cast<float>(SegmentIndex) / static_cast<float>(SafeSegmentCount);
		const float VariationAlpha = GetBoundaryVariationAlpha(SegmentIndex, SafeSegmentCount);
		const float HeightScale = FMath::Lerp(1.0f - BoundaryHeightVariation, 1.0f + BoundaryHeightVariation, VariationAlpha);
		const float ThicknessScale = FMath::Lerp(1.0f - BoundaryThicknessVariation, 1.0f + BoundaryThicknessVariation, 1.0f - VariationAlpha);
		const float RadiusOffset = FMath::Lerp(-BoundaryThickness, BoundaryThickness, VariationAlpha - 0.5f) * BoundaryRadiusVariation;
		const float SegmentHeight = FMath::Max(BoundaryHeight * HeightScale, 100.0f);
		const float SegmentThickness = FMath::Max(BoundaryThickness * ThicknessScale, 50.0f);
		const float BoundaryCenterRadius = BaseBoundaryCenterRadius + RadiusOffset;
		const FVector SegmentScale(
			FMath::Max(SegmentThickness / SafeSourceSize, 0.01f),
			FMath::Max(SegmentLength / SafeSourceSize, 0.01f),
			FMath::Max(SegmentHeight / SafeSourceSize, 0.01f)
		);
		const FVector SegmentLocation(
			FMath::Cos(AngleRadians) * BoundaryCenterRadius,
			FMath::Sin(AngleRadians) * BoundaryCenterRadius,
			SegmentHeight * 0.5f
		);
		const FRotator SegmentRotation(0.0f, FMath::RadiansToDegrees(AngleRadians), FMath::Lerp(-4.0f, 4.0f, VariationAlpha));

		BoundaryMeshInstances->AddInstance(FTransform(SegmentRotation, SegmentLocation, SegmentScale));
	}
}

void ABossBattleArenaActor::RebuildBoundaryCollision(int32 SafeSegmentCount, float SegmentLength)
{
	for (UBoxComponent* BoundaryCollisionComponent : BoundaryCollisionComponents)
	{
		if (BoundaryCollisionComponent)
		{
			BoundaryCollisionComponent->DestroyComponent();
		}
	}
	BoundaryCollisionComponents.Reset();

	const float BoundaryCenterRadius = FMath::Max(ArenaRadius, 500.0f) + BoundaryThickness * 0.5f;
	const FVector BoxExtent(
		FMath::Max(BoundaryThickness * 0.5f, 1.0f),
		FMath::Max(SegmentLength * 0.5f, 1.0f),
		FMath::Max(BoundaryHeight * 0.5f, 1.0f)
	);

	for (int32 SegmentIndex = 0; SegmentIndex < SafeSegmentCount; ++SegmentIndex)
	{
		const float AngleRadians = 2.0f * UE_PI * static_cast<float>(SegmentIndex) / static_cast<float>(SafeSegmentCount);
		const FVector SegmentLocation(
			FMath::Cos(AngleRadians) * BoundaryCenterRadius,
			FMath::Sin(AngleRadians) * BoundaryCenterRadius,
			BoundaryHeight * 0.5f
		);
		const FRotator SegmentRotation(0.0f, FMath::RadiansToDegrees(AngleRadians), 0.0f);

		UBoxComponent* BoundaryCollisionComponent = NewObject<UBoxComponent>(this);
		BoundaryCollisionComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		BoundaryCollisionComponent->SetupAttachment(ArenaRoot);
		BoundaryCollisionComponent->RegisterComponent();
		BoundaryCollisionComponent->SetRelativeLocation(SegmentLocation);
		BoundaryCollisionComponent->SetRelativeRotation(SegmentRotation);
		BoundaryCollisionComponent->SetBoxExtent(BoxExtent);
		BoundaryCollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BoundaryCollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
		BoundaryCollisionComponent->SetHiddenInGame(true);

		BoundaryCollisionComponents.Add(BoundaryCollisionComponent);
	}
}

float ABossBattleArenaActor::GetBoundaryVariationAlpha(int32 SegmentIndex, int32 SafeSegmentCount) const
{
	const float NormalizedIndex = static_cast<float>(SegmentIndex) / static_cast<float>(FMath::Max(SafeSegmentCount, 1));
	const float LargeWave = FMath::Sin(NormalizedIndex * UE_PI * 2.0f * 3.0f);
	const float SmallWave = FMath::Sin((NormalizedIndex * UE_PI * 2.0f * 7.0f) + 0.75f);
	return FMath::Clamp((LargeWave * 0.65f + SmallWave * 0.35f + 1.0f) * 0.5f, 0.0f, 1.0f);
}
