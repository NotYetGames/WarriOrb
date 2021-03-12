// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "CharacterBase/SoMortal.h"
#include "SoEffectHandlerComponent.generated.h"


class USoEffectBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoEffectStateChanged, USoEffectBase*, Effect, bool, bActivated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SORB_API USoEffectHandlerComponent : public UActorComponent
{
GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoEffectHandlerComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	virtual void AddEffect(const TSubclassOf<USoEffectBase>& EffectClass);
	virtual void RemoveEffect(const TSubclassOf<USoEffectBase>& EffectClass);

	UFUNCTION(BlueprintPure)
	bool HasEffect(const TSubclassOf<USoEffectBase>& EffectClass);

	UFUNCTION(BlueprintPure)
	USoEffectBase* GetEffect(const TSubclassOf<USoEffectBase>& EffectClass);

	UFUNCTION(BlueprintPure)
	void GetEffects(const TSubclassOf<USoEffectBase>& EffectClass, TArray<USoEffectBase*>& OutEffects);

	UFUNCTION()
	void OnOwnerDeath();

	UFUNCTION()
	void OnOwnerMeleeHit(AActor* Target, const FSoMeleeHitParam& Param);

	UFUNCTION()
	void OnOwnerMeleeHitTaken(AActor* Source, const FSoMeleeHitParam& Param);

	UFUNCTION()
	void OnOwnerHitTaken(const FSoDmg& Dmg, const FSoHitReactDesc& Param);

	UFUNCTION()
	void OnOwnerSpellsReset();

	UFUNCTION()
	void OnWalkThroughMortal(AActor* Mortal);

protected:

	void RemoveEffect_Internal(USoEffectBase* Effect, bool bInterrupt);

protected:

	UPROPERTY(BlueprintAssignable)
	FSoEffectStateChanged OnEffectStateChanged;

	/** container containing all active effect */
	UPROPERTY()
	TArray<USoEffectBase*> ActiveEffects;


	/** container containing the effects the component has to tick */
	UPROPERTY()
	TArray<USoEffectBase*> EffectsToTick;

	/** container containing the effects the component has to notify on melee hit */
	UPROPERTY()
	TArray<USoEffectBase*> OnMeleeHitNotifyList;

	/** container containing the effects the component has to notify on melee hit taken */
	UPROPERTY()
	TArray<USoEffectBase*> OnMeleeHitTakenNotifyList;

	UPROPERTY()
	TArray<USoEffectBase*> OnHitTakenNotifyList;

	UPROPERTY()
	TArray<USoEffectBase*> OnWalkThroughMortalList;

private:
	FSoOnMortalNoParam OnOwnerDeathSubscription;
	FSoOnMortalNoParam OnOwnerSpellsResetedSubscription;
	FSoOnMeleeHit OnOwnerMeleeHitSubscription;
	FSoOnMeleeHit OnOwnerMeleeHitTakenSubscription;
	FSoOnHit OnOwnerHitTakenSubscription;
	FSoNotifyActorSingle OnWalkThroughMortalListSubscription;
};
