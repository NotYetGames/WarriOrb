// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISpellSelection.h"

#include "Components/WrapBox.h"
#include "Components/HorizontalBox.h"

#include "SoUISpellSlot.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Items/SoInventoryComponent.h"
#include "Basic/SoAudioManager.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSelection::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);

	SelectedColIndex = 0;
	SelectedRowIndex = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSelection::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	if (bEnable && bOpened)
	{
		UE_LOG(LogSoUIActivity, Warning, TEXT("SpellSelection request rejected, already opened"));
		return true;
	}

	bOpened = bEnable;
	SetVisibility(bOpened ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bOpened)
	{
		Reinitialize();
	}
	else
	{
		if (bEditable)
		{
			WriteEquippedStonesToCharacterSheet();
			USoAudioManager::PlaySound2D(this, SFXSpellPanelClosed);
		}
	}

	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSelection::HandleCommand_Implementation(ESoUICommand Command)
{
	bool bHandledCommand = false;
	const int32 OldIndex = CalculateIndex();
	switch (Command)
	{
		case ESoUICommand::EUC_Left:
		case ESoUICommand::EUC_MainMenuLeft:
			bHandledCommand = true;
			SelectedColIndex = USoMathHelper::WrapIndexAround(SelectedColIndex - 1, ColNum);
			USoAudioManager::PlaySound2D(this, SFXSpellSelectionSwitch);
			break;

		case ESoUICommand::EUC_Right:
		case ESoUICommand::EUC_MainMenuRight:
			bHandledCommand = true;
			SelectedColIndex = USoMathHelper::WrapIndexAround(SelectedColIndex + 1, ColNum);
			USoAudioManager::PlaySound2D(this, SFXSpellSelectionSwitch);
			break;

		case ESoUICommand::EUC_Up:
		case ESoUICommand::EUC_MainMenuUp:
			bHandledCommand = true;
			SelectedRowIndex = USoMathHelper::WrapIndexAround(SelectedRowIndex - 1, RowNum);
			USoAudioManager::PlaySound2D(this, SFXSpellSelectionSwitch);
			break;

		case ESoUICommand::EUC_Down:
		case ESoUICommand::EUC_MainMenuDown:
			bHandledCommand = true;
			SelectedRowIndex = USoMathHelper::WrapIndexAround(SelectedRowIndex + 1, RowNum);
			USoAudioManager::PlaySound2D(this, SFXSpellSelectionSwitch);
			break;

		case ESoUICommand::EUC_Action0:
			bHandledCommand = true;
			if (bEditable)
				EquipSelected();
			break;

		case ESoUICommand::EUC_Action1:
			bHandledCommand = true;
			if (bEditable)
				UnequipSelected();
			break;

		case ESoUICommand::EUC_MainMenuBack:
			if (!bEditable)
				return false;
			bOpened = false;
			bHandledCommand = true;
		default:
			break;
	}

	const int32 NewIndex = CalculateIndex();
	if (OldIndex != NewIndex)
	{
		bHandledCommand = true;
		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(OldIndex)))
			SpellSlot->SetSelected(false);

		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(NewIndex)))
			SpellSlot->SetSelected(true);

		OnSelectionChange(CurrentRuneStones.IsValidIndex(NewIndex) ? CurrentRuneStones[NewIndex] : nullptr, IsIndexEquipped(NewIndex));
		UpdateEquippedBorders();
	}

	return bHandledCommand;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSelection::Update_Implementation(float DeltaSeconds)
{
	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISpellSelection::CalculateIndex() const
{
	return SelectedRowIndex * ColNum + SelectedColIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSelection::CanEquipSelected() const
{
	return CanEquip(CalculateIndex());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSelection::CanEquip(int32 Index) const
{
	if (!CurrentRuneStones.IsValidIndex(Index) || CurrentRuneStones[Index] == nullptr)
		return false;

	return ((EquippedRuneStoneCapacity + CurrentRuneStones[Index]->CapacityCost <= MaxCapacity) &&
		(EquippedRuneStoneCount < EquippedContainer->GetChildrenCount() || HasOneEquipped(CurrentRuneStones[Index])));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISpellSelection::GetEquippedCount(USoItemTemplateRuneStone* RuneStone) const
{
	for (int32 Index = 0; Index < EquippedContainer->GetChildrenCount(); ++Index)
		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(Index)))
			if (SpellSlot->GetItem() == RuneStone)
			{
				return SpellSlot->GetAmount();
			}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSelection::HasOneEquipped(USoItemTemplateRuneStone* RuneStone, int32* IndexPtr) const
{
	for (int32 Index = 0; Index < EquippedContainer->GetChildrenCount(); ++Index)
		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(Index)))
			if (SpellSlot->GetItem() == RuneStone)
			{
				if (IndexPtr != nullptr)
					*IndexPtr = Index;
				return true;
			}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSelection::IsIndexEquipped(int32 Index) const
{
	if (Index >= 0 && Index < SoWrapBox->GetChildrenCount())
		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(Index)))
			return SpellSlot->IsEquipped();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSelection::Reinitialize()
{
	ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (SoCharacter != nullptr)
	{
		EquippedRuneStoneCount = 0;
		EquippedRuneStoneCapacity = 0;
		MaxCapacity = SoCharacter->GetSpellsCapacity();

		// reorder runestones in inventory
		SoCharacter->GetInventory()->OrderRuneStones();

		// initialize equipped list
		EquippedRuneStones = SoCharacter->GetPlayerCharacterSheet()->GetEquippedSpells();
		TArray<FSoItem> TempEquippedRuneStones = EquippedRuneStones;
		for (const FSoItem& RuneStone : TempEquippedRuneStones)
		{
			const USoItemTemplateRuneStone* SpellTemplate = RuneStone.GetTemplateAsRuneStone();
			EquippedRuneStoneCapacity += RuneStone.Amount * SpellTemplate->CapacityCost;
			if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(EquippedRuneStoneCount++)))
				SpellSlot->SetItem(SpellTemplate, RuneStone.Amount);
		}

		for (int32 Index = EquippedRuneStoneCount; Index < EquippedContainer->GetChildrenCount(); ++Index)
			if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(Index)))
				SpellSlot->SetItem(nullptr, 0);


		// update current list
		CurrentRuneStones.Empty();
		const TArray<FSoItem>& ItemList = SoCharacter->GetInventory()->GetItemList();
		for (const FSoItem& Item : ItemList)
			if (USoItemTemplateRuneStone* RuneStone = Cast<USoItemTemplateRuneStone>(Item.Template))
				CurrentRuneStones.Add(RuneStone);

		for (int32 i = 0; i < SoWrapBox->GetChildrenCount(); ++i)
			if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(i)))
			{
				SpellSlot->SetSelected(false);
				SpellSlot->SetEquipped(false);
				if (CurrentRuneStones.IsValidIndex(i))
				{
					SpellSlot->SetItem(CurrentRuneStones[i], 1);
					for (int32 j = 0; j < TempEquippedRuneStones.Num(); ++j)
						if (TempEquippedRuneStones[j].Template == CurrentRuneStones[i] && TempEquippedRuneStones[j].Amount > 0)
						{
							SpellSlot->SetEquipped(true);
							TempEquippedRuneStones[j].Amount -= 1;
							break;
						}
					SpellSlot->SetCanBeEquipped(CanEquip(i));
				}
				else
					SpellSlot->SetItem(nullptr, 0);
			}

		// set selection to 0,0
		SelectedColIndex = 0;
		SelectedRowIndex = 0;
		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(0)))
		{
			SpellSlot->SetSelected(true);
			OnSelectionChange(CurrentRuneStones.IsValidIndex(0) ? CurrentRuneStones[0] : nullptr, SpellSlot->IsEquipped());
		}
	}
	OnEquippedListChange();
	UpdateEquippedBorders();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSelection::UpdateCanBeEquippedStates()
{
	for (int32 i = 0; i < SoWrapBox->GetChildrenCount(); ++i)
		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(i)))
			if (CurrentRuneStones.IsValidIndex(i))
				SpellSlot->SetCanBeEquipped(CanEquip(i));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSelection::EquipSelected()
{
	const int32 Index = CalculateIndex();
	if (!CurrentRuneStones.IsValidIndex(Index))
		return;

	if (!IsIndexEquipped(Index) && CanEquipSelected())
	{
		USoAudioManager::PlaySound2D(this, SFXSpellAdded);

		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(Index)))
			SpellSlot->SetEquipped(true);

		int32 EquippedIndex = -1;
		if (HasOneEquipped(CurrentRuneStones[Index], &EquippedIndex))
			Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(EquippedIndex))->ModifyAmount(1);
		else
		{
			Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(EquippedRuneStoneCount))->SetItem(CurrentRuneStones[Index], 1);
			EquippedRuneStoneCount += 1;
		}
		EquippedRuneStoneCapacity += CurrentRuneStones[Index]->CapacityCost;

		bool bAdded = false;
		for (FSoItem& RuneStone : EquippedRuneStones)
			if (RuneStone.Template == CurrentRuneStones[Index])
			{
				bAdded = true;
				RuneStone.Amount += 1;
			}

		if (!bAdded)
		{
			FSoItem Stone;
			Stone.Amount = 1;
			Stone.Template = CurrentRuneStones[Index];
			EquippedRuneStones.Add(Stone);
		}

		OnSelectionChange(CurrentRuneStones[Index], IsIndexEquipped(Index));
		OnEquippedListChange();
		UpdateCanBeEquippedStates();

		UpdateEquippedBorders();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSelection::UnequipSelected()
{
	const int32 Index = CalculateIndex();
	if (!CurrentRuneStones.IsValidIndex(Index))
		return;

	int32 EquippedIndex = -1;
	if (HasOneEquipped(CurrentRuneStones[Index], &EquippedIndex) && IsIndexEquipped(Index))
	{
		USoAudioManager::PlaySound2D(this, SFXSpellRemoved);

		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(SoWrapBox->GetChildAt(Index)))
			SpellSlot->SetEquipped(false);


		USoUISpellSlot* EquippedSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(EquippedIndex));
		if (!EquippedSlot->ModifyAmount(-1))
		{
			EquippedRuneStoneCount -= 1;

			bool bRemoved = false;
			// remove from container
			for (int32 i = EquippedContainer->GetChildrenCount() - 1; i >= 0 && !bRemoved; --i)
				if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(i)))
					if (SpellSlot->GetItem() == CurrentRuneStones[Index])
					{
						bRemoved = true;
						for (int32 j = i; j < EquippedContainer->GetChildrenCount() - 1; ++j)
						{
							USoUISpellSlot* PrevSpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(j));
							USoUISpellSlot* NextSpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(j + 1));
							PrevSpellSlot->SetItem(NextSpellSlot->GetItem(), NextSpellSlot->GetAmount());
						}
						Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(EquippedContainer->GetChildrenCount() - 1))->SetItem(nullptr, 0);
					}
		}

		EquippedRuneStoneCapacity -= CurrentRuneStones[Index]->CapacityCost;
		// remove from list
		for (int32 i = 0; i < EquippedRuneStones.Num(); ++i)
			if (EquippedRuneStones[i].Template == CurrentRuneStones[Index])
			{
				EquippedRuneStones[i].Amount -= 1;
				if (EquippedRuneStones[i].Amount == 0)
					EquippedRuneStones.RemoveAtSwap(i);
				break;
			}

		OnSelectionChange(CurrentRuneStones[Index], IsIndexEquipped(Index));
		OnEquippedListChange();
		UpdateCanBeEquippedStates();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSelection::UpdateEquippedBorders()
{
	USoItemTemplateRuneStone* RuneStone = nullptr;
	const int32 SelectedIndex = CalculateIndex();
	if (CurrentRuneStones.IsValidIndex(SelectedIndex))
		RuneStone = CurrentRuneStones[SelectedIndex];

	for (int32 Index = 0; Index < EquippedContainer->GetChildrenCount(); ++Index)
		if (USoUISpellSlot* SpellSlot = Cast<USoUISpellSlot>(EquippedContainer->GetChildAt(Index)))
			SpellSlot->ShowBorderIfItemMatches(RuneStone);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSelection::WriteEquippedStonesToCharacterSheet()
{
	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		SoCharacter->GetPlayerCharacterSheet()->SetEquippedSpells(EquippedRuneStones);
}
