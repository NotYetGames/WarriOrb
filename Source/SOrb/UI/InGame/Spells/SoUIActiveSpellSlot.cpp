// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIActiveSpellSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Items/SoItem.h"
#include "Items/ItemTemplates/SoUsableItemTemplate.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "CharacterBase/SoCharacterMovementComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);

	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		SoCharacter->OnCooldownStarted().AddDynamic(this, &USoUIActiveSpellSlot::OnCooldownChanged);
		SoCharacter->OnCooldownEnded().AddDynamic(this, &USoUIActiveSpellSlot::OnCooldownChanged);
		SoCharacter->OnMovementModeChangedNotify.AddDynamic(this, &USoUIActiveSpellSlot::OnSpellUsabilityMightChanged);
		SoCharacter->OnCanUseSoulKeeperChanged().AddDynamic(this, &USoUIActiveSpellSlot::OnSpellUsabilityMightChanged);
		SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellChanged.AddDynamic(this, &USoUIActiveSpellSlot::OnSelectedSpellChanged);
		SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellSwitched.AddDynamic(this, &USoUIActiveSpellSlot::OnSelectedSpellChanged);
		SoCharacter->GetPlayerCharacterSheet()->OnSlotChanged.AddDynamic(this, &USoUIActiveSpellSlot::OnCharacterItemSlotChanged);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::NativeDestruct()
{
	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		SoCharacter->OnCooldownStarted().RemoveDynamic(this, &USoUIActiveSpellSlot::OnCooldownChanged);
		SoCharacter->OnCooldownEnded().RemoveDynamic(this, &USoUIActiveSpellSlot::OnCooldownChanged);
		SoCharacter->OnMovementModeChangedNotify.RemoveDynamic(this, &USoUIActiveSpellSlot::OnSpellUsabilityMightChanged);
		SoCharacter->OnCanUseSoulKeeperChanged().RemoveDynamic(this, &USoUIActiveSpellSlot::OnSpellUsabilityMightChanged);
		SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellChanged.RemoveDynamic(this, &USoUIActiveSpellSlot::OnSelectedSpellChanged);
		SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellSwitched.RemoveDynamic(this, &USoUIActiveSpellSlot::OnSelectedSpellChanged);
		SoCharacter->GetPlayerCharacterSheet()->OnSlotChanged.RemoveDynamic(this, &USoUIActiveSpellSlot::OnCharacterItemSlotChanged);
	}
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::OnSelectedSpellChanged()
{
	Refresh();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::OnCharacterItemSlotChanged(ESoItemSlot ChangedSlot)
{
	if (ChangedSlot == ESoItemSlot::EIS_Item0)
		UpdateVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::OnCooldownChanged(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown)
{
	if (ObjectWithCooldown == CurrentlySelectedSpell && CurrentlySelectedSpell != nullptr)
	{
		bCooldownBlocked = RemainingTime > 0.0f;
		UpdateInactiveFadeVisibility();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::OnSpellUsabilityMightChanged()
{
	if (CurrentlySelectedSpell != nullptr)
		UpdateInactiveFadeVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::UpdateVisibility()
{
	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		bool bSpellCasterItemActive = false;
		const FSoItem& Item = SoCharacter->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Item0);
		if (USoUsableItemTemplate* Usable = Cast<USoUsableItemTemplate>(Item.Template))
			bSpellCasterItemActive = (Usable->UsableType == ESoUsableItemType::EUIT_Spell);

		SetVisibility((bSpellCasterItemActive && CurrentlySelectedSpell != nullptr && bCanBeDisplayed) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::UpdateInactiveFadeVisibility()
{
	if (!InactiveFade)
		return;

	const ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!SoCharacter)
		return;

	const bool bNotOnGroundBlocked = !CurrentlySelectedSpell->bCanBeUsedInAir && !SoCharacter->GetSoMovement()->IsMovingOnGround();
	const bool bNoSKZoneBlocked = !CurrentlySelectedSpell->bCanBeUsedInSKFreeZone && !SoCharacter->CanUseSoulkeeperAtLocation();
	InactiveFade->SetVisibility((bCooldownBlocked || bRunOut || bNotOnGroundBlocked || bNoSKZoneBlocked) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIActiveSpellSlot::Refresh()
{
	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		if (USoPlayerCharacterSheet* SoSheet = SoCharacter->GetPlayerCharacterSheet())
		{
			CurrentlySelectedSpell = SoSheet->GetActiveSpell();
			if (CurrentlySelectedSpell != nullptr)
			{
				Icon->SetBrushFromTexture(CurrentlySelectedSpell->GetIcon());
				const int32* AmountPtr = SoSheet->GetSpellMap().Find(CurrentlySelectedSpell);
				AmountText->SetVisibility(AmountPtr == nullptr ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
				if (AmountPtr != nullptr)
					AmountText->SetText(FText::AsNumber(*AmountPtr, &FNumberFormattingOptions::DefaultNoGrouping()));

				bRunOut = (AmountPtr != nullptr) && (*AmountPtr == 0);
				bCooldownBlocked = false;
				for (const auto& Cooldown : SoCharacter->GetCooldowns())
					if (Cooldown.Object == CurrentlySelectedSpell)
						bCooldownBlocked = true;

				UpdateInactiveFadeVisibility();
			}
		}

	UpdateVisibility();
}
