// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Templates/SubclassOf.h"
#include "GameFramework/Actor.h"

#include "Logic/SoTriggerable.h"

#include "SoEnemySpawner.generated.h"

class ASoEnemy;
class ASoEnemySpawnHandler;
class UFMODEvent;

USTRUCT(BlueprintType)
struct FSoSpawnEntry
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ASoEnemy> Class;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 1;

	/** Tries to use this spawner before anything else, falls back to the rest if it is not set / can't spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ASoEnemySpawnHandler* SpawnerOverride = nullptr;

	int32 AlreadySpawned = 0;
};


USTRUCT(BlueprintType)
struct FSoSpawnGroup
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FSoSpawnEntry> Entry;
};


UCLASS()
class SORB_API ASoEnemySpawner : public AActor, public ISoTriggerable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoEnemySpawner(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Trigger_Implementation(const FSoTriggerData& TriggerData) override;

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void Spawn();

	UFUNCTION()
	void OnSpawnedDied(ASoEnemy* Victim);

	UFUNCTION()
	void OnNotSpawnedDied(ASoEnemy* Victim);

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void OnPlayerRespawn();

	FName GetSoGroupName() const { return SoGroupName; }

	/** returns with the amount of enemy this spawner will spawn */
	UFUNCTION(Category = "Spawn")
	int32 GetEnemyNum() const { return SoEnemyToSpawnNum; }

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void Activate();


	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void DeactivateAndCancelKilledOnes();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void DeactivateAndReset();

	bool IsActiveAndNotDone() const { return bActivated && !bFinished; }

	const TArray<ASoEnemy*>& GetActiveEnemies() const { return AllEnemiesAlive; }

protected:

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawn")
	void DeactivateAndResetBP();

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawn")
	void OnActivatedBP();

	UFUNCTION()
	void OnReload();

	/** @Return: false -> spawner finished */
	UFUNCTION(BlueprintNativeEvent, Category = "Spawn")
	bool SpawnInternal();

	UFUNCTION(BlueprintNativeEvent, Category = "Spawn")
	bool ShouldSpawnMore();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void Finish();

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawn")
	void OnFinishedSpawningAll(bool bBecauseReloaded);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawn")
	void OnSpawnedDiedBP();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	bool SpawnCreature(ASoEnemySpawnHandler* SpawnHandler, TSubclassOf<ASoEnemy> Class);

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	bool SpawnCreatureFromRandomHandler(TSubclassOf<ASoEnemy> Class);

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void DestroySpawnedCreatures(bool bFadeOutAlive = false);

	UFUNCTION(BlueprintImplementableEvent, Category = "Spawn")
	void OnPlayerRespawnBP();

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawn)
	FName SoGroupName;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawn)
	float EnemyGroupDefeatedDelayOverride = -1.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawn)
	TArray<ASoEnemySpawnHandler*> SpawnHandlers;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawn)
	TArray<FSoSpawnGroup> SpawnGroups;

	/** maximum amount of spawned creatures alive at any given time */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawn)
	int32 MaxSpawnedCreatureNum = 3;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawn)
	bool bAutoActivate = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Spawn)
	bool bAutoRestartOnPlayerDeath = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = SFX)
	UFMODEvent* Music = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Spawn)
	int32 SoEnemyToSpawnNum;

	UPROPERTY(EditAnywhere, Category = Spawn)
	bool bSpawnNextOnSpawnedDied = true;

	/** Enemies placed in the level, combat isn't over until they are alive */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<ASoEnemy*> NotSpawnedEnemiesAttached;

	UPROPERTY(BlueprintReadOnly)
	TArray<ASoEnemy*> NotSpawnedEnemiesWatched;

	UPROPERTY(BlueprintReadOnly)
	TArray<ASoEnemy*> SpawnedEnemiesAlive;

	/** SpawnedEnemiesAlive + NotSpawnedEnemiesWatched */
	UPROPERTY(BlueprintReadOnly)
	TArray<ASoEnemy*> AllEnemiesAlive;


	UPROPERTY()
	TArray<ASoEnemy*> SpawnedEnemiesDeadTemp;

	UPROPERTY(BlueprintReadOnly)
	int32 ActiveGroup = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bActivated = false;

	UPROPERTY(BlueprintReadOnly)
	bool bFinished = false;

	UPROPERTY()
	ASoEnemySpawnHandler* LastUsedSpawnHandler = nullptr;

	UPROPERTY()
	TArray<ASoEnemySpawnHandler*> HandlersUsedInCurrentSpawnGroup;

	FTimerHandle SpawnDelayTimer;

	int32 SpawnedDiedCounter = 0;
};
