// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CharacterBase/SoIMortalTypes.h"
#include "UObject/Interface.h"
#include "SoMortal.generated.h"

DECLARE_DYNAMIC_DELEGATE(FSoOnMortalNoParam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoOnMortalNoParamMulti);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FSoOnMeleeHit, AActor*, Actor, const FSoMeleeHitParam&, Param);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoOnMeleeHitMulti, AActor*, Actor, const FSoMeleeHitParam&, Param);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FSoOnHit, const FSoDmg&, Dmg, const FSoHitReactDesc&, Param);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoOnHitMulti, const FSoDmg&, Dmg, const FSoHitReactDesc&, Param);

DECLARE_DYNAMIC_DELEGATE_OneParam(FSoNotifyActorSingle, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoNotifyActor, AActor*, Actor);

/**
*
*/
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoMortal : public UInterface
{
	GENERATED_UINTERFACE_BODY()

};


/**
 * interface for damageable entities
 * both C++ and Blueprint classes can implement and use the interface
 */
class SORB_API ISoMortal
{
	GENERATED_IINTERFACE_BODY()


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	bool IsAlive() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	bool NeedsHeal() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	bool HasInvulnerability() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	bool IsBounceAble() const;

	/**
	 * Attack tries to simply destroy target no matter what
	 *
	 * @param	bPhysical			Damage type for statistics/log
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	void Kill(bool bPhysical);


	/**
	 * Causes some damage with different damage types
	 * Resistance/immunity may still applied to the input by the receiver
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	void CauseDmg(const FSoDmg& Dmg, const FSoHitReactDesc& HitReactDesc);

	/**
	 * Causes some damage with different damage types
	 * Resistance/immunity may still applied to the input by the receiver
	 *
	 * @param	HitParam		struct describing the hit params, may change in the future
	 * @return	true: successful hit, false: blocked (it may or may not interrupt the attacker)
	 */
	// return value ? hit : blocked
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	bool MeleeHit(const FSoMeleeHitParam& HitParam);

	/**
	 * Restores health
	 *
	 * @param	bAll	True -> HP is restored to max
	 * @param	fDelta	!bAll -> HP is increased with fDelta (can't be more than max)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Health)
	void Heal(bool bAll = true, float Delta = 0.0f);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Effect)
	void ApplyEffect(TSubclassOf<USoEffectBase> EffectClass, bool bApply = true);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = MortalEvent)
	bool SubscribeOnDied(const FSoOnMortalNoParam& OnDeath, bool bSubscribe = true);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = MortalEvent)
	bool SubscribeOnSpellsReset(const FSoOnMortalNoParam& OnSpellsReset, bool bSubscribe = true);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = MortalEvent)
	bool SubscribeOnMeleeHit(const FSoOnMeleeHit& OnMeleeHit, bool bSubscribe = true);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = MortalEvent)
	bool SubscribeOnMeleeHitTaken(const FSoOnMeleeHit& OnMeleeHit, bool bSubscribe = true);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = MortalEvent)
	bool SubscribeOnHitTaken(const FSoOnHit& OnHit, bool bSubscribe = true);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = MortalEvent)
	bool SubscribeOnWalkThroughMortal(const FSoNotifyActorSingle& OnWalkThrough, bool bSubscribe = true);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = VisualStuff)
	void OnStatusEffectChanged(ESoStatusEffect Effect, bool bGained);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = VisualStuff)
	void DisplayVisualEffect(ESoVisualEffect Effect);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = VisualStuff)
	FVector GetEffectAttachLocation();
};
