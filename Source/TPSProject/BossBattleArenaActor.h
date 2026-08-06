// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossBattleArenaActor.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class TPSPROJECT_API ABossBattleArenaActor : public AActor
{
	GENERATED_BODY()

public:
	// 원형 보스 전투장의 기본 컴포넌트와 조정 가능한 값을 만든다.
	ABossBattleArenaActor();

	// 에디터에서 값이 바뀔 때마다 전투장 바닥, 외곽 Mesh, 충돌 경계를 다시 구성한다.
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> ArenaRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FloorMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> BoundaryMeshInstances;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "500.0", AllowPrivateAccess = "true"))
	float ArenaRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "8", ClampMax = "64", AllowPrivateAccess = "true"))
	int32 BoundarySegmentCount = 24;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "50.0", AllowPrivateAccess = "true"))
	float BoundaryThickness = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "100.0", AllowPrivateAccess = "true"))
	float BoundaryHeight = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float BoundaryHeightVariation = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float BoundaryRadiusVariation = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float BoundaryThicknessVariation = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Shape", meta = (ClampMin = "0.5", ClampMax = "2.0", AllowPrivateAccess = "true"))
	float BoundarySegmentOverlapScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> FloorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> BoundaryMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual", meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float FloorMeshSourceSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Visual", meta = (ClampMin = "1.0", AllowPrivateAccess = "true"))
	float BoundaryMeshSourceSize = 100.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> BoundaryCollisionComponents;

	// 기존 Construction 충돌 컴포넌트를 제거하고 현재 수치 기준으로 다시 만든다.
	void RebuildArena();

	// 중앙의 평평한 전투 공간 Mesh를 현재 반지름에 맞게 갱신한다.
	void UpdateFloorMesh();

	// 외곽 산/절벽/바위 링처럼 보일 Instanced Static Mesh 경계를 현재 수치에 맞게 갱신한다.
	void UpdateBoundaryMeshes(int32 SafeSegmentCount, float SegmentLength);

	// 플레이어와 보스가 전투장 밖으로 나가지 못하도록 Box 충돌 경계를 원형으로 배치한다.
	void RebuildBoundaryCollision(int32 SafeSegmentCount, float SegmentLength);

	// 세그먼트마다 같은 결과가 나오도록 산 능선처럼 보이는 높낮이 값을 만든다.
	float GetBoundaryVariationAlpha(int32 SegmentIndex, int32 SafeSegmentCount) const;
};
