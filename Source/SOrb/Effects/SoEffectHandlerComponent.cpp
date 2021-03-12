// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEffectHandlerComponent.h"
#include "GameFramework/Actor.h"

#include "SoEffectBase.h"
#include "CharacterBase/SoMortal.h"
#include "Basic/SoGameInstance.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoEffectHandlerComponent::USoEffectHandlerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->GetClass()->ImplementsInterface(USoMortal::StaticClass()))
	{
		UE_LOG(LogSoEffectSystem, Error, TEXT("USoEffectHandlerComponent owner *has to* implement USoMortal!"));
		return;
	}

	OnOwnerDeathSubscription.BindUFunction(this, FName("OnOwnerDeath"));
	ISoMortal::Execute_SubscribeOnDied(GetOwner(), OnOwnerDeathSubscription, true);

	OnOwnerSpellsResetedSubscription.BindUFunction(this, FName("OnOwnerSpellsReset"));
	ISoMortal::Execute_SubscribeOnSpellsReset(GetOwner(), OnOwnerSpellsResetedSubscription, true);

	OnOwnerMeleeHitSubscription.BindUFunction(this, FName("OnOwnerMeleeHit"));
	ISoMortal::Execute_SubscribeOnMeleeHit(GetOwner(), OnOwnerMeleeHitSubscription, true);

	OnOwnerMeleeHitTakenSubscription.BindUFunction(this, FName("OnOwnerMeleeHitTaken"));
	ISoMortal::Execute_SubscribeOnMeleeHitTaken(GetOwner(), OnOwnerMeleeHitTakenSubscription, true);

	OnOwnerHitTakenSubscription.BindUFunction(this, FName("OnOwnerHitTaken"));
	ISoMortal::Execute_SubscribeOnHitTaken(GetOwner(), OnOwnerHitTakenSubscription, true);

	OnWalkThroughMortalListSubscription.BindUFunction(this, FName("OnWalkThroughMortal"));
	ISoMortal::Execute_SubscribeOnWalkThroughMortal(GetOwner(), OnWalkThroughMortalListSubscription, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ISoMortal::Execute_SubscribeOnDied(GetOwner(), OnOwnerDeathSubscription, false);
	ISoMortal::Execute_SubscribeOnSpellsReset(GetOwner(), OnOwnerSpellsResetedSubscription, false);
	ISoMortal::Execute_SubscribeOnMeleeHit(GetOwner(), OnOwnerMeleeHitSubscription, false);
	ISoMortal::Execute_SubscribeOnMeleeHitTaken(GetOwner(), OnOwnerMeleeHitTakenSubscription, false);
	ISoMortal::Execute_SubscribeOnHitTaken(GetOwner(), OnOwnerHitTakenSubscription, false);
	ISoMortal::Execute_SubscribeOnWalkThroughMortal(GetOwner(), OnWalkThroughMortalListSubscription, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Handler - TickComponent"), STAT_Handler_TickComponent, STATGROUP_SoEffect);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 Index = EffectsToTick.Num() - 1; Index >= 0; --Index)
		if (!EffectsToTick[Index]->Tick(DeltaTime))
			RemoveEffect_Internal(EffectsToTick[Index], false);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::AddEffect(const TSubclassOf<USoEffectBase>& EffectClass)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Handler - AddEffect"), STAT_Handler_AddEffect, STATGROUP_SoEffect);

	// only one timebased/counted effect can be used from the same type
	for (USoEffectBase* Effect : ActiveEffects)
		if (Effect->GetClass() == EffectClass &&
		   (Effect->GetEffectType() == ESoEffectType::EET_TimeBased || Effect->GetEffectType() == ESoEffectType::EET_Counted))
		{
			if (Effect->Reapply())
				return;
		}

	USoEffectBase* Effect = USoGameInstance::Get(GetOwner()).ClaimEffect(EffectClass);

	for (const auto& EffectClassToRemove : Effect->GetEffectsToRemoveOnApply())
		RemoveEffect(EffectClassToRemove);

	if (Effect->Apply(GetOwner()))
	{
		ActiveEffects.Add(Effect);

		if (Effect->ShouldReceiveTick())
			EffectsToTick.Add(Effect);

		if (Effect->ShouldReceiveNotifyOnMeleeHit())
			OnMeleeHitNotifyList.Add(Effect);

		if (Effect->ShouldReceiveNotifyOnMeleeHitTaken())
			OnMeleeHitTakenNotifyList.Add(Effect);

		if (Effect->ShouldReceiveNotifyOnHitTaken())
			OnHitTakenNotifyList.Add(Effect);

		if (Effect->ShouldReceiveNotifyOnWalkThroughEnemies())
			OnWalkThroughMortalList.Add(Effect);

		OnEffectStateChanged.Broadcast(Effect, true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::RemoveEffect(const TSubclassOf<USoEffectBase>& EffectClass)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Handler - RemoveEffect"), STAT_Handler_RemoveEffect, STATGROUP_SoEffect);

	for (int32 i = 0; i < ActiveEffects.Num(); ++i)
		if (ActiveEffects[i]->GetClass() == EffectClass)
		{
			RemoveEffect_Internal(ActiveEffects[i], true);
			return;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEffectHandlerComponent::HasEffect(const TSubclassOf<USoEffectBase>& EffectClass)
{
	for (int32 i = 0; i < ActiveEffects.Num(); ++i)
		if (ActiveEffects[i]->GetClass() == EffectClass)
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEffectBase* USoEffectHandlerComponent::GetEffect(const TSubclassOf<USoEffectBase>& EffectClass)
{
	for (int32 i = 0; i < ActiveEffects.Num(); ++i)
		if (ActiveEffects[i]->GetClass() == EffectClass)
			return ActiveEffects[i];

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::GetEffects(const TSubclassOf<USoEffectBase>& EffectClass, TArray<USoEffectBase*>& OutEffects)
{
	OutEffects.Empty();

	for (int32 i = 0; i < ActiveEffects.Num(); ++i)
		if (ActiveEffects[i]->GetClass() == EffectClass)
			OutEffects.Add(ActiveEffects[i]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::OnOwnerMeleeHit(AActor* Target, const FSoMeleeHitParam& Param)
{
	for (USoEffectBase* Effect : OnMeleeHitNotifyList)
		Effect->OnOwnerMeleeHit(Target, Param);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::OnOwnerMeleeHitTaken(AActor* Source, const FSoMeleeHitParam& Param)
{
	for (USoEffectBase* Effect : OnMeleeHitTakenNotifyList)
		Effect->OnOwnerMeleeHitTaken(Param);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::OnOwnerHitTaken(const FSoDmg& Dmg, const FSoHitReactDesc& Param)
{
	for (USoEffectBase* Effect : OnHitTakenNotifyList)
		Effect->OnOwnerHitTaken(Dmg, Param);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::OnOwnerDeath()
{
	// notify if necessary
	for (USoEffectBase* Effect : ActiveEffects)
		if (Effect->ShouldReceiveNotifyOnDeath())
			Effect->OnOwnerDeath();

	// remove if necessary
	for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
		if (ActiveEffects[Index]->ShouldBeDestroyedOnDeath())
			RemoveEffect_Internal(ActiveEffects[Index], true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::OnOwnerSpellsReset()
{
	// remove if necessary
	for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
		if (ActiveEffects[Index]->ShouldBeDestroyedOnSpellReset())
			RemoveEffect_Internal(ActiveEffects[Index], true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::OnWalkThroughMortal(AActor* Mortal)
{
	for (USoEffectBase* Effect : OnWalkThroughMortalList)
		Effect->OnWalkThroughMortal(Mortal);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEffectHandlerComponent::RemoveEffect_Internal(USoEffectBase* Effect, bool bInterrupt)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Handler - RemoveEffect_Internal"), STAT_Handler_RemoveEffect_Internal, STATGROUP_SoEffect);

	if (bInterrupt)
		Effect->Interrupt();

	OnEffectStateChanged.Broadcast(Effect, false);

	ActiveEffects.Remove(Effect);
	EffectsToTick.Remove(Effect);
	OnMeleeHitNotifyList.Remove(Effect);
	OnMeleeHitTakenNotifyList.Remove(Effect);
	OnHitTakenNotifyList.Remove(Effect);
	OnWalkThroughMortalList.Remove(Effect);

	USoGameInstance::Get(GetOwner()).ReturnEffect(Effect);
}
