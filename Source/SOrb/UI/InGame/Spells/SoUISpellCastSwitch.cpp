// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISpellCastSwitch.h"

#include "Components/WrapBox.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/SoGameInstance.h"
#include "Basic/SoAudioManager.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Character/SoPlayerController.h"

#include "SoUISpellSlot.h"
#include "SoUISpellCastSwitchSlot.h"
#include "UI/General/Commands/SoUICommandTooltip.h"
#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	KeyboardShortcutsContainer->SetVisibility(ESlateVisibility::Collapsed);

	SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	SoController = SoCharacter->GetSoPlayerController();
	SoSheet = SoCharacter->GetPlayerCharacterSheet();
	verify(SoCharacter);
	verify(SoController);
	verify(SoSheet);

	TooltipSelect->SetUICommand(ESoUICommand::EUC_SpellSelect);
	TooltipSelectAndCast->SetUICommand(ESoUICommand::EUC_SpellSelectAndCast);
	SubscribeToDeviceChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::NativeDestruct()
{
	UnSubscribeFromDeviceChanged();
	SoCharacter = nullptr;
	SoController = nullptr;
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellCastSwitch::SetInGameActivityEnabled_Implementation(UObject *Source, bool bEnable)
{
	if (bEnable && bOpened)
	{
		UE_LOG(LogSoUIActivity, Warning, TEXT("SpellCastSwitch reject, already opened"));
		return true;
	}

	SetOpen(bEnable);
	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellCastSwitch::HandleCommand_Implementation(ESoUICommand Command)
{
	if (!bOpened)
		return true;

	if (!bIgnoredFirstUICommand && Command == ESoUICommand::EUC_SpellSelect)
	{
		bIgnoredFirstUICommand = true;
		return true;
	}

	const bool bMultipleCommandsPressed = SoCharacter->GetNumUIInputsPressed() >= 2;
	bool bSpellKeyboardQuickCommand = false;

	int32 NewIndex = CurrentCircleSpellSlotIndex;
	switch (Command)
	{
	case ESoUICommand::EUC_Left:
		if (!bQuickSelectionMode)
			NewIndex = USoMathHelper::WrapIndexAround(CurrentCircleSpellSlotIndex - 1, CircleLinearSpellsForEquipped.Num());
		break;

	case ESoUICommand::EUC_Right:
		if (!bQuickSelectionMode)
			NewIndex = USoMathHelper::WrapIndexAround(CurrentCircleSpellSlotIndex + 1, CircleLinearSpellsForEquipped.Num());
		break;

	case ESoUICommand::EUC_Up:
		if (!bQuickSelectionMode)
			NewIndex = USoMathHelper::WrapIndexAround(CurrentCircleSpellSlotIndex - 1, CircleLinearSpellsForEquipped.Num());
		break;

	case ESoUICommand::EUC_Down:
		if (!bQuickSelectionMode)
			NewIndex = USoMathHelper::WrapIndexAround(CurrentCircleSpellSlotIndex + 1, CircleLinearSpellsForEquipped.Num());
		break;

	case ESoUICommand::EUC_SpellLeft:
		if (!bQuickSelectionMode)
		{
			bSpellKeyboardQuickCommand = true;
			NewIndex = GetCircleIndexForSpellSlot(Slot_W);
		}
		break;

	case ESoUICommand::EUC_SpellRight:
		if (!bQuickSelectionMode)
		{
			bSpellKeyboardQuickCommand = true;
			NewIndex = GetCircleIndexForSpellSlot(Slot_E);
		}
		break;

	case ESoUICommand::EUC_SpellUp:
		if (!bQuickSelectionMode)
		{
			bSpellKeyboardQuickCommand = true;
			NewIndex = GetCircleIndexForSpellSlot(Slot_N);
		}
		break;

	case ESoUICommand::EUC_SpellDown:
		if (!bQuickSelectionMode)
		{
			bSpellKeyboardQuickCommand = true;
			NewIndex = GetCircleIndexForSpellSlot(Slot_S);
		}
		break;

	case ESoUICommand::EUC_SpellSelectAndCast:
		SetOpen(false);

		// Cast the spell
		if (SoSheet->CanCastActiveSpell())
		{
			SoSheet->UseActiveRuneStoneIfCastable();
			SoSheet->OnItemUsed.Broadcast();

			// Disable hint usage
			IDlgDialogueParticipant::Execute_ModifyBoolValue(SoCharacter, SpellSelectedAndCastedName, true);

			USoAudioManager::PlaySound2D(this, SFXSelectAndCast);
		}
		break;

	case ESoUICommand::EUC_SpellSelect:
		SetOpen(false);
		if (SoSheet->CanCastActiveSpell())
		{
			// Disable hint usage
			IDlgDialogueParticipant::Execute_ModifyBoolValue(SoCharacter, SpellSelectedName, true);
			USoAudioManager::PlaySound2D(this, SFXSelect);
		}
		break;

	case ESoUICommand::EUC_MainMenuBack:
		SetOpen(false);
		break;

	default:
		break;
	}

	if (bSpellKeyboardQuickCommand)
	{
		// Check diagonals
		if (bMultipleCommandsPressed)
		{
			if (SoCharacter->AreUIInputsPressed({ ESoUICommand::EUC_SpellUp, ESoUICommand::EUC_SpellLeft }))
				NewIndex = GetCircleIndexForSpellSlot(Slot_NW);
			else if (SoCharacter->AreUIInputsPressed({ ESoUICommand::EUC_SpellUp, ESoUICommand::EUC_SpellRight }))
				NewIndex = GetCircleIndexForSpellSlot(Slot_NE);
			else if (SoCharacter->AreUIInputsPressed({ ESoUICommand::EUC_SpellDown, ESoUICommand::EUC_SpellLeft }))
				NewIndex = GetCircleIndexForSpellSlot(Slot_SW);
			else if (SoCharacter->AreUIInputsPressed({ ESoUICommand::EUC_SpellDown, ESoUICommand::EUC_SpellRight }))
				NewIndex = GetCircleIndexForSpellSlot(Slot_SE);
		}

		// Disable hint usage for the next time the panel is open
		if (SoController->GetCurrentDeviceType() == ESoInputDeviceType::Keyboard)
		{
			IDlgDialogueParticipant::Execute_ModifyBoolValue(SoCharacter, KeyboardQuickSelectSpellsName, true);
		}
	}

	// Move focus
	if (NewIndex != CurrentCircleSpellSlotIndex)
	{
		USoAudioManager::PlaySound2D(this, SFXNavigate);
		SetCurrentCircleSpellIndex(NewIndex);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellCastSwitch::Update_Implementation(float DeltaSeconds)
{
	// Gamepad right thumb stick
	const bool bUsingRightAxis = SoCharacter->IsUsingRightThumbStick();
	// const bool bUsingLeftAxis = SoCharacter->IsUsingLeftThumbStick();
	if (bOpened && bUsingRightAxis)
	{
		TickShouldCloseQuickSelectionStart = 0.f;
		//bFastChangingNeighbour = CountChanges >= 2;
		//if (bFastChangingNeighbour)
		//	CountChanges = 0;

		// https://en.wikipedia.org/wiki/Atan2#/media/File:Atan2_60.svg
		// Normalize to top values are negative and bottom values are positive

		// Already normalized
		const float DirectionDegrees = SoCharacter->GetGamepadRightDirectionDegrees();
		const float DifferenceFromPreviousTick = FMath::Abs(FMath::FindDeltaAngleDegrees(DirectionDegrees, PreviousTickDirectionDegrees));
		PreviousTickDirectionDegrees = DirectionDegrees;

		// Split in 8 parts, so 45 degrees, we verify in both directions
		// 0, 45, -45, 90, -90, 135, -135, -180 == 180

		// Normally it would have been 45.f / 2.f but this does not leave room distance between spell
		//static constexpr float DirectionDelta = 45.f / 7.5f;
		//static constexpr float DirectionDeltaDiagonals = 45.f / 4.f;

		static constexpr float DirectionDelta = 45.f / 4.f;
		static constexpr float DirectionDeltaDiagonals = 45.f / 2.f;

		// Prevent snapping
		static constexpr float ThresholdSnap = DirectionDelta * 3.f;
		if (DifferenceFromPreviousTick > ThresholdSnap)
		{
			// Early exit
			static constexpr int32 Frames = 2;
			if (!SoCharacter->IsUsingRightThumbStickWithSameValueForAtLeastFrames(Frames))
			{
	/*			if (!bFastChangingNeighbour)
				{
					UE_LOG(LogTemp, Warning, TEXT("USoUISpellCastSwitch Ignoring because bFastChangingNeighbour = %d AND CountChanges = %d"), bFastChangingNeighbour, CountChanges);
				}
				else
				{*/
					//UE_LOG(LogTemp, Warning, TEXT("USoUISpellCastSwitch Ignoring because DifferenceFromPreviousTick (%f) > ThresholdSnap (%f)"), DifferenceFromPreviousTick, ThresholdSnap);
				//}
				return bOpened;
			}
		}

		static constexpr float Degrees_E = 0.f;
		static constexpr float Degrees_SE = 45.f;
		static constexpr float Degrees_S = 90.f;
		static constexpr float Degrees_SW = 135.f;
		static constexpr float Degrees_W = 180.f;

		static constexpr float Degrees_NW = -Degrees_SW;
		static constexpr float Degrees_N = -Degrees_S;
		static constexpr float Degrees_NE = -Degrees_SE;

		int32 NewIndex = CurrentCircleSpellSlotIndex;
		if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_N - DirectionDelta, Degrees_N + DirectionDelta))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_N);
		}
		else if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_NE - DirectionDeltaDiagonals, Degrees_NE + DirectionDeltaDiagonals))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_NE);
		}
		else if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_E - DirectionDelta, Degrees_E + DirectionDelta))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_E);
		}
		else if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_SE - DirectionDeltaDiagonals, Degrees_SE + DirectionDeltaDiagonals))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_SE);
		}
		else if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_S - DirectionDelta, Degrees_S + DirectionDelta))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_S);
		}
		else if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_SW - DirectionDeltaDiagonals, Degrees_SW + DirectionDeltaDiagonals))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_SW);
		}
		else if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_NW - DirectionDeltaDiagonals, Degrees_NW + DirectionDeltaDiagonals))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_NW);
		}

		// Check both variants -180 and 180
		else if (USoMathHelper::IsInClosedInterval(DirectionDegrees, Degrees_W - DirectionDelta, Degrees_W + DirectionDelta)
			|| USoMathHelper::IsInClosedInterval(DirectionDegrees, -Degrees_W - DirectionDelta, -Degrees_W + DirectionDelta))
		{
			NewIndex = GetCircleIndexForSpellSlot(Slot_W);
		}

		if (NewIndex != CurrentCircleSpellSlotIndex)
			USoAudioManager::PlaySound2D(this, SFXNavigate);

		SetCurrentCircleSpellIndex(NewIndex);
	}
	else
	{
		// Not using thumbstick
		CountChanges = 0;
		bFastChangingNeighbour = false;

		if (bQuickSelectionMode && bOpened)
		{
			// NOTE: we don't use here the delta as the time is dilated
			if (FMath::IsNearlyZero(TickShouldCloseQuickSelectionStart))
			{
				// First Time
				TickShouldCloseQuickSelectionStart = GetWorld()->GetRealTimeSeconds();
			}
			else
			{
				// Can we close it already?
				const float TimePassed = GetWorld()->GetRealTimeSeconds() - TickShouldCloseQuickSelectionStart;
				if (TimePassed > ThresholdShouldCloseQuickSelection)
					SetOpen(false);
			}
		}
	}

	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::SetQuickSelectionMode(bool bInValue)
{
	bQuickSelectionMode = bInValue;

	// Don't ignore if quick selection mode
	bIgnoredFirstUICommand = bQuickSelectionMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::SetOpen(bool bOpen)
{
	if (bOpened == bOpen)
		return;

	USoAudioManager::PlaySound2D(this, bOpen ? SFXOnOpen : SFXOnClose);

	bOpened = bOpen;
	TickShouldCloseQuickSelectionStart = 0.f;

	USoAudioManager::Get(this).SetPlaybackSpeed(
		bOpen ? AudioSpeedMultiplier : 1.0f,
		bOpen ? AudioSlowDownTimeIn : AudioSlowDownTimeOut
	);

	if (bOpened)
	{
		// Open
		TickOpenAgainQuickSelectionStart = 0.f;

		// Figure out if there is a key conflict between ToggleSpell and SpellSelect for a keyboard
		const bool bIsThereAKeyConflict = USoGameSettings::Get().DoActionNamesHaveTheSameKey(
			FSoInputActionName::ToggleSpell,
			FSoInputActionName::UICommandToActionName(ESoUICommand::EUC_SpellSelect),
			true
		);

		if (USoInputHelper::IsLastInputFromKeyboard(this))
			// Keyboard
			// NOTE: does not have have quick selection mode
			bIgnoredFirstUICommand = !bIsThereAKeyConflict;
		else
			// Gamepad, in quick selection mode we don't ignore the first UI command
			bIgnoredFirstUICommand = bQuickSelectionMode;

		USoGameSettings::Get().SetTemporaryGameSpeed(0.1f);
		ReinitializeSpells();
	}
	else
	{
		// Close
		TickOpenAgainQuickSelectionStart = GetWorld()->GetRealTimeSeconds();

		// Reset game speed to setting values
		USoGameSettings::Get().ApplyGameSettings(true);

		// Sync spell index
		SoSheet->SetActiveSpellIndex(ConvertCircleSpellIndexToTrueIndex(CurrentCircleSpellSlotIndex));
	}

	SetVisibility(bOpened ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	UpdateHintsVisibilities();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISpellCastSwitch::CanBeOpened() const
{
	const float TimePassed = GetWorld()->GetRealTimeSeconds() - TickOpenAgainQuickSelectionStart;
	const bool bResult = TimePassed > ThresholdOpenAgainQuickSelection;
	//if (!bResult)
	//	UE_LOG(LogTemp, Warning, TEXT("CanBeOpened = false, TickOpenAgainQuickSelectionStart = %f, TimePassed = %f, ThresholdOpenAgainQuickSelection = %f"),
	//		TickOpenAgainQuickSelectionStart, TimePassed, ThresholdOpenAgainQuickSelection);

	return bResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISpellCastSwitch::ConvertCircleSpellIndexToTrueIndex(int32 CircleSlotIndex) const
{
	if (!CircleLinearSpellsForEquipped.IsValidIndex(CircleSlotIndex))
		return INDEX_NONE;

	const USoUISpellCastSwitchSlot* SpellSlot = CircleLinearSpellsForEquipped[CircleSlotIndex];
	if (SpellSlot == nullptr)
		return INDEX_NONE;

	return GetTrueIndexForSpellSlot(SpellSlot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISpellCastSwitch::ConvertTrueIndexToCircleSpellIndex(int32 TrueIndex) const
{
	if (!SpellsForEquipped.IsValidIndex(TrueIndex))
		return INDEX_NONE;

	const USoUISpellCastSwitchSlot* SpellSlot = SpellsForEquipped[TrueIndex];
	if (SpellSlot == nullptr)
		return INDEX_NONE;

	return GetCircleIndexForSpellSlot(SpellSlot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISpellCastSwitch::GetCircleIndexForSpellSlot(const USoUISpellCastSwitchSlot* SpellSlot) const
{
	for (int32 Index = 0; Index < CircleLinearSpellsForEquipped.Num(); Index++)
		if (CircleLinearSpellsForEquipped[Index] == SpellSlot)
			return Index;

	return INDEX_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISpellCastSwitch::GetTrueIndexForSpellSlot(const USoUISpellCastSwitchSlot* SpellSlot) const
{
	for (int32 Index = 0; Index < SpellsForEquipped.Num(); Index++)
		if (SpellsForEquipped[Index] == SpellSlot)
			return Index;

	return INDEX_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::ReinitializeSpells()
{
	// Match the indices from character sheet
	SpellsForEquipped = {
		Slot_N, Slot_E, Slot_S, Slot_W, Slot_NW, Slot_NE, Slot_SE, Slot_SW
	};

	// Circle clockwise order
	CircleLinearSpellsForEquipped = {
		Slot_N, Slot_NE, Slot_E, Slot_SE, Slot_S, Slot_SW, Slot_W, Slot_NW
	};

	// Reset
	for (USoUISpellCastSwitchSlot* SpellSlot : CircleLinearSpellsForEquipped)
		SpellSlot->Reset();

	const int32 CurrentTrueIndex = SoSheet->GetActiveSpellIndex();

	const TArray<FSoItem>& EquippedRuneStones = SoSheet->GetEquippedSpells();
	for (int32 Index = 0; Index < EquippedRuneStones.Num() && Index < SpellsForEquipped.Num(); Index++)
	{
		const FSoItem& RuneStone = EquippedRuneStones[Index];
		const USoItemTemplateRuneStone* SpellTemplate = RuneStone.GetTemplateAsRuneStone();
		SpellsForEquipped[Index]->SetSpell(SpellTemplate);

		if (Index == CurrentTrueIndex)
		{
			SpellsForEquipped[Index]->SetSelected(true);
		}
	}

	SetCurrentCircleSpellIndex(ConvertTrueIndexToCircleSpellIndex(CurrentTrueIndex), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::SetCurrentCircleSpellIndex(int32 NewSpellIndex, bool bForce)
{
	// Same index, ignore
	if (!bForce && (CurrentCircleSpellSlotIndex == NewSpellIndex || !CircleLinearSpellsForEquipped.IsValidIndex(NewSpellIndex)))
		return;

	CountChanges++;
	CircleLinearSpellsForEquipped[CurrentCircleSpellSlotIndex]->SetSelected(false);
	CircleLinearSpellsForEquipped[NewSpellIndex]->SetSelected(true);
	CurrentCircleSpellSlotIndex = NewSpellIndex;

	// Set text for spell
	if (SelectedSpellNameText)
	{
		if (const USoItemTemplateRuneStone* Currentspell = CircleLinearSpellsForEquipped[CurrentCircleSpellSlotIndex]->GetSpell())
			SelectedSpellNameText->SetText(Currentspell->GetItemName());
		else
			SelectedSpellNameText->SetText(FText::GetEmpty());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::UpdateHintsVisibilities()
{
	const bool bWasSelectHintDisplayed = IDlgDialogueParticipant::Execute_GetBoolValue(SoCharacter, SpellSelectedName);
	const bool bWasSelectAndCastHintDisplayed = IDlgDialogueParticipant::Execute_GetBoolValue(SoCharacter, SpellSelectedAndCastedName);
	const bool bWasKeyboardQuickSelectionDisplayed = IDlgDialogueParticipant::Execute_GetBoolValue(SoCharacter, KeyboardQuickSelectSpellsName);
	const bool bCanCastActiveSpell = SoSheet->CanCastActiveSpell();

	TooltipSelect->SetVisibility(!bWasSelectHintDisplayed && bCanCastActiveSpell ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	TooltipSelectAndCast->SetVisibility(!bWasSelectAndCastHintDisplayed && bCanCastActiveSpell ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (SoController->GetCurrentDeviceType() == ESoInputDeviceType::Keyboard)
	{
		KeyboardShortcutsContainer->SetVisibility(!bWasKeyboardQuickSelectionDisplayed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	else
	{
		KeyboardShortcutsContainer->SetVisibility(ESlateVisibility::Collapsed);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
{
	UpdateHintsVisibilities();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::SubscribeToDeviceChanged()
{
	SoController->OnDeviceTypeChanged().AddUniqueDynamic(this, &Self::HandleDeviceTypeChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISpellCastSwitch::UnSubscribeFromDeviceChanged()
{
	SoController->OnDeviceTypeChanged().RemoveDynamic(this, &Self::HandleDeviceTypeChanged);
}
