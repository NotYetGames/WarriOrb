// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CharacterBase/SoCharacterSheet.h"
#include "Items/SoItem.h"
#include "SaveFiles/SoWorldStateTypes.h"
#include "Basic/SoDifficulty.h"

#include "SoPlayerCharacterSheet.generated.h"

class USoItemTemplateRuneStone;
class ASoCharacter;
class USoUsableItemTemplate;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SORB_API USoPlayerCharacterSheet : public USoCharacterSheet
{
	GENERATED_BODY()
public:

	USoPlayerCharacterSheet();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

	// called before the game state is serialized (aka saved)
	UFUNCTION()
	void OnSave();

	// called after the game state is reloaded (aka loaded)
	UFUNCTION()
	void OnReload();

	/** call when difficulty is changed during game so the proper SK rune can be activated */
	void OnDifficultyChanged(ESoDifficulty NewDifficulty);

	void OnRevive(bool bSoulKeeperNotUsed);

	/** Heals, restores SK, spells etc. */
	UFUNCTION(BlueprintCallable, Category = Health)
	void Heal();

	UFUNCTION(BlueprintCallable, Category = Health)
	void ApplyHealthUpgrade();


	// res rune

	UFUNCTION(BlueprintCallable, Category = Spells)
	void AddSoulStone(USoItemTemplateRuneStone* SoulStoneTemplate);

	UFUNCTION(BlueprintCallable, Category = Spells)
	int32 GetMaxResRunCount() const { return MaxRuneNum; }

	// sets only the current one, not the default/max value
	void SetSpellUsageCount(const USoItemTemplateRuneStone* Spell, int32 Amount, bool bDelta);

	void SetUsableAmount(FName UsableName, int32 Amount, bool bDelta);
	int32 GetUsableAmount(FName UsableName) const;

	void DrinkPotionFromInventory(int32 ItemIndex);

	void ApplyPotionEffects(USoUsableItemTemplate* Potion);

	UFUNCTION(BlueprintCallable, Category = Stats)
	void ApplyAttackSpeedModifier(float Multiplier) { AttackSpeedMultiplier *= Multiplier; }

	UFUNCTION(BlueprintCallable, Category = Stats)
	void ApplyCooldownCounterMultiplier(float Multiplier) { CooldownCounterMultiplier *= Multiplier; }

	UFUNCTION(BlueprintCallable, Category = Stats)
	void ModifyCriticalHitChance(float Delta) { CriticalHitChance += Delta; }

	UFUNCTION(BlueprintCallable, Category = Stats)
	void AddDamageModifer(float Multiplier) { DamageModifiers *= Multiplier; }


	float GetCooldownCounterMultiplier() { return CooldownCounterMultiplier; }
	float GetCriticalHitChance() { return CriticalHitChance; }


	// Helper to get the active checkpoint name

	// dynamic values defined by FName, can be used in story scripts/dialogues
	void SetIntValue(FName ValueName, int32 Value, bool bDelta = false);
	int32 GetIntValue(FName ValueName) const;

	void SetFloatValue(FName ValueName, float Value, bool bDelta = false);
	float GetFloatValue(FName ValueName) const;

	UFUNCTION(BlueprintCallable, Category = DlgVariables)
	void SetBoolValue(FName ValueName, bool bValue);

	void RemoveBoolsWithPrefix(const FString& Prefix);

	UFUNCTION(BlueprintPure, Category = DlgVariables)
	bool GetBoolValue(FName ValueName) const;

	UFUNCTION(BlueprintCallable, Category = DlgVariables)
	void SetNameValue(FName ValueName, FName Value);

	UFUNCTION(BlueprintPure, Category = DlgVariables)
	FName GetNameValue(FName ValueName);

	void UpdateDisplayName();

	// items

	void OnItemPickedUp(const FSoItem& Item, int32 InventorySlotIndex);
	void OnItemRemoved(const FSoItem& Item);

	// Equip an item from the inventory
	void Equip(int32 SourceInventoryIndex, ESoItemSlot TargetSlot);

	// Equip an item from the inventory
	void EquipFromSlot(int32 SourceInventoryIndex);

	void Unequip(ESoItemSlot TargetSlot);

	// move an item from a character sheet slot to another
	// TargetSlot can be empty, SourceSlot can not
	void SwapSlots(ESoItemSlot SourceSlot, ESoItemSlot TargetSlot);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	const FSoItem& GetEquippedItem(ESoItemSlot ItemSlot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsSlotNotEmpty(ESoItemSlot ItemSlot) const;

	FSoItem& GetEquippedItem(ESoItemSlot ItemSlot);
	bool IsItemEquipped(USoItemTemplate* Template) const;

	/** decreases only stackable items, return value: true if there is any left, false otherwise */
	bool DecreaseItemCountOnSlot(ESoItemSlot ItemSlot, int32 Delta = 1);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void DestroyItemOnSlot(ESoItemSlot TargetSlot);

	void UpdateSlot(ESoItemSlot TargetSlot);

	UFUNCTION()
	void OnSlotChange(ESoItemSlot Slot);

	//
	// Spells
	//
	void UseActiveRuneStoneIfCastable();

	/** assumed to be the selected one */
	UFUNCTION(BlueprintCallable, Category = Spells)
	void DecreaseRuneStoneCharges(USoItemTemplateRuneStone* RuneStone);

	UFUNCTION(BlueprintCallable, Category = Spells)
	bool RegainLastUsedRuneStoneCharge();

	// sets equipped runestones, reinitializes the spell map (rest/spell reset is expected here)
	void SetEquippedSpells(const TArray<FSoItem>& RuneStones);

	UFUNCTION(BlueprintCallable, Category = Spells)
	void RestoreSpells();

	/** Regains spell charges but does not remove any active spells */
	UFUNCTION(BlueprintCallable, Category = Spells)
	void RegainSpellCharges();

	const TArray<FSoItem>& GetEquippedSpells() const { return EquippedRuneStones; }
	bool HasAnyEquippedSpells() const { return EquippedRuneStones.Num() > 0;  }

	const TMap<const USoItemTemplateRuneStone*, int32>& GetSpellMap() const { return SpellMap; }

	bool HasActiveSpell() const { return GetActiveSpell() != nullptr; }

	const USoItemTemplateRuneStone* GetActiveSpell() const;

	UFUNCTION(BlueprintPure, Category = Spells)
	int32 GetRuneStoneEquippedCountFromSpellEffect(UObject* Effect) const;

	UFUNCTION(BlueprintPure, Category = Spells)
	int32 GetRuneStoneEquippedCount(USoItemTemplateRuneStone* RuneStone) const;

	UFUNCTION(BlueprintPure, Category = Spells)
	int32 GetRuneStoneActiveChargeCount(USoItemTemplateRuneStone* RuneStone) const;

	UFUNCTION(BlueprintPure, Category = Spells)
	USoItemTemplateRuneStone* GetActiveSpell();

	bool CanCastActiveSpell() const { return CanUseSpells() && HasActiveSpell(); }

	int32 GetActiveSpellIndex() const { return ActiveEquippedRuneStoneIndex; }
	void SetActiveSpellIndex(const int32 SpellIndex);

	void ToggleSpells();

	// Only if we can use the spells, does not tell us if there is something to cast
	// bool CanUseSpells() const { return ESoUsableItemType::EUIT_Spell == GetPrimaryItemType(); }

	// let's try to allow spell switch and usage from spell switch wheel without having the spellcaster in primary slot
	bool CanUseSpells() const { return EquippedRuneStones.Num() > 0; }


	void ToggleItemSlots();
	void ToggleWeapons();

	bool HasWeaponReady() const;


	UFUNCTION(BlueprintCallable, Category = "Equipment")
	ESoUsableItemType GetPrimaryItemType() const;

	/**
	 *  Return: -1 -> no item
	 *  Return: 0  -> already full
	 *  Return: >0 -> x added
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	int32 RefillTestStoneBag(FName BagName);

	void RecalculateStats();

	void ResetSoulkeeperRespawnPenalty() { SoulkeeperRespawnPenalty = 0.0f; }
	float IncreaseSoulkeeperRespawnPenalty();
	void DecreaseSoulkeeperRespawnPenalty(float Amount) { SoulkeeperRespawnPenalty = FMath::Max(SoulkeeperRespawnPenalty - Amount, 0.0f); }

	const float GetDamageModifiers() const { return DamageModifiers; }

	float GetAttackSpeedMultiplier() const { return AttackSpeedMultiplier; }

protected:

	bool ToggleSlotContents2(int32 FirstIndex);

	/**
	 * Called each time an item is equipped/unequipped, used to update item effects
	 * @PARAM Slot: the changed slot
	 * @PARAM Item: the item which was equipped/unequipped
	 * @PARAM bEquiped: true if equipped, false if unequipped
	 */
	void OnItemEquiped(ESoItemSlot Slot, const FSoItem& Item, bool bEquiped);

	void SaveEquipment();
	void LoadEquipment();

	int32 GetStrikeIndexFromAssetName(const FString& Path);

	/** special function to swap res rune based on difficulty change */
	bool ReplaceRuneStone(USoItemTemplateRuneStone* OldStone, USoItemTemplateRuneStone* NewStone);

public:

	UPROPERTY(BlueprintAssignable)
	FSoEquipmentSlotChanged OnSlotChanged;

	/** fired when a usable item is used */
	UPROPERTY(BlueprintAssignable)
	FSoUINotify OnItemUsed;

	UPROPERTY(BlueprintAssignable)
	FSoUINotify OnRecalculateAttributes;

	UPROPERTY(BlueprintAssignable)
	FSoUINotify OnSelectedSpellChanged;

	UPROPERTY(BlueprintAssignable)
	FSoUINotify OnSelectedSpellCasted;

	UPROPERTY(BlueprintAssignable)
	FSoUINotify OnSelectedSpellSwitched;

	UPROPERTY(BlueprintAssignable)
	FSoUINotify OnEquippedSpellListChanged;

protected:
	static const FName HealthUpgradeName;

	const float HealthUpgradeValue = 5.0f;

	// resurrection runes
	int32 MaxRuneNum = 1;

	UPROPERTY(BlueprintReadWrite)
	float SoulkeeperRespawnPenalty = 0.0f;

	UPROPERTY(EditAnywhere)
	USoItemTemplateRuneStone* SoulStoneTemplateIntended;

	UPROPERTY(EditAnywhere)
	USoItemTemplateRuneStone* SoulStoneTemplateSane;

	UPROPERTY(BlueprintReadWrite)
	USoItemTemplate* SpellCasterTemplate;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Equipped items
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FSoItem> CurrentEquipment;

	// Spells
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FSoItem> EquippedRuneStones;

	// Current Spell index
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	int32 ActiveEquippedRuneStoneIndex = 0;

	// used to register how much spell do the player still have from the runes, must be > 0 to cast the spell
	// if a template isn't part of this map it is considered to be usable
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<const USoItemTemplateRuneStone*, int32> SpellMap;


	// script/story data
	FSoPlayerData PlayerData;

	// Our master
	UPROPERTY()
	ASoCharacter* SoOwner = nullptr;

	/** Current damage modifiers from clothes, effects, etc. */
	UPROPERTY(BlueprintReadOnly)
	float DamageModifiers = 1.0f;

	float AttackSpeedMultiplier = 1.0f;
	float CooldownCounterMultiplier = 1.0f;

	float CriticalHitChance = 0.0f;

	static const FName SKSplineName;
	static const FName SKPenaltyName;
	static const FName SKDistanceOnSplineName;
	static const FName SKZName;
	static const FName SKDirectionName;
};
