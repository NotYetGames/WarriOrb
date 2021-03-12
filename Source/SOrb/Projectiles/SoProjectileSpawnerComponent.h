// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Components/ArrowComponent.h"

#include "Projectiles/SoProjectile.h"
#include "Basic/SoEventHandler.h"

#include "SoProjectileSpawnerComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SORB_API USoProjectileSpawnerComponent : public UArrowComponent, public ISoEventHandler
{
	GENERATED_BODY()
public:
	USoProjectileSpawnerComponent();

	virtual void InitializeComponent() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void HandleSoPostLoad_Implementation() override { ClearSpawnedProjectiles(); }
	virtual void HandlePlayerRematerialize_Implementation() override { ClearSpawnedProjectiles(); }
	virtual void HandlePlayerRespawn_Implementation() override {};
	virtual void HandleWhitelistedSplinesEntered_Implementation() override {};
	virtual void HandleWhitelistedSplinesLeft_Implementation() override {};

	void SetProjectileClass(TSubclassOf<ASoProjectile> NewProjectileClass) { ProjectileClass = NewProjectileClass; }
	void SetSpawnProjectileOnSpline(bool bOnSpline) { bSpawnProjectileOnSpline = bOnSpline;	}

	UFUNCTION(BlueprintCallable, Category = "SoProjectileSpawner")
	ASoProjectile* SpawnProjectile();

	/** Non-spline projectile with custom class, location, orientation - and maybe custom InitData, if it is not nullptr */
	ASoProjectile* SpawnProjectileFromClass(const TSubclassOf<ASoProjectile>& ClassToSpawn,
											const FVector& Location,
											const FRotator& Orientation,
											const FSoProjectileInitData* InitDataOverride = nullptr);

	/** Non-spline projectile with custom class, location, and orientation - uses the projectile data for init/runtime, not the component one */
	UFUNCTION(BlueprintCallable, Category = "SoProjectileSpawner")
	ASoProjectile* SpawnFromProjectile(const TSubclassOf<ASoProjectile> ClassToSpawn,
									   const FVector& Location,
									   const FRotator& Orientation);

	/**
	 *  Projectile with custom class, location, and orientation - uses the projectile data for init/runtime, not the component one
	 *  Can be both Spline and Non-Spline projectile
	 */
	ASoProjectile* SpawnFromProjectileOnSpline(const TSubclassOf<ASoProjectile> ClassToSpawn,
											   const FVector& Location,
											   const FRotator& Orientation,
											   const FSoSplinePoint& SplineLocation);

	UFUNCTION()
	virtual void OnProjectileDied(ASoProjectile* Projectile);

	bool GetOverrideProjectileInitData() const { return bOverrideProjectileInitData; }
	const FSoProjectileInitData& GetProjectileInitData() const { return ProjectileInitData; }

	UFUNCTION(BlueprintPure, Category = "SoProjectileSpawner")
	bool HasActiveProjectile() const { return SpawnedProjectiles.Num() > 0; }


	/** Spawned Projectiles are deactivated and placed back to cache */
	UFUNCTION(BlueprintCallable)
	void ClearSpawnedProjectiles();

protected:

	ASoProjectile* SpawnProjectile_Internal(const FVector& Location, const FRotator& Rotation);
	ASoProjectile* SpawnProjectile_Internal(const FSoSplinePoint& SplineLocation,
											float ZValue,
											const FRotator& Rotation,
											const FVector& OffsetFromSpline);

protected:

	/** Called in BeginPlay  */
	virtual void HandleProjectileSpawnerBeginPlay();
	/** Called in EndPlay */
	virtual void HandleProjectileSpawnerEndPlay();

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	TSubclassOf<ASoProjectile> ProjectileClass;

	/**
	*  if it is set to true and the owner implements ISoSplineWalker interface the projectile is spawned on spline
	*  The projectile class must support splines in this case!!!
	*  Should not be changed runtime
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	bool bSpawnProjectileOnSpline = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	bool bOverrideProjectileInitData = false;

	/** only used if bOverrideProjectileInitData is true */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	FSoProjectileInitData ProjectileInitData;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	bool bOverrideProjectileRuntimeData = false;

	/** only used if bOverrideProjectileRuntimeData is true */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	FSoProjectileRuntimeData ProjectileRuntimeData;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	bool bClearParticlesOnPlayerRematerialize = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "SoProjectileSpawner")
	bool bClearParticlesOnReload = true;


	UPROPERTY()
	TArray<ASoProjectile*> SpawnedProjectiles;
};
