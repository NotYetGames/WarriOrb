// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "Basic/SoEventHandler.h"
#include "Logic/SoTriggerable.h"
#include "SplineLogic/SoSplinePointPtr.h"
#include "SoProjectileTypes.h"

#include "SoProjectileSpawnerAutomata.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoProjectileSpawnNotify);

class ASoProjectile;
class UArrowComponent;
class UFMODEvent;

UCLASS()
class SORB_API ASoProjectileSpawnerAutomata : public AActor, public ISoEventHandler, public ISoTriggerable
{
	GENERATED_BODY()

public:
	ASoProjectileSpawnerAutomata();

	// AActor:

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ISoEventHandler:
	virtual void HandleSoPostLoad_Implementation() override;
	virtual void HandlePlayerRematerialize_Implementation() override;
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};

	// ISoTriggerable
	virtual void Trigger_Implementation(const FSoTriggerData& InTriggerData) override;


	UFUNCTION(BlueprintCallable, Category = "SoProjectileSpawnerAutomata")
	void StartExecution();

	UFUNCTION(BlueprintCallable, Category = "SoProjectileSpawnerAutomata")
	void StopExecution(bool bCanUpdateSerializedState);

	UFUNCTION()
	void Fire();

	UFUNCTION()
	void TriggerTargets();

	UFUNCTION()
	void OnProjectileDied(ASoProjectile* Projectile);

	UFUNCTION()
	void OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline);

protected:

	UFUNCTION(BlueprintNativeEvent, Category = "SoProjectileSpawnerAutomata")
	void OnFireProjectiles();

	void UpdateSerializedState();

	void StartPreSimulation();

	void ClearTimer();

	/** Spawned Projectiles are deactivated and placed back to cache */
	void ClearSpawnedProjectiles();
	void ClearSpawnedProjectile(ASoProjectile* Projectile);

	bool ShouldUseWhitelistedSplines() const;
	bool CharIsOnWhitelistedSplines() const;

	UPROPERTY(BlueprintAssignable)
	FSoProjectileSpawnNotify OnProjectileSpawned;

	UPROPERTY(BlueprintAssignable)
	FSoProjectileSpawnNotify OnAllSpawnedProjectileDied;

protected:

	/** Projectiles are removed and state is reseted to default if set */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bResetOnPlayerRematerialize = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bActiveByDefault;

	UPROPERTY(BlueprintReadOnly, Category = "SoProjectileSpawnerAutomata")
	bool bWorking = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bLoop;

	/** If it is set the state (if active or not) will be serialized. Should not be changed runtime */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bSerializeState;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	float PreSimulationTime = -1.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bPreSimulationPhase = false;


	/** Used each time in StartExecution before we wait for the first time, obviously skipped if < KINDA_SMALL_NUMBER */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	float FirstTimeOffset = -1.0f;

	/** used only if FirstTimeOffset is ~0, decides if the very first time we start with spawn or wait */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bStartWithWait = true;

	/** Wait TimeTable[ActiveIndex], then a projectile is spawned and the index is increased */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	TArray<float> TimeTable;


	/**
	 *  If this has any element the spawner resets and stays deactivated if the player isn't in one of the whitelisted splines
	 *  Only works if it is always Active or if the state is serialized
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	TArray<TAssetPtr<ASoSpline>> WhiteListedSplinePtrList;

	/** weak hardware instead of switch now, but the name is kept because it is already used :( */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bUseWhitelistedSplinesOnlyOnSwitch = false;

	UPROPERTY(BlueprintReadOnly, Category = "SoProjectileSpawnerAutomata")
	TArray<ASoSpline*> WhiteListedSplineList;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bKillProjectilesOnWhitelistedSplinesLeft = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	bool bPreSimulateOnWhitelistedSplineEnterred = false;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	TSubclassOf<ASoProjectile> ProjectileClass;


	/** Used if >0, spawn request is ignored if this number matches the amount of projectiles alive */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	int32 MaxSpawnedProjectileNum = -1;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	bool bOverrideProjectileInitData = true;

	/** only used if bOverrideProjectileInitData is true */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	FSoProjectileInitData ProjectileInitData;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	bool bOverrideProjectileRuntimeData = true;

	/** only used if bOverrideProjectileRuntimeData is true */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	FSoProjectileRuntimeData ProjectileRuntimeData;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	bool bTeleportAbovePlayer = false;

	/** > 0 -> used, otherwise z is fixed */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	float TeleportAbovePlayerZDist = -1.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	AActor* TriggerOnFire = nullptr;

	/** Triggers with 1 if execution starts, 0 if stops (whitelisted splines + begin play) */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	AActor* TriggerOnStateChange = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnFire = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXProjectileImpactOverride = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileData")
	int32 TriggerOnFireValue = 1;


	/** if it is set to true the actor is aligned to spline and the projectile is spawned on it as well */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSplinity")
	bool bSpawnProjectileOnSpline = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSplinity")
	bool bUpdateSplineLocation = true;

	/** used only if bSpawnProjectileOnSpline */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSplinity")
	FSoSplinePointPtr SplineLocationPtr;

	/** used only if bSpawnProjectileOnSpline */
	UPROPERTY(BlueprintReadOnly, Category = "SoProjectileSplinity")
	FSoSplinePoint SplineLocation;


	/** If bLoop isn't set once the projectile is finished it will trigger all these targets */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileTriggerOnFinished")
	TArray<AActor*> TriggerTargetList;

	/** delay between last projectile spawned and targets are triggered */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileTriggerOnFinished")
	float TriggerDelay;

	/** All actor is triggered with this same value */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileTriggerOnFinished")
	FSoTriggerData TriggerData;



	int32 ActiveIndex = -1;

	/** has to be destroyed if >=0 */
	int32 TimerID = -1;

	UPROPERTY()
	TArray<ASoProjectile*> SpawnedProjectiles;

	UPROPERTY(EditAnywhere)
	FLinearColor SplineColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere)
	FLinearColor NonSplineColor = FLinearColor(0.0f, 2.0f, 1.0f, 1.0f);

	/** used to represent orientation, color tells if it is on spline or not */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileSpawnerAutomata")
	UArrowComponent* ArrowComponent;
};
