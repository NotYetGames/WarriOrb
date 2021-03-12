// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISpellCastSwitchSlot.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterBase.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoPlayerCharacterSheet.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateFromState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::NativeDestruct()
{
	UnsubscribeFromEvents();
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	UpdateFromState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::SetSpell(const USoItemTemplateRuneStone* InActiveItem)
{
	ActiveSpell = InActiveItem;
	UpdateFromState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::Reset()
{
	ActiveSpell = nullptr;
	bIsSelected = false;
	UpdateFromState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::UpdateFromState()
{
	if (ActiveSpell)
	{
		SubscribeToEvents();
		RefreshActiveSpellState();
		Background->SetBrushFromTexture(SpellBackground);
		Icon->SetVisibility(ESlateVisibility::Visible);
		Icon->SetBrushFromTexture(ActiveSpell->GetIcon());
		Border->SetColorAndOpacity(bIsSelected ? ActiveSelectedBorderColor : ActiveUnSelectedBorderColor);
	}
	else
	{
		UnsubscribeFromEvents();
		Background->SetBrushFromTexture(EmptyBackground);
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		AmountText->SetVisibility(ESlateVisibility::Collapsed);
		InactiveFade->SetVisibility(ESlateVisibility::Collapsed);
		Border->SetColorAndOpacity(bIsSelected ? InActiveSelectedBorderColor : InActiveUnSelectedBorderColor);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::SubscribeToEvents()
{
	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		SoCharacter->OnCooldownStarted().AddUniqueDynamic(this, &Self::OnCooldownChanged);
		SoCharacter->OnCooldownEnded().AddUniqueDynamic(this, &Self::OnCooldownChanged);
		SoCharacter->OnMovementModeChangedNotify.AddUniqueDynamic(this, &Self::OnSpellUsabilityMightChanged);
		SoCharacter->OnCanUseSoulKeeperChanged().AddUniqueDynamic(this, &Self::OnSpellUsabilityMightChanged);
		//SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellChanged.AddDynamic(this, &Self::OnSelectedSpellChanged);
		//SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellSwitched.AddDynamic(this, &Self::OnSelectedSpellChanged);
		//SoCharacter->GetPlayerCharacterSheet()->OnSlotChanged.AddDynamic(this, &Self::OnCharacterItemSlotChanged);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::UnsubscribeFromEvents()
{
	if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		SoCharacter->OnCooldownStarted().RemoveDynamic(this, &Self::OnCooldownChanged);
		SoCharacter->OnCooldownEnded().RemoveDynamic(this, &Self::OnCooldownChanged);
		SoCharacter->OnMovementModeChangedNotify.RemoveDynamic(this, &Self::OnSpellUsabilityMightChanged);
		SoCharacter->OnCanUseSoulKeeperChanged().RemoveDynamic(this, &Self::OnSpellUsabilityMightChanged);
		//SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellChanged.RemoveDynamic(this, &Self::OnSelectedSpellChanged);
		//SoCharacter->GetPlayerCharacterSheet()->OnSelectedSpellSwitched.RemoveDynamic(this, &Self::OnSelectedSpellChanged);
		//SoCharacter->GetPlayerCharacterSheet()->OnSlotChanged.RemoveDynamic(this, &Self::OnCharacterItemSlotChanged);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::UpdateInactiveFadeVisibility()
{
	if (!ActiveSpell)
	{
		InactiveFade->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (!InactiveFade)
		return;

	const ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!SoCharacter)
		return;

	const bool bNotOnGroundBlocked = !ActiveSpell->bCanBeUsedInAir && !SoCharacter->GetSoMovement()->IsMovingOnGround();
	const bool bNoSKZoneBlocked = !ActiveSpell->bCanBeUsedInSKFreeZone && !SoCharacter->CanUseSoulkeeperAtLocation();
	InactiveFade->SetVisibility((bCooldownBlocked || bRunOut || bNotOnGroundBlocked || bNoSKZoneBlocked) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::RefreshActiveSpellState()
{
	if (ActiveSpell)
	{
		if (const ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		{
			if (const USoPlayerCharacterSheet* SoSheet = SoCharacter->GetPlayerCharacterSheet())
			{
				// Update spell text amount
				const int32* AmountPtr = SoSheet->GetSpellMap().Find(ActiveSpell);
				AmountText->SetVisibility(AmountPtr ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
				bRunOut = false;
				if (AmountPtr)
				{
					bRunOut = *AmountPtr == 0;
					AmountText->SetText(FText::AsNumber(*AmountPtr, &FNumberFormattingOptions::DefaultNoGrouping()));
				}

				// Check if blocked
				bCooldownBlocked = false;
				for (const auto& Cooldown : SoCharacter->GetCooldowns())
					if (Cooldown.Object == ActiveSpell)
						bCooldownBlocked = true;
			}
		}
	}

	UpdateInactiveFadeVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::OnCooldownChanged(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown)
{
	if (ActiveSpell && ActiveSpell == ObjectWithCooldown)
	{
		bCooldownBlocked = RemainingTime > 0.0f;
		UpdateInactiveFadeVisibility();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitchSlot::OnSpellUsabilityMightChanged()
{
	if (ActiveSpell)
		UpdateInactiveFadeVisibility();
}
