// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"

#include "CharacterBase/SoIMortalTypes.h"
#include "SoProjectileTypes.h"

#include "SoProjectile.generated.h"

class ASoProjectile;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoProjectileNotify, ASoProjectile*, Projectile);

DECLARE_STATS_GROUP(TEXT("SoProjectile"), STATGROUP_SoProjectile, STATCAT_Advanced);


class UFMODEvent;
class UParticleSystem;
class UProjectileMovementComponent;

UCLASS()
class SORB_API ASoProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASoProjectile();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/**
	 * Activates the projectile: collision and rendering is enabled, transformation and velocity is reinitialized
	 *
	 * @param InitialLocation 	Initial location the projectile is spawned at
	 * @param InitialRotation 	Orientation of projectile, velocity is relative to this
	 * @param InitData 			Data from spawner used to initialize the projectile
	 * @param RuntimeData		Data the projectile will need while its active. The passed value must be valid while the projectile is active!
	 * @param Spawner			The spawner actor, he won't be effected by the projectile for a while
	 */
	void ActivateProjectile(const FVector& InitialLocation,
							const FRotator& InitialRotation,
							const FSoProjectileInitData* InitDataOverride = nullptr,
							const FSoProjectileRuntimeData* RuntimeData = nullptr,
							AActor* Spawner = nullptr);

	/**
	 * Like above, but on spline. Projectile is expected to be spline based
	 *
	 * @param OffsetFromSpline 	Fix offset from the real spline location, it is interpolated to FVector(0, 0, 0)
	 * @param RuntimeData		Data the projectile will need while its active. The passed value must be valid while the projectile is active!
	 * @param InitDataOverride	Data from spawner used to initialize the projectile
	 * @param Spawner			The spawner actor, he won't be effected by the projectile for a while
	 */
	void ActivateProjectileOnSpline(const struct FSoSplinePoint& SplineLocation,
									float LocationZ,
									const FVector& OffsetFromSpline,
									const FRotator& InitialRotation,
									const FSoProjectileInitData* InitDataOverride = nullptr,
									const FSoProjectileRuntimeData* RuntimeData = nullptr,
									AActor* Spawner = nullptr);

	///**
	// * Like above, but on spline, initialized with 3D velocity
	// *
	// * @param OffsetFromSpline 	Fix offset from the real spline location, it is interpolated to FVector(0, 0, 0)
	// * @param RuntimeData		Data the projectile will need while its active. The passed value must be valid while the projectile is active!
	// * @param InitDataOverride	Data from spawner used to initialize the projectile
	// * @param Spawner			The spawner actor, he won't be effected by the projectile for a while
	// */
	//void ActivateProjectileOnSpline(const struct FSoSplinePoint& SplineLocation,
	//								const FVector& Location,
	//								const FVector& OffsetFromSpline,
	//								const FRotator& InitialRotation,
	//								const FSoProjectileInitData* InitDataOverride = nullptr,
	//								const FSoProjectileRuntimeData* RuntimeData = nullptr,
	//								AActor* Spawner = nullptr);

	/**
	 * Deactivates the projectile: collision and rendering is disabled, stored runtime data is cleared
	 * It does not "kill" the projectile with fadeout, it is instantly turned off
	 * Must be called only if the projectile is stopped from outside, projectile calls it before OnDeathNotify is fired
	 */
	void DeactivateProjectile();

	/** Velocity is changed to FVector(0, 0, |Velocity|) */
	void ForceVelocityToZ();


	/**
	 *  Called if the projectile became inactive, spawner is supposed to place it back to the pool
	 *  Projectile can't do it itself, because the spawner has to keep a list of projectiles and that list must be updated as well
	 *  It is a must, because we must be able to reset any projectile e.g. on reload
	 */
	UPROPERTY(BlueprintAssignable)
	FSoProjectileNotify OnDeathNotify;

	bool IsSplineProjectile() const;

	const FSoProjectileInitData& GetDefaultInitData() const { return DefaultInitData; }

	UFUNCTION(BlueprintPure, Category = Projectile)
	bool HasDamage() const;

	UFUNCTION(BlueprintNativeEvent)
	void ForceUpdate(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void SetSFXOverride(UFMODEvent* SFX) { SFXOverride = SFX; }

	/** Projectile template dmg is reset to class default on activate */
	UFUNCTION(BlueprintCallable)
	void OverrideDamage(const FSoDmg& Dmg) { DefaultRuntimeData.Damage = Dmg; }

	UFUNCTION(BlueprintPure, Category = ">Projectile")
	FSoDmg GetDamage() { return DefaultRuntimeData.Damage; }

protected:

	/**
	 * Called when the projectile has to die, default implementation simply deactivates the projectile via calling OnProjectileDestroyed()
	 * OnProjectileDestroyed() must be called in custom implementation as well (can be delayed)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SoProjectile")
	void DestroyProjectile();

	/** must be called if the projectile dies in a "natural" way, fires death notify and deactivates projectile */
	UFUNCTION(BlueprintCallable, Category = "SoProjectile")
	void OnProjectileDestroyed();

	/** disable all c++ timers on the projectile because it is already being destroyed */
	UFUNCTION(BlueprintCallable, Category = "SoProjectile")
	void OnDestroyStartDisableTimers();


	/** Called after the projectile is activated */
	UFUNCTION(BlueprintImplementableEvent, Category = "SoProjectile")
	void OnProjectileActivatedBP();

	/** Called before the projectile is deactivated */
	UFUNCTION(BlueprintImplementableEvent, Category = "SoProjectile")
	void OnProjectileDeactivatedBP();


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnTargetHit(AActor* OtherActor, const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent)
	void OnTargetHitReallyHappened(AActor* OtherActor, const FHitResult& SweepResult);



	/** Called when the PrimitiveComponent overlaps */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/** Called when the PrimitiveComponent collides */
	UFUNCTION()
	void OnHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** should be only called if CollisionActiveDistanceFromPlayer is set to a sane value */
	UFUNCTION()
	void UpdateCollision();

	UFUNCTION(BlueprintImplementableEvent)
	void OnCollisionUpdatedBP(bool bEnabled);

	UFUNCTION(BlueprintImplementableEvent)
	void OnHitBP(const FHitResult& Hit, bool bStillAlive);

private:

	/**
	* Activates the projectile: collision and rendering is enabled
	*
	* @param InitData 			Data from spawner used to initialize the projectile
	* @param RuntimeData		Data the projectile will need while its active. The passed value must be valid while the projectile is active!
	* @param Spawner			The spawner actor, he won't be effected by the projectile for a while
	*/
	void ActivateProjectile_Internal(const FSoProjectileInitData& InitData,
									 const FSoProjectileRuntimeData& RuntimeData,
									 AActor* Spawner);

protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	bool bEnableCollisionOnActivate = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	bool bEnableMovementOnActivate = true;

	/** Used if the spawner does not want to override it */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	FSoProjectileInitData DefaultInitData;

	/** Used if the spawner does not want to override it */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	FSoProjectileRuntimeData DefaultRuntimeData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	bool bCanUseDamageOverride = true;

	/** weather the projectile is destroyed or not if it hits a mortal */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	bool bKillProjectileOnTargetHit = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	bool bHitOneActorOnlyOnce = true;

	UPROPERTY(BlueprintReadWrite, Category = "SoProjectileData")
	bool bCurrentKillProjectileOnTargetHit = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	bool bOnlyHitAliveTargets = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoProjectileData")
	bool bReduceDmgAgainstNotPlayer = true;


	/** must be created in Blueprint and initialized in BP BeginPlay(), but optional, simply ignored if not set */
	UPROPERTY(BlueprintReadWrite)
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	/** must be created in Blueprint and initialized in BP BeginPlay() */
	UPROPERTY(BlueprintReadWrite)
	UPrimitiveComponent* PrimitiveComponent;

	const FSoProjectileRuntimeData* RuntimeDataPtr = nullptr;

	UPROPERTY(EditAnywhere)
	float ZSpawnOffset = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TEnumAsByte<ECollisionEnabled::Type> PrimitiveComponentCollisionType = ECollisionEnabled::QueryAndPhysics;

	UPROPERTY(BlueprintReadWrite)
	AActor* SpawnerActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoHitReactDesc HitReactDesc;

	UPROPERTY(BlueprintReadWrite)
	float RemainingHitCount = -1;

	UPROPERTY(BlueprintReadWrite)
	bool bDoNotDamageSpawner = false;

	UPROPERTY(BlueprintReadWrite)
	TSet<AActor*> ActorsAlreadyHit;

	//
	// SFX
	//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnTargetHit = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnTargetDeath = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnEnemyTargetHit = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SFX)
	UFMODEvent* SFXOnEnemyTargetHit2D = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = SFX)
	UFMODEvent* SFXOverride = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = VFX)
	UParticleSystem* VFXOnTargetHit = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = VFX)
	bool bAttachImpactToTarget = false;

	/** effect applied to target - so he can hanle it instead of this actor */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = VFX)
	ESoVisualEffect EffectToApplyOnTarget = ESoVisualEffect::EVE_MAX;

	/** used with OnLifetimeOver() to deactivate projectile after its lifetime is expired, must be reset if the projectile is deactivated before */
	FTimerHandle LifeTimer;

	/** used to save spawner from instant suicide on projectile spawn */
	FTimerHandle SpawnerImmunityTimer;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = PerfTrick)
	float CollisionRefreshFrequency = -1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = PerfTrick)
	float CollisionActiveDistanceFromPlayerSquered = -1.0f;

	bool bCollisionActiveOnPrimitiveComponent = true;

	/**
	 *  Used if CollisionRefreshFrequency > 0 and CollisionActiveDistanceFromPlayer > 0
	 *  We only enable collision on the primitive component if the player is in CollisionActiveDistanceFromPlayer
	 */
	FTimerHandle CollisionRefreshTimer;
};
