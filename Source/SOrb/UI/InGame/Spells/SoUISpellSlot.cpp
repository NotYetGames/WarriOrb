// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISpellSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSlot::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	OnStateChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSlot::SetEquipped(bool bEquipped)
{
	bIsEquipped = bEquipped;
	EquippedIcon->SetVisibility(bEquipped && bShowEquippedIcon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	OnStateChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSlot::SetCanBeEquipped(bool bCanBe)
{
	bCanBeEquipped = bCanBe;
	OnStateChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSlot::SetItem(const USoItemTemplateRuneStone* InActiveItem, int32 InAmount)
{
	ActiveItem = InActiveItem;
	if (ActiveItem != nullptr)
	{
		// Item with number
		Background->SetBrushFromTexture(SpellBackground);
		Icon->SetVisibility(ESlateVisibility::Visible);
		Icon->SetBrushFromTexture(ActiveItem->GetIcon());

		Amount = InAmount;
		AmountText->SetText(FText::AsNumber(Amount));
		AmountText->SetVisibility((Amount > 1 && !bShowEquippedIcon) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	else
	{
		// Empty spell slot
		Background->SetBrushFromTexture(EmptyBackground);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		AmountText->SetVisibility(ESlateVisibility::Collapsed);
		bIsEquipped = false;
		bCanBeEquipped = true;
		OnStateChanged();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellSlot::ModifyAmount(int32 Delta)
{
	Amount += Delta;
	AmountText->SetVisibility((Amount > 1 && !bShowEquippedIcon) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	AmountText->SetText(FText::AsNumber(Amount));

	return Amount > 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSlot::ShowBorderIfItemMatches(const USoItemTemplateRuneStone* Item)
{
	Border->SetColorAndOpacity(FLinearColor::White);
	Border->SetVisibility((ActiveItem == Item && Item != nullptr) ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellSlot::OnStateChanged()
{
	if (ActiveItem != nullptr)
	{
		Background->SetBrushFromTexture((bIsEquipped || bCanBeEquipped) ? SpellBackground : SpellBackgroundCantEquip);
		Border->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Background->SetBrushFromTexture(EmptyBackground);
		Border->SetVisibility(bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	Border->SetColorAndOpacity(bIsSelected ? FLinearColor::White
										   : (bIsEquipped ? FLinearColor::Black
														  : (bCanBeEquipped ? CanEquipBorderColor : CantEquipBorderColor)));
}
