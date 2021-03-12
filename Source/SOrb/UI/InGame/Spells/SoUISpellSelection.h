// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGame/SoUIGameActivity.h"
#include "Items/SoItem.h"

#include "SoUISpellSelection.generated.h"

class UWrapBox;
class USoItemTemplateRuneStone;
class UHorizontalBox;
class UFMODEvent;

UCLASS()
class SORB_API USoUISpellSelection : public USoInGameUIActivity
{
	GENERATED_BODY()

protected:
	// Begin UUserWidget Interface
	void NativeConstruct() override;
	// End UUserWidget Interface

	// Begin USoInGameUIActivity Interface
	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;
	bool HandleCommand_Implementation(ESoUICommand Command) override;
	bool Update_Implementation(float DeltaSeconds) override;
	// End USoInGameUIActivity Interface

protected:
	void Reinitialize();

	void UpdateCanBeEquippedStates();

	void EquipSelected();
	void UnequipSelected();

	void UpdateEquippedBorders();

	void WriteEquippedStonesToCharacterSheet();

	UFUNCTION(BlueprintPure, Category = SpellSelection)
	bool CanEquipSelected() const;

	UFUNCTION(BlueprintPure, Category = SpellSelection)
	bool CanEquip(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = SpellSelection)
	int32 GetEquippedCount(USoItemTemplateRuneStone* RuneStone) const;


	bool HasOneEquipped(USoItemTemplateRuneStone* RuneStone, int32* IndexPtr = nullptr) const;

	UFUNCTION(BlueprintPure, Category = SpellSelection)
	int32 CalculateIndex() const;

	UFUNCTION(BlueprintPure, Category = SpellSelection)
	bool IsIndexEquipped(int32 Index) const;

	UFUNCTION(BlueprintImplementableEvent, Category = SpellSelection)
	void OnSelectionChange(USoItemTemplateRuneStone* RuneStone, bool bIsItEquipped);

	UFUNCTION(BlueprintImplementableEvent, Category = SpellSelection)
	void OnEquippedListChange();

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UWrapBox* SoWrapBox;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UHorizontalBox* EquippedContainer;

	bool bOpened = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Selection")
	int32 ColNum = 8;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Selection")
	int32 RowNum = 7;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Selection")
	bool bEditable = false;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	TArray<USoItemTemplateRuneStone*> CurrentRuneStones;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	TArray<FSoItem> EquippedRuneStones;

	int32 SelectedRowIndex = 0;
	int32 SelectedColIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	int32 EquippedRuneStoneCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	int32 EquippedRuneStoneCapacity = 0;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	int32 MaxCapacity = 0;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXSpellSelectionSwitch = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXSpellAdded= nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXSpellRemoved = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXSpellPanelClosed = nullptr;
};
