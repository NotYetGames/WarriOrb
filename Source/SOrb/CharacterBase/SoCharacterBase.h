// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

// interface
#include "CoreMinimal.h"

#include "GameFramework/Character.h"

#include "SoIMortalTypes.h"
#include "CharacterBase/SoMortal.h"
#include "SplineLogic/SoSplineWalker.h"

#include "SoCharacterBase.generated.h"

class USoEffectHandlerComponent;
class USoEffectBase;
class ASoPlayerSpline;
class UParticleSystemComponent;
class UFMODEvent;
class USoProjectileSpawnerComponent;
class USoCharacterMovementComponent;
class USoCharacterSheet;
struct FSoSplinePoint;
struct FSoItem;


DECLARE_STATS_GROUP(TEXT("SoCharacter"), STATGROUP_SoCharacter, STATCAT_Advanced);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Common parent class for the player character and for the spline based AI
 */
UCLASS(Abstract)
class SORB_API ASoCharacterBase : public ACharacter, public ISoMortal, public ISoSplineWalker
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoCharacterBase(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	// So interfaces:

	// mortal
	virtual bool IsAlive_Implementation() const override { return false; }
	virtual void Kill_Implementation(bool bPhysical) override {};
	virtual bool NeedsHeal_Implementation() const override;
	virtual bool HasInvulnerability_Implementation() const override { return false; }
	virtual bool IsBounceAble_Implementation() const override { return false; }

	virtual bool MeleeHit_Implementation(const FSoMeleeHitParam& HitParam) override { return false; }

	virtual void ApplyEffect_Implementation(TSubclassOf<USoEffectBase> EffectClass, bool bApply) override;

	virtual void Heal_Implementation(bool bAll, float Delta) override;

	virtual bool SubscribeOnDied_Implementation(const FSoOnMortalNoParam& OnDeath, bool bSubscribe = true) override;
	virtual bool SubscribeOnSpellsReset_Implementation(const FSoOnMortalNoParam& OnDeath, bool bSubscribe = true) override;
	virtual bool SubscribeOnMeleeHit_Implementation(const FSoOnMeleeHit& InOnMeleeHit, bool bSubscribe = true) override;
	virtual bool SubscribeOnMeleeHitTaken_Implementation(const FSoOnMeleeHit& InOnMeleeHit, bool bSubscribe = true) override;
	virtual bool SubscribeOnHitTaken_Implementation(const FSoOnHit& InOnHit, bool bSubscribe = true) override;
	virtual bool SubscribeOnWalkThroughMortal_Implementation(const FSoNotifyActorSingle& OnWalkThrough, bool bSubscribe = true) override { return true; }


	void OnStatusEffectChanged_Implementation(ESoStatusEffect Effect, bool bGained);
	void DisplayVisualEffect_Implementation(ESoVisualEffect Effect);

	virtual FVector GetEffectAttachLocation_Implementation() override;


	// SoSplineWalker
	FSoSplinePoint GetSplineLocationI_Implementation() const override;
	void SetSplineLocation_Implementation(const FSoSplinePoint& SplinePoint, bool bUpdateOrientation) override;
	void OnPushed_Implementation(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage) override {};


	// <Movement>
	// used by SoCharacter only

	// used by SoCharacter for camera/input direction handling
	virtual void OnSplineChanged(const FSoSplinePoint& OldLocation, const FSoSplinePoint& NewLocation) {};

	virtual bool ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult = nullptr) const { return false; }
	// true: land should go on, false: character is bounced back
	virtual bool OnPreLanded(const FHitResult& Hit) { return true; }

	// nope nope nope - those are player related stuffs, only SoCharacter implements it (which is the player char only), everyone else should never try those
	// can be Check(false) because ShouldBounceOnHit() should stop the function from being executed if it is irrelevant anyway
	virtual void OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal) { check(false); }
	// return true if character can perform a wall jump right now
	virtual bool CanPerformWallJump(const FHitResult& HitResult) { ensure(false); return false; }
	// </Movement>


	// SoCombatComponent calls on the owner if the owner's strike is blocked
	virtual void OnBlocked() {}


	void SetRootMotionDesc(const FSoRootMotionDesc& Desc);
	void ClearRootMotionDesc();

	// TODO: is it used at all?
	UFUNCTION(BlueprintCallable, Category = SplineLocation)
	void ChangeSpline(ASoPlayerSpline* NewSpline);

	USoCharacterMovementComponent* GetSoMovement() { return SoMovement; }
	USoCharacterMovementComponent* GetSoMovement() const { return SoMovement; }

	void ApplyStaticEffects(const FSoItem& Item, bool bApply);

	USoProjectileSpawnerComponent* GetProjectileSpawner() { return ProjectileSpawner; }
	const USoProjectileSpawnerComponent* GetProjectileSpawner() const { return ProjectileSpawner; }

	UFUNCTION(BlueprintCallable, Category = Misc)
	void PlayMaterialAnimation(int32 Index, bool bForward = true, float Speed = 1.0f);

	UFUNCTION(BlueprintCallable, Category = Misc)
	void StopMaterialAnimation(int32 Index);

	UFUNCTION(BlueprintCallable, Category = Misc)
	void StopAllMaterialAnimations();

	UFUNCTION(BlueprintCallable, Category = Misc)
	void SetScalarOnParticleMaterials(UParticleSystemComponent* PS, FName ParamName, float Value);

	void UpdateMaterialAnimations(float DeltaSeconds);

protected:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = General)
	void OnStatusEffect(ESoStatusEffect Effect, bool bGained);

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnDisplayVisualEffect(ESoVisualEffect Effect);

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnMaterialAnimationFinished(int32 MaterialAnimationIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = General)
	void OnMaterialAnimationInterrupted(int32 MaterialAnimationIndex);

protected:

	// Components:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoProjectileSpawnerComponent* ProjectileSpawner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement)
	USoCharacterMovementComponent* SoMovement;

	/** Name of the Character Sheet component. Use this name if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass). */
	static FName CharacterSheetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	USoCharacterSheet* SoCharacterSheet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EffectSystem)
	USoEffectHandlerComponent* SoEffectHandler;


	FSoOnMortalNoParamMulti OnMortalDeath;

	FSoOnMortalNoParamMulti OnSpellsReseted;

	/** event fired when the character hits something with a melee attack */
	FSoOnMeleeHitMulti OnMeleeHit;

	FSoOnMeleeHitMulti OnMeleeHitSuffered;

	FSoOnHitMulti OnHitSuffered;


	friend class USoCombatComponent;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXFOnHeal = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	TArray<FSoMaterialAnimationEntry> MaterialAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCanBeSlowed = true;
};
