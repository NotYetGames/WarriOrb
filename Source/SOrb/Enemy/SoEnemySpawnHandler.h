// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "Enemy/SoEnemy.h"
#include "SoEnemySpawnHandler.generated.h"

UCLASS()
class SORB_API ASoEnemySpawnHandler : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoEnemySpawnHandler(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Spawn)
	bool CanSpawn() const;

	UFUNCTION(BlueprintCallable, Category = Spawn)
	bool IsEnemyCompatible(TSubclassOf<ASoEnemy>& Class) const { return CompatibleEnemies.Contains(Class); }

	UFUNCTION(BlueprintCallable, Category = Spawn)
	ASoEnemy* SpawnCreature(TSubclassOf<ASoEnemy> Class);

	void SetSoGroupName(FName GroupName) { SoGroupName = GroupName; }

protected:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Spawn)
	ASoEnemy* SpawnCreatureBP(TSubclassOf<ASoEnemy> Class);


	UFUNCTION(BlueprintCallable, Category = Spawn)
	bool GetPointNearTarget(UPARAM(Ref)FSoSplinePointPtr& PointOnSpline,
							const FSoSplinePoint& Min,
							const FSoSplinePoint& Max,
							const FSoSplinePoint& Target,
							float DistanceFromTarget,
							bool bMoveActorToPoint,
							bool bLookAtTarget,
							int32 PreferredDirection = 0);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<ASoEnemy>> CompatibleEnemies;

	/** used if the child spawner handler visibility depends on the groups state, the spawners are supposed to set this in editor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SpawnedCreatureParam)
	FName SoGroupName;

	/** sets this array as marker list for the spawned creature */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SpawnedCreatureParam)
	TArray<ASoMarker*> MarkerList;

	/** used if not known: creature will get this is default secondary action list if it is valid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SpawnedCreatureParam)
	FName DefaultScriptOverride;

	UPROPERTY(EditAnywhere)
	bool bAllowSpawnMaterialAnimation = true;
};
