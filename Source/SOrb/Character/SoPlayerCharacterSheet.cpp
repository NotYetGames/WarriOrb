// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoPlayerCharacterSheet.h"


#include "EngineUtils.h"
#include "Engine/ObjectLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"

#include "Items/SoInventoryComponent.h"
#include "Items/SoItem.h"
#include "Items/ItemTemplates/SoItemTemplateShard.h"
#include "Items/ItemTemplates/SoUsableItemTemplate.h"
#include "Items/ItemTemplates/SoQuestItemTemplate.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"

#include "Interactables/SoInteractableActor.h"

#include "Character/SoCharStates/SoADefault.h"
#include "Character/SoCharStates/SoALillian.h"
#include "Character/SoCharStates/SoAInUI.h"
#include "Character/SoCharStates/SoATeleport.h"
#include "Character/SoPlayerProgress.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SoCharacterStrike.h"
#include "SplineLogic/SoMarker.h"

#include "Basic/SoGameMode.h"
#include "Basic/SoGameSingleton.h"
#include "SaveFiles/SoWorldState.h"

#include "Levels/SoLevelHelper.h"
#include "Basic/SoGameInstance.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Materials/MaterialInstance.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "UI/SoUIHelper.h"

#include "SoLocalization.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoPlayerCharacterSheet, All, All)

const FName USoPlayerCharacterSheet::HealthUpgradeName = TEXT("HealthUpgrade");

const FName USoPlayerCharacterSheet::SKSplineName = FName("SoulKeeperSpline");
const FName USoPlayerCharacterSheet::SKPenaltyName = FName("SoulKeeperPenalty");
const FName USoPlayerCharacterSheet::SKDistanceOnSplineName = FName("SoulKeeperDistanceOnSpline");
const FName USoPlayerCharacterSheet::SKZName = FName("SoulKeeperZ");
const FName USoPlayerCharacterSheet::SKDirectionName = FName("SoulKeeperDirection");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoPlayerCharacterSheet::USoPlayerCharacterSheet()
{
	bWantsInitializeComponent = true;

	CurrentEquipment.SetNum(static_cast<int32>(ESoItemSlot::EIS_Max));

	SaneHPMultiplier = 1.0f;
	InsaneHPMultiplier = 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::InitializeComponent()
{
	Super::InitializeComponent();

	SoOwner = Cast<ASoCharacter>(GetOwner());
	check(SoOwner);

	OnSlotChanged.AddDynamic(this, &USoPlayerCharacterSheet::OnSlotChange);

	if (ASoGameMode* GameMode = ASoGameMode::GetInstance(this))
	{
		GameMode->OnPreSave.AddDynamic(this, &USoPlayerCharacterSheet::OnSave);
		GameMode->OnPostLoad.AddDynamic(this, &USoPlayerCharacterSheet::OnReload);
	}

	// only used in editor, modify ASoCharacter::EnsureSpellsCapacityLimits() to modify the default capacity value
	SoOwner->EnsureSpellsCapacityLimits();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::UninitializeComponent()
{
	if (ASoGameMode* GameMode = ASoGameMode::GetInstance(this))
	{
		GameMode->OnPreSave.RemoveDynamic(this, &USoPlayerCharacterSheet::OnSave);
		GameMode->OnPostLoad.RemoveDynamic(this, &USoPlayerCharacterSheet::OnReload);
	}

	Super::UninitializeComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnSave()
{
	if (SoOwner->IsSoulkeeperActive() && SoOwner->bCanUseSoulkeeper)
	{
		SetNameValue(SKSplineName, SoOwner->ResLocation.GetSpline() ? SoOwner->ResLocation.GetSpline()->GetSplineFName() : NAME_None);
		SetIntValue(SKPenaltyName, SoulkeeperRespawnPenalty);
		SetFloatValue(SKDistanceOnSplineName, SoOwner->ResLocation.GetDistance());
		SetFloatValue(SKZName, SoOwner->ResLocationZValue);

		const FVector SKSplineLocation = SoOwner->ResLocation.ToVector(SoOwner->ResLocationZValue);
		const FVector SKLocation = SoOwner->ResRuneVisualActor->GetActorLocation();
		const FVector SKDir = (SKLocation - SKSplineLocation).GetSafeNormal();
		const FVector NormalDir = SoOwner->ResLocation.GetPlaneNormal();
		SetIntValue(SKDirectionName, (SKDir | NormalDir) > 0.0f ? 1 : -1);
	}
	else
	{
		SetNameValue(SKSplineName, NAME_None);
	}

	FSoWorldState::Get().SetCheckpointLocation(SoOwner->GetActiveCheckpointLocationName());
	FSoWorldState::Get().SetMapName(USoLevelHelper::GetChapterNameFromActor(SoOwner));

	// Player data
	FSoWorldState::Get().SetPlayerData(PlayerData);

	// Inventory
	SoOwner->GetInventory()->Save();

	// Current equipment
	SaveEquipment();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnReload()
{
	if (SoOwner->SoActivity != nullptr)
		SoOwner->SoActivity->SwitchActivity(SoOwner->SoADefault);

	// clear old items
	for (int32 i = 0; i < static_cast<int32>(ESoItemSlot::EIS_Max); ++i)
		if (CurrentEquipment[i].Template != nullptr)
			OnItemEquiped(static_cast<ESoItemSlot>(i), CurrentEquipment[i], false);

	// NOTE: we do not need to set any checkpoint as the current world state stores that
	// NOTE: NAME_None == GetDefaultCheckpointLocationName()
	auto& GameInstance = USoGameInstance::Get(this);
	PlayerData = FSoWorldState::Get().GetPlayerData();

	// INVENTORY FIRST!
	SoOwner->GetInventory()->Reload();

	AttackSpeedMultiplier = 1.0f;
	DamageModifiers = 1.0f;

	// Current equipment
	LoadEquipment();

	const float InitialMaxHealth = FSoWorldState::Get().GetInitialMaxHealth();;
	const float MaxHealth = InitialMaxHealth + HealthUpgradeValue * GetIntValue(HealthUpgradeName);
	SetMaxHealth(MaxHealth);

	// Reload the location in non editor builds
	// NOTE: Same method is called in ASoCharacter::BeginPlay.
	// This is useful when a new save is loaded
	// TODO: wtf is this shit? maybe on reload in the same map?
	bool bTeleportedToSK = false;
#if WARRIORB_WITH_EDITOR
	// Editor
	if (GameInstance.IsEpisode())
	{
		SoOwner->TeleportToActiveCheckpointName();
	}
#else
	// Non Editor


	const FName SplineName = GetNameValue(SKSplineName);
	if (SplineName != NAME_None)
	{
		for (TActorIterator<ASoPlayerSpline> ActorItr(SoOwner->GetWorld()); ActorItr; ++ActorItr)
		{
			ASoPlayerSpline* SoSpline = *ActorItr;
			if (SoSpline->GetSplineFName() == SplineName)
			{
				bTeleportedToSK = true;

				SoulkeeperRespawnPenalty = GetIntValue(SKPenaltyName);
				SetHealth(FMath::Max(MaxHealthPoints - SoulkeeperRespawnPenalty, MaxHealthPoints * 0.3f));
				SoOwner->ResLocationZValue = GetFloatValue(SKZName);
				SoOwner->ResLocation = FSoSplinePoint(SoSpline, GetFloatValue(SKDistanceOnSplineName));

				const FVector LocOnSpline = SoOwner->ResLocation.ToVector(SoOwner->ResLocationZValue - SoOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				const FVector TotemLocation = LocOnSpline + SoOwner->ResLocation.GetPlaneNormal() * GetIntValue(SKDirectionName) * 100;
				SoOwner->ResRuneVisualActor->SetActorLocation(TotemLocation);
				SoOwner->ResRuneVisualActor->SetActorRotation((LocOnSpline - TotemLocation).Rotation());
				SoOwner->ResRuneVisualActor->SetActorHiddenInGame(false);
				SoOwner->ResRuneVisualActor->Activate();
				SoOwner->OnSoulkeeperPlaced.Broadcast();

				SoOwner->SoATeleport->SetupTeleport(SoOwner->ResLocation, SoOwner->ResLocationZValue, false, false);

				SetSpellUsageCount(SoOwner->SoulKeeperSpell, 0, false);

				break;
			}
		}
	}

	if (!bTeleportedToSK)
	{
		SoOwner->TeleportToActiveCheckpointName();
	}
#endif // WARRIORB_WITH_EDITOR

	// Set initial spells capacity
	SoOwner->EnsureSpellsCapacityLimits(FSoWorldState::Get().GetInitialSpellsCapacity());

	UpdateDisplayName();

	// add/remove status effect if necessary
	SoOwner->UpdateMaterialBP();

	static const FName WorthyName = FName("bWorthy");
	ISoMortal::Execute_OnStatusEffectChanged(SoOwner, ESoStatusEffect::ESE_Worthy, GetBoolValue(WorthyName));

	if (SoOwner->IsSoulkeeperActive() && !bTeleportedToSK)
		SoOwner->PickupResRune();


	if (GetBoolValue(USoStaticHelper::GetLillianFormName()))
	{
		SoOwner->SoActivity->SwitchActivity(SoOwner->SoALillian);
		SoOwner->ChangeUIVisibility.Broadcast(false);
	}
	else if (SoOwner->SoActivity == SoOwner->SoALillian)
		SoOwner->SoActivity->SwitchActivity(SoOwner->SoADefault);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnDifficultyChanged(ESoDifficulty NewDifficulty)
{
	if (SoOwner->GetInventory()->HasItem(SpellCasterTemplate) ||
		CurrentEquipment[static_cast<int32>(ESoItemSlot::EIS_Item0)].Template == SpellCasterTemplate ||
		CurrentEquipment[static_cast<int32>(ESoItemSlot::EIS_Item1)].Template == SpellCasterTemplate)
	{
		// only do anything if the spellcaster is active

		switch (NewDifficulty)
		{
			case ESoDifficulty::Sane:
				if (!ReplaceRuneStone(SoulStoneTemplateIntended, SoulStoneTemplateSane) &&
					!SoOwner->GetInventory()->HasItem(SoulStoneTemplateSane))
				{
					FSoItem Item;
					Item.Template = SoulStoneTemplateSane;
					SoOwner->AddItem(Item, false);
					// TODO: equip newly added runestone if there is a slot for it
				}
				break;

			case ESoDifficulty::Intended:
				if (!ReplaceRuneStone(SoulStoneTemplateSane, SoulStoneTemplateIntended) &&
					!SoOwner->GetInventory()->HasItem(SoulStoneTemplateIntended))
				{
					FSoItem Item;
					Item.Template = SoulStoneTemplateIntended;
					SoOwner->AddItem(Item, false);
					// TODO: equip newly added runestone if there is a slot for it
				}
				break;

			case ESoDifficulty::Insane:
				SoOwner->RemoveItem(SoulStoneTemplateIntended);
				SoOwner->RemoveItem(SoulStoneTemplateSane);
				for (int32 i = EquippedRuneStones.Num() - 1; i >= 0; --i)
					if (EquippedRuneStones[i].Template == SoulStoneTemplateIntended ||
						EquippedRuneStones[i].Template == SoulStoneTemplateSane)
					{
						EquippedRuneStones.RemoveAt(i);
						if (ActiveEquippedRuneStoneIndex >= i && ActiveEquippedRuneStoneIndex > 0)
							ActiveEquippedRuneStoneIndex -= 1;
					}
				break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnRevive(bool bSoulKeeperNotUsed)
{
	ClearBonusHealth();

	if (bSoulKeeperNotUsed)
	{
		RestoreHealth();
		ResetSoulkeeperRespawnPenalty();
		RestoreSpells();
	}
	else
	{
		const float Penalty = IncreaseSoulkeeperRespawnPenalty();
		SetHealth(FMath::Max(MaxHealthPoints - Penalty, MaxHealthPoints * 0.3f));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::Heal()
{
	RestoreHealth();
	SoulkeeperRespawnPenalty = 0;
	RestoreSpells();
	ClearBonusHealth();
	SoOwner->PlayHealLightEffectBP();
	SoOwner->RemoveAllCooldown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::ApplyHealthUpgrade()
{
	SetIntValue(HealthUpgradeName, 1, true);
	SetMaxHealth(GetMaxHealth() + HealthUpgradeValue, false);
	IncreaseHealth(HealthUpgradeValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::AddSoulStone(USoItemTemplateRuneStone* SoulStoneTemplate)
{
	if (SoulStoneTemplate == nullptr)
		return;

	FSoItem SoulStone;
	SoulStone.Template = SoulStoneTemplate;
	SoOwner->AddItem(SoulStone, false);

	for (FSoItem& Item : EquippedRuneStones)
		if (Item.Template == SoulStoneTemplate)
		{
			Item.Amount += 1;
			RestoreSpells();
			return;
		}

	EquippedRuneStones.Add(SoulStone);
	OnEquippedSpellListChanged.Broadcast();
	RestoreSpells();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::UseActiveRuneStoneIfCastable()
{
	if (EquippedRuneStones.IsValidIndex(ActiveEquippedRuneStoneIndex))
	{
		USoItemTemplateRuneStone* RuneStone = EquippedRuneStones[ActiveEquippedRuneStoneIndex].GetTemplateAsRuneStone();
		int32* Amount = SpellMap.Find(RuneStone);

		// check if it isn't used up all
		if (Amount == nullptr || (*Amount) > 0)
		{
			// check if it isn't waiting for cooldown
			bool bWaitForCooldown = false;
			for (int32 i = 0; i < SoOwner->Cooldowns.Num(); ++i)
				if (SoOwner->Cooldowns[i].Object == EquippedRuneStones[ActiveEquippedRuneStoneIndex].Template)
				{
					bWaitForCooldown = true;
					SoOwner->CooldownBlocksEvent.Broadcast(i, SoOwner->Cooldowns[i].Counter, SoOwner->Cooldowns[i].Object);
					break;
				}

			const bool bCantCauseInAir = !RuneStone->bCanBeUsedInAir && !SoOwner->SoMovement->IsMovingOnGround();
			const bool bCantCauseNoSKZone = !RuneStone->bCanBeUsedInSKFreeZone && !SoOwner->bCanUseSoulkeeper;

			if (!bWaitForCooldown && !bCantCauseInAir && !bCantCauseNoSKZone)
			{
				const TSubclassOf<USoEffectBase>& Effect = RuneStone->GetEffect();
				if (Effect != nullptr)
				{
					USoEffectBase* DefaultEffectObject = Effect->GetDefaultObject<USoEffectBase>();
					if (DefaultEffectObject->CanBeApplied(SoOwner))
					{
						ISoMortal::Execute_ApplyEffect(SoOwner, Effect, true);

						if (RuneStone->SFXOnUse != nullptr)
							USoAudioManager::PlaySoundAtLocation(SoOwner, RuneStone->SFXOnUse, SoOwner->GetActorTransform());

						if (Amount != nullptr)
							(*Amount) -= 1;

						const float Cooldown = ISoCooldown::Execute_GetCooldownDuration(RuneStone);
						const bool bCanCountdownInAir = ISoCooldown::Execute_CanCountDownInAir(RuneStone);
						if (Cooldown > KINDA_SMALL_NUMBER)
							SoOwner->AddCooldown(RuneStone, Cooldown, bCanCountdownInAir);

						SoOwner->GetPlayerProgress()->OnSpellUsed(RuneStone);
						OnSelectedSpellChanged.Broadcast();
						OnSelectedSpellCasted.Broadcast();
						return;
					}

					const ESoDisplayText ReasonText = DefaultEffectObject->GetCantBeAppliedReason();
					if (ReasonText != ESoDisplayText::EDT_Max)
						ASoGameMode::Get(this).DisplayText(SoOwner->GetActorLocation(), ReasonText);
				}
			}
			else
			{
				if (bCantCauseNoSKZone)
				{
					ASoGameMode::Get(this).DisplayText(SoOwner->GetActorLocation(), ESoDisplayText::EDT_CantUseInSKFreeZone);
				}
				else
				{
					if (bCantCauseInAir)
						ASoGameMode::Get(this).DisplayText(SoOwner->GetActorLocation(), ESoDisplayText::EDT_CantUseInAir);
				}
			}
		}
	}

	USoAudioManager::PlaySoundAtLocation(SoOwner, SoOwner->SFXFCanNot, {});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::DecreaseRuneStoneCharges(USoItemTemplateRuneStone* RuneStone)
{
	int32* Amount = SpellMap.Find(RuneStone);
	if (Amount != nullptr && *Amount > 0)
	{
		(*Amount) -= 1;
		OnSelectedSpellChanged.Broadcast();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::RegainLastUsedRuneStoneCharge()
{
	const USoItemTemplateRuneStone* ActiveRuneStone = GetActiveSpell();
	int32* Amount = SpellMap.Find(ActiveRuneStone);
	if (Amount != nullptr && ActiveRuneStone != nullptr)
		for (const FSoItem& Rune : EquippedRuneStones)
			if (ActiveRuneStone == Rune.GetTemplateAsRuneStone() && ActiveRuneStone->UsageCount > 0)
			{
				(*Amount) = FMath::Min((*Amount + 1), ActiveRuneStone->UsageCount * Rune.Amount);
				OnSelectedSpellChanged.Broadcast();
				return true;
			}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SetSpellUsageCount(const USoItemTemplateRuneStone* Spell, int32 Amount, bool bDelta)
{
	int32* AmountPtr = SpellMap.Find(Spell);
	if (AmountPtr != nullptr)
	{
		if (bDelta)
			(*AmountPtr) += Amount;
		else
			(*AmountPtr) = Amount;
		OnSelectedSpellChanged.Broadcast();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SaveEquipment()
{
	FSoWorldState::Get().WriteEquippedItemList(CurrentEquipment);
	FSoWorldState::Get().WriteEquippedSpellItemList(EquippedRuneStones);

	// Write the current slot equipment templates
	TArray<USoItemTemplate*> ItemTemplates;
	for (const FSoItem& Item : CurrentEquipment)
	{
		if (Item.Template != nullptr)
		{
			ItemTemplates.Add(Item.Template);
		}
	}

	FSoWorldState::Get().SetMetadataEquippedItems(ItemTemplates);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::LoadEquipment()
{
	for (int32 i = 0; i < CurrentEquipment.Num(); ++i)
		CurrentEquipment[i] = FSoItem{};

	TArray<FSoItem> Items;
	TArray<FSoItem> Spells;
	FSoWorldState::Get().ReadEquippedItemList(Items);
	FSoWorldState::Get().ReadEquippedSpellItemList(Spells);

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].Template != nullptr)
		{
			if (i < CurrentEquipment.Num() && Items[i].Template->IsSlotCompatible(static_cast<ESoItemSlot>(i)))
			{
				CurrentEquipment[i] = Items[i];
				OnItemEquiped(static_cast<ESoItemSlot>(i), Items[i], true);
			}
			else
				SoOwner->GetInventory()->AddItem(Items[i]);
		}
	}

	EquippedRuneStones.Empty();
	for (const FSoItem& Spell : Spells)
		if (Cast<USoItemTemplateRuneStone>(Spell.Template) != nullptr)
			EquippedRuneStones.Add(Spell);
	OnEquippedSpellListChanged.Broadcast();

	for (int32 i = 0; i < CurrentEquipment.Num(); ++i)
	{
		UpdateSlot(static_cast<ESoItemSlot>(i));
		OnSlotChange(static_cast<ESoItemSlot>(i));
	}

	RestoreSpells();
	OnSelectedSpellChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::ReplaceRuneStone(USoItemTemplateRuneStone* OldStone, USoItemTemplateRuneStone* NewStone)
{
	if (OldStone == nullptr || NewStone == nullptr)
	{
		UE_LOG(LogSoPlayerCharacterSheet, Error, TEXT("USoPlayerCharacterSheet::ReplaceRuneStone called with invalid template(s)"))
		return false;
	}

	USoInventoryComponent* InventoryComponent = SoOwner->GetInventory();

	bool bFound = false;

	int32 ItemIndex = InventoryComponent->GetIndexFromTemplate(OldStone);
	if (ItemIndex >= 0)
	{
		bFound = true;
		InventoryComponent->GetItemList()[ItemIndex].Template = NewStone;

	}

	for (int32 i = EquippedRuneStones.Num() - 1; i>=0; --i)
		if (EquippedRuneStones[i].Template == OldStone)
		{
			bFound = true;
			EquippedRuneStones[i].Template = NewStone;
		}

	if (bFound)
	{
		SpellMap.Remove(OldStone);
		OnSelectedSpellChanged.Broadcast();
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoItem& USoPlayerCharacterSheet::GetEquippedItem(ESoItemSlot ItemSlot) const
{
	const int32 Index = static_cast<int32>(ItemSlot);
	if (CurrentEquipment.IsValidIndex(Index))
		return CurrentEquipment[Index];

	UE_LOG(LogSoPlayerCharacterSheet, Error, TEXT("USoPlayerCharacterSheet::GetEquippedItem called with invalid index %d"), Index);
	return FSoItem::Invalid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::IsSlotNotEmpty(ESoItemSlot ItemSlot) const
{
	const int32 Index = static_cast<int32>(ItemSlot);
	return (CurrentEquipment.IsValidIndex(Index) && CurrentEquipment[Index].Template != nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoItem& USoPlayerCharacterSheet::GetEquippedItem(ESoItemSlot ItemSlot)
{
	const int32 Index = static_cast<int32>(ItemSlot);
	check(Index >= 0 && Index < CurrentEquipment.Num());
	return CurrentEquipment[Index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::IsItemEquipped(USoItemTemplate* Template) const
{
	for (const FSoItem& Item : CurrentEquipment)
		if (Item.Template == Template)
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::DecreaseItemCountOnSlot(ESoItemSlot ItemSlot, int32 Delta)
{
	const int32 Index = static_cast<int32>(ItemSlot);

	if (CurrentEquipment[Index].Template == nullptr || CurrentEquipment[Index].Template->IsStackable() == false)
		return true;

	if (CurrentEquipment[Index].Amount <= Delta && CurrentEquipment[Index].Template->DestroyOnRunOut())
	{
		DestroyItemOnSlot(ItemSlot);
		ToggleItemSlots();
		return false;
	}

	CurrentEquipment[Index].Amount -= Delta;
	UpdateSlot(ItemSlot);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::DestroyItemOnSlot(ESoItemSlot TargetSlot)
{
	const int32 Index = static_cast<int32>(TargetSlot);
	if (Index < 0 || Index > CurrentEquipment.Num())
	{
		UE_LOG(LogSoPlayerCharacterSheet, Error, TEXT("Failed to destroy equipped item: wrong slot index!"));
		return;
	}
	if (CurrentEquipment[Index].Template != nullptr)
	{
		OnItemEquiped(TargetSlot, CurrentEquipment[Index], false);
		CurrentEquipment[Index] = FSoItem{};
		OnSlotChanged.Broadcast(TargetSlot);
		RecalculateStats();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::UpdateSlot(ESoItemSlot TargetSlot)
{
	OnSlotChanged.Broadcast(TargetSlot);
	RecalculateStats();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnSlotChange(ESoItemSlot Slot)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SetEquippedSpells(const TArray<FSoItem>& RuneStones)
{
	ActiveEquippedRuneStoneIndex = 0;
	EquippedRuneStones = RuneStones;
	OnEquippedSpellListChanged.Broadcast();

	RestoreSpells();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::RestoreSpells()
{
	TArray<TSubclassOf<USoEffectBase>> EffectsToApply;
	SpellMap.Empty();
	for (const FSoItem& Runes : EquippedRuneStones)
	{
		const USoItemTemplateRuneStone* RuneStone = Runes.GetTemplateAsRuneStone();
		if (RuneStone->UsageCount > 0)
			SpellMap.Add(RuneStone, RuneStone->UsageCount  * Runes.Amount);
		else if (RuneStone->UsageCount == 0)
		{
			SpellMap.Add(RuneStone, 0);
			EffectsToApply.Add(RuneStone->GetEffect());
		}
	}
	OnSelectedSpellChanged.Broadcast();

	SoOwner->OnSpellsReseted.Broadcast();

	for (auto& Effect : EffectsToApply)
		ISoMortal::Execute_ApplyEffect(SoOwner, Effect, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::RegainSpellCharges()
{
	for (const FSoItem& Runes : EquippedRuneStones)
	{
		const USoItemTemplateRuneStone* RuneStone = Runes.GetTemplateAsRuneStone();
		if (RuneStone->UsageCount > 0)
			SpellMap.Add(RuneStone, RuneStone->UsageCount  * Runes.Amount);
		else if (RuneStone->UsageCount == 0)
			SpellMap.Add(RuneStone, 0);
	}
	OnSelectedSpellChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const USoItemTemplateRuneStone* USoPlayerCharacterSheet::GetActiveSpell() const
{
	if (EquippedRuneStones.IsValidIndex(ActiveEquippedRuneStoneIndex))
		return EquippedRuneStones[ActiveEquippedRuneStoneIndex].GetTemplateAsRuneStone();

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoItemTemplateRuneStone* USoPlayerCharacterSheet::GetActiveSpell()
{
	if (EquippedRuneStones.IsValidIndex(ActiveEquippedRuneStoneIndex))
		return EquippedRuneStones[ActiveEquippedRuneStoneIndex].GetTemplateAsRuneStone();

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoPlayerCharacterSheet::GetRuneStoneEquippedCountFromSpellEffect(UObject* Effect) const
{
	int32 Amount = 0;
	if (Effect != nullptr)
		if (UClass* EftClass = Effect->GetClass())
			for (const FSoItem& EquippedRuneStone : EquippedRuneStones)
				if (USoItemTemplateRuneStone* RuneStone = Cast<USoItemTemplateRuneStone>(EquippedRuneStone.Template))
					if (RuneStone->Effect == EftClass)
					{
						Amount += EquippedRuneStone.Amount;
					}

	return Amount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoPlayerCharacterSheet::GetRuneStoneEquippedCount(USoItemTemplateRuneStone* RuneStone) const
{
	int32 Amount = 0;
	for (const FSoItem& EquippedRuneStone : EquippedRuneStones)
		if (EquippedRuneStone.Template == RuneStone)
			{
				Amount += EquippedRuneStone.Amount;
			}

	return Amount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoPlayerCharacterSheet::GetRuneStoneActiveChargeCount(USoItemTemplateRuneStone* RuneStone) const
{
	const int32* Amount = SpellMap.Find(RuneStone);
	// check if it isn't used up all
	if (Amount == nullptr)
		return RuneStone->UsageCount;

	return *Amount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoUsableItemType USoPlayerCharacterSheet::GetPrimaryItemType() const
{
	const FSoItem& Item = CurrentEquipment[static_cast<int32>(ESoItemSlot::EIS_Item0)];
	if (Item.Template == nullptr)
		return ESoUsableItemType::EUIT_MAX;

	check(Cast<USoUsableItemTemplate>(Item.Template));
	return Cast<USoUsableItemTemplate>(Item.Template)->UsableType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoPlayerCharacterSheet::RefillTestStoneBag(FName BagName)
{
	auto CheckItem = [BagName](FSoItem& Item) -> bool
	{
		if (USoUsableItemTemplate* Usable = Cast<USoUsableItemTemplate>(Item.Template))
			return (Usable->UsableItemName == BagName);

		return false;
	};

	auto OnItemFound = [](FSoItem& Item)
	{
		const int32 AmountToAdd = FMath::Max(0, Item.Template->GetMaxStackNum() - Item.Amount);
		Item.Amount += AmountToAdd;
		return AmountToAdd;
	};

	for (int32 i = static_cast<int32>(ESoItemSlot::EIS_Item0); i <= static_cast<int32>(ESoItemSlot::EIS_Item1); ++i)
		if (CheckItem(CurrentEquipment[i]))
		{
			const int32 Result = OnItemFound(CurrentEquipment[i]);
			OnSlotChanged.Broadcast(static_cast<ESoItemSlot>(i));
			return Result;
		}

	TArray<FSoItem>& ItemList = SoOwner->SoInventory->GetItemList();
	for (int32 i = 0; i < ItemList.Num(); ++i)
		if (CheckItem(ItemList[i]))
			return OnItemFound(ItemList[i]);

	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::ToggleItemSlots()
{
	if (ToggleSlotContents2(static_cast<int32>(ESoItemSlot::EIS_Item0)))
		USoAudioManager::PlaySoundAtLocation(SoOwner, SoOwner->SFXItemSwitch, SoOwner->GetActorTransform());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::ToggleWeapons()
{
	if (ToggleSlotContents2(static_cast<int32>(ESoItemSlot::EIS_Weapon0)))
		USoAudioManager::PlaySoundAtLocation(SoOwner, SoOwner->SFXWeaponSwitch, SoOwner->GetActorTransform());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::ToggleSpells()
{
	if (EquippedRuneStones.Num() > 0)
	{
		ActiveEquippedRuneStoneIndex = USoMathHelper::WrapIndexAround(ActiveEquippedRuneStoneIndex + 1, EquippedRuneStones.Num());
		OnSelectedSpellSwitched.Broadcast();
	}
}

void USoPlayerCharacterSheet::SetActiveSpellIndex(const int32 SpellIndex)
{
	if (EquippedRuneStones.IsValidIndex(SpellIndex))
	{
		ActiveEquippedRuneStoneIndex = SpellIndex;
		OnSelectedSpellSwitched.Broadcast();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::ToggleSlotContents2(int32 A)
{
	check(A >= 0 && A + 1 < CurrentEquipment.Num());

	const int32 B = A + 1;
	if (CurrentEquipment[B].Template == nullptr)
		return false;

	Swap(CurrentEquipment[A], CurrentEquipment[B]);

	OnSlotChanged.Broadcast(static_cast<ESoItemSlot>(A));
	OnSlotChanged.Broadcast(static_cast<ESoItemSlot>(B));

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::HasWeaponReady() const
{
	return CurrentEquipment[static_cast<int32>(ESoItemSlot::EIS_Weapon0)].Template != nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnItemPickedUp(const FSoItem& Item, int32 InventorySlotIndex)
{
	if (Item.Template == nullptr)
		return;


	USoQuestItemTemplate* QuestTemplate = Cast<USoQuestItemTemplate>(Item.Template);
	if (QuestTemplate != nullptr && QuestTemplate->BoolToAdd != NAME_None && InventorySlotIndex != -1)
	{
		if (SoOwner->SoInventory->GetItem(InventorySlotIndex).Amount == QuestTemplate->MaxStackNum)
			SetBoolValue(QuestTemplate->BoolToAdd, true);
	}

	// add to existing character sheet slot if possible?
	for (int32 i = 0; i < CurrentEquipment.Num(); ++i)
		if (Item.Template == CurrentEquipment[i].Template &&
			CurrentEquipment[i].Amount + Item.Amount < CurrentEquipment[i].Template->GetMaxStackNum())
		{
			CurrentEquipment[i].Amount += Item.Amount;
			OnSlotChanged.Broadcast(static_cast<ESoItemSlot>(i));

			if (!SoOwner->SoInventory->RemoveItem(InventorySlotIndex, Item.Amount))
			{
				UE_LOG(LogSoPlayerCharacterSheet, Error, TEXT("OnItemPickedUp: Failed to remove item amount at SlotIndex = %d"), InventorySlotIndex);
			}
			return;
		}

	// auto equip
	for (int32 i = 0; i < CurrentEquipment.Num(); ++i)
		if (Item.Template->IsSlotCompatible(static_cast<ESoItemSlot>(i)) && CurrentEquipment[i].Template == nullptr)
		{
			Equip(InventorySlotIndex, static_cast<ESoItemSlot>(i));
			return;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnItemRemoved(const FSoItem& Item)
{
	USoQuestItemTemplate* QuestTemplate = Cast<USoQuestItemTemplate>(Item.Template);
	if (QuestTemplate != nullptr && QuestTemplate->BoolToAdd != NAME_None)
		SetBoolValue(QuestTemplate->BoolToAdd, false);

	// remove equipped spells if necessary
	if (USoItemTemplateRuneStone* RuneStone = Cast<USoItemTemplateRuneStone>(Item.Template))
	{
		for (int32 i = 0; i < EquippedRuneStones.Num(); ++i)
			if (EquippedRuneStones[i].Template == RuneStone)
			{
				const int32 MaxAmount = SoOwner->GetInventory()->CalcItemCountFromTemplate(RuneStone);
				EquippedRuneStones[i].Amount = FMath::Min(EquippedRuneStones[i].Amount, MaxAmount);
				if (EquippedRuneStones[i].Amount == 0)
				{
					SpellMap.Remove(RuneStone);
					EquippedRuneStones.RemoveAt(i);
				}
				else
					if (int32* AmountPtr = SpellMap.Find(RuneStone))
						*AmountPtr = FMath::Min(*AmountPtr, MaxAmount);

				OnSelectedSpellChanged.Broadcast();
				break;
			}
		OnEquippedSpellListChanged.Broadcast();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::EquipFromSlot(int32 SourceInventoryIndex)
{
	USoInventoryComponent* Inventory = SoOwner->GetInventory();
	const TArray<FSoItem>& ItemList = Inventory->GetItemList();
	if (SourceInventoryIndex < 0 || SourceInventoryIndex >= ItemList.Num() || ItemList[SourceInventoryIndex].Template == nullptr)
		return;

	const ESoItemSlot Slot = ItemList[SourceInventoryIndex].Template->GetDefaultSlot();

	switch (Slot)
	{
		case ESoItemSlot::EIS_Max:
			return;

		case ESoItemSlot::EIS_Item0:
		{
			int32 i = static_cast<int32>(ESoItemSlot::EIS_Item0);
			for (; i <= static_cast<int32>(ESoItemSlot::EIS_Item1); ++i)
				if (CurrentEquipment[i].Template == nullptr)
					break;

			if (i > static_cast<int32>(ESoItemSlot::EIS_Item1))
				i = static_cast<int32>(ESoItemSlot::EIS_Item0);

			Equip(SourceInventoryIndex, static_cast<ESoItemSlot>(i));
		}
		break;

		default:
			Equip(SourceInventoryIndex, Slot);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::Equip(int32 SourceInventoryIndex, ESoItemSlot TargetSlot)
{
	USoInventoryComponent* Inventory = SoOwner->GetInventory();
	const TArray<FSoItem> ItemList = Inventory->GetItemList();

	if (SourceInventoryIndex >= 0 && SourceInventoryIndex < ItemList.Num())
	{
		const FSoItem Item = ItemList[SourceInventoryIndex];
		if (Item.Template != nullptr && Item.Template->IsSlotCompatible(TargetSlot))
		{
			Inventory->RemoveItem(SourceInventoryIndex, Item.Amount);
			Unequip(TargetSlot);
			const int32 EquippedIndex = static_cast<int32>(TargetSlot);
			CurrentEquipment[EquippedIndex] = Item;
			OnItemEquiped(TargetSlot, Item, true);

			OnSlotChanged.Broadcast(TargetSlot);
			RecalculateStats();

			if (USoPlayerProgress* Progress = SoOwner->GetPlayerProgress())
				Progress->OnEquippedItem(Item.Template);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::Unequip(ESoItemSlot TargetSlot)
{
	const int32 EquippedIndex = static_cast<int32>(TargetSlot);
	check(EquippedIndex >= 0 && EquippedIndex < CurrentEquipment.Num());

	if (CurrentEquipment[EquippedIndex].Template != nullptr)
	{
		const FSoItem Item = CurrentEquipment[EquippedIndex];
		CurrentEquipment[EquippedIndex] = FSoItem{};
		OnItemEquiped(TargetSlot, Item, false);
		SoOwner->GetInventory()->AddItem(Item);

		OnSlotChanged.Broadcast(TargetSlot);
		RecalculateStats();

		if (USoPlayerProgress* Progress = SoOwner->GetPlayerProgress())
			Progress->OnUnEquippedItem(Item.Template);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SwapSlots(ESoItemSlot SourceSlot, ESoItemSlot TargetSlot)
{
	const int32 SourceIndex = static_cast<int32>(SourceSlot);
	const int32 TargetIndex = static_cast<int32>(TargetSlot);
	check(SourceIndex >= 0 && SourceIndex < CurrentEquipment.Num());
	check(TargetIndex >= 0 && TargetIndex < CurrentEquipment.Num());

	if (CurrentEquipment[SourceIndex].Template == nullptr)
		return;

	if (CurrentEquipment[SourceIndex].Template->IsSlotCompatible(TargetSlot))
	{
		OnItemEquiped(SourceSlot, CurrentEquipment[SourceIndex], false);
		OnItemEquiped(TargetSlot, CurrentEquipment[TargetIndex], false);

		Swap(CurrentEquipment[SourceIndex], CurrentEquipment[TargetIndex]);

		OnItemEquiped(SourceSlot, CurrentEquipment[SourceIndex], true);
		OnItemEquiped(TargetSlot, CurrentEquipment[TargetIndex], true);

		OnSlotChanged.Broadcast(SourceSlot);
		OnSlotChanged.Broadcast(TargetSlot);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SetUsableAmount(FName UsableName, int32 Amount, bool bDelta)
{
	auto IsDesiredItem = [UsableName](const FSoItem& Item) -> bool
	{
		const USoUsableItemTemplate* Usable = Cast<USoUsableItemTemplate>(Item.Template);
		return (Usable != nullptr && Usable->GetUsableItemName() == UsableName);
	};

	// check equiped items
	for (int32 i = 0; i < 3; ++i)
	{
		const int32 Index = static_cast<int32>(ESoItemSlot::EIS_Item0) + i;
		const ESoItemSlot Slot = static_cast<ESoItemSlot>(Index);
		if (IsDesiredItem(CurrentEquipment[Index]))
		{
			if (bDelta)
				DecreaseItemCountOnSlot(Slot, -Amount);
			else
			{
				CurrentEquipment[Index].Amount = Amount;
				OnSlotChanged.Broadcast(Slot);
			}
			return;
		}
	}

	// check inventory
	const TArray<FSoItem>& ItemList = SoOwner->GetInventory()->GetItemList();
	for (int32 i = 0; i < ItemList.Num(); ++i)
		if (IsDesiredItem(ItemList[i]))
		{
			SoOwner->GetInventory()->ModifyItemCount(i, bDelta, Amount);
			return;
		}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoPlayerCharacterSheet::GetUsableAmount(FName UsableName) const
{
	auto IsDesiredItem = [UsableName](const FSoItem& Item) -> bool
	{
		const USoUsableItemTemplate* Usable = Cast<USoUsableItemTemplate>(Item.Template);
		return (Usable != nullptr && Usable->GetUsableItemName() == UsableName);
	};

	// check equiped items
	for (int32 i = 0; i < 3; ++i)
	{
		const int32 Index = static_cast<int32>(ESoItemSlot::EIS_Item0) + i;
		ESoItemSlot Slot = static_cast<ESoItemSlot>(Index);
		if (IsDesiredItem(CurrentEquipment[Index]))
			return CurrentEquipment[Index].Amount;
	}

	// check inventory
	const TArray<FSoItem>& ItemList = SoOwner->GetInventory()->GetItemList();
	for (int32 i = 0; i < ItemList.Num(); ++i)
		if (IsDesiredItem(ItemList[i]))
			return ItemList[i].Amount;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::DrinkPotionFromInventory(int32 ItemIndex)
{
	USoInventoryComponent* Inventory = SoOwner->GetInventory();
	TArray<FSoItem>& ItemList = Inventory->GetItemList();
	if (ItemList.IsValidIndex(ItemIndex) && ItemList[ItemIndex].Amount > 0)
	{
		if (USoUsableItemTemplate* Potion = Cast<USoUsableItemTemplate>(ItemList[ItemIndex].Template))
		{
			if (Potion->UsableType == ESoUsableItemType::EUIT_Potion)
			{
				ApplyPotionEffects(Potion);
				USoAudioManager::PlaySoundAtLocation(SoOwner, SoOwner->SFXFOnHeal, SoOwner->GetActorTransform());
				ItemList[ItemIndex].Amount -= 1;
				if (ItemList[ItemIndex].Amount == 0 && Potion->DestroyOnRunOut())
					Inventory->RemoveItem(ItemIndex, 0);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::ApplyPotionEffects(USoUsableItemTemplate* Potion)
{
	if (Potion != nullptr && Potion->UsableType == ESoUsableItemType::EUIT_Potion)
	{
		for (const TSubclassOf<USoEffectBase>& EffectClass : Potion->Effects)
			ISoMortal::Execute_ApplyEffect(SoOwner, EffectClass, true);

		if (Potion->HealAmount > 0.0f)
		{
			IncreaseHealth(Potion->HealAmount);
			DecreaseSoulkeeperRespawnPenalty(Potion->HealAmount);
			SoOwner->DisplayVisualEffect_Implementation(ESoVisualEffect::EVE_Heal);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::OnItemEquiped(ESoItemSlot Slot, const FSoItem& Item, bool bEquiped)
{
	// not all item slots can give static effects
	switch (Slot)
	{
		case ESoItemSlot::EIS_Item0:
		case ESoItemSlot::EIS_Item1:
		case ESoItemSlot::EIS_Weapon1:
			return;

		case ESoItemSlot::EIS_Weapon0:
			if (bEquiped)
				SoOwner->SelectWeapon(Item);
			else
			{
				SoOwner->SoSword->SetVisibility(false, true);
				SoOwner->SoOffHandWeapon->SetVisibility(false, true);
			}
			return;

		default:
			break;
	}

	SoOwner->ApplyStaticEffects(Item, bEquiped);

	USoItemTemplateShard* ShardTemplate = Cast<USoItemTemplateShard>(Item.Template);
	if (ShardTemplate != nullptr)
		USoUIHelper::PreCacheTexture2D(ShardTemplate->GetIcon());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::RecalculateStats()
{
	OnRecalculateAttributes.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoPlayerCharacterSheet::IncreaseSoulkeeperRespawnPenalty()
{
	if (FSoWorldState::Get().GetGameDifficulty() != ESoDifficulty::Sane)
		SoulkeeperRespawnPenalty += 10.0f;

	return SoulkeeperRespawnPenalty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SetIntValue(FName ValueName, int32 Value, bool bDelta)
{
	if (!PlayerData.Ints.Contains(ValueName))
		PlayerData.Ints.Add(ValueName, 0);

	if (bDelta)
		PlayerData.Ints[ValueName] += Value;
	else
		PlayerData.Ints[ValueName] = Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoPlayerCharacterSheet::GetIntValue(FName ValueName) const
{
	if (!PlayerData.Ints.Contains(ValueName))
	{
		return 0;
	}
	return PlayerData.Ints[ValueName];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SetFloatValue(FName ValueName, float Value, bool bDelta)
{
	if (!PlayerData.Floats.Contains(ValueName))
		PlayerData.Floats.Add(ValueName, 0.0f);

	if (bDelta)
		PlayerData.Floats[ValueName] += Value;
	else
		PlayerData.Floats[ValueName] = Value;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoPlayerCharacterSheet::GetFloatValue(FName ValueName) const
{
	if (!PlayerData.Floats.Contains(ValueName))
	{
		return 0.0f;
	}
	return PlayerData.Floats[ValueName];
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SetBoolValue(FName ValueName, bool bValue)
{
	if (bValue)
		PlayerData.TrueBools.Add(ValueName);
	else
		PlayerData.TrueBools.Remove(ValueName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::RemoveBoolsWithPrefix(const FString& Prefix)
{
	TSet<FName> NewSet;

	for (FName& Name : PlayerData.TrueBools)
		if (Name.ToString().Find(Prefix) == 0)
			NewSet.Add(Name);

	PlayerData.TrueBools = NewSet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoPlayerCharacterSheet::GetBoolValue(FName ValueName) const
{
	return PlayerData.TrueBools.Contains(ValueName);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::SetNameValue(FName ValueName, FName Value)
{
	if (PlayerData.Names.Contains(ValueName))
		PlayerData.Names[ValueName] = Value;
	else
		PlayerData.Names.Add(ValueName, Value);

	if (ValueName == SoOwner->ActiveDisplayNameName)
		UpdateDisplayName();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoPlayerCharacterSheet::GetNameValue(FName ValueName)
{
	if (PlayerData.Names.Contains(ValueName))
		return PlayerData.Names[ValueName];

	return NAME_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoPlayerCharacterSheet::UpdateDisplayName()
{
	FText* DisplayNamePtr = SoOwner->DialogueDisplayNameMap.Find(GetNameValue(SoOwner->ActiveDisplayNameName));
	if (DisplayNamePtr != nullptr)
		SoOwner->DialogueData.ParticipantDisplayName = *DisplayNamePtr;
	else
		SoOwner->DialogueData.ParticipantDisplayName = FROM_STRING_TABLE_DIALOGUE("char_name_warriorb");
}
