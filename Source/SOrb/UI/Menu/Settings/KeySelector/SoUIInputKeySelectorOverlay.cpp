// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIInputKeySelectorOverlay.h"

#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"

#include "SoUIInputKeySelector.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "Components/Image.h"
#include "UI/SoUIHelper.h"

#include "Settings/Input/SoInputHelper.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"
#include "Localization/SoLocalization.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (KeySelector)
	{
		KeySelector->SetAllowModifierKeysInSelection(bAllowModifierKeysInSelection);
		KeySelector->SetEscapeKeys(EscapeKeys);
		KeySelector->SetAllowedInputType(AllowedInputType);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	KeySelectorImageRepresentation->SetVisibility(ESlateVisibility::Collapsed);
	ErrorMessage->SetVisibility(ESlateVisibility::Collapsed);

	if (KeySelector)
	{
		KeySelector->OnKeySelectedEvent().AddDynamic(this, &ThisClass::HandleKeySelected);
		KeySelector->OnIsSelectingKeyChangedEvent().AddDynamic(this, &ThisClass::HandleIsSelectingKeyChanged);
	}

	if (ButtonImagesTooltipsArray)
	{
		ButtonImagesTooltipsArray->OnNavigateOnPressedHandleChildEvent().BindLambda([this](int32 SelectedChild, USoUIUserWidget* SoUserWidget)
		{
			const ESoUICommand PressedCommand = ButtonImagesTooltipsArray->GetButtonIndexCommand(SelectedChild);
			OnPressedButtonTooltipsCommand(PressedCommand);
		});
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::NativeDestruct()
{
	if (KeySelector)
	{
		KeySelector->OnKeySelectedEvent().RemoveDynamic(this, &ThisClass::HandleKeySelected);
		KeySelector->OnIsSelectingKeyChangedEvent().RemoveDynamic(this, &ThisClass::HandleIsSelectingKeyChanged);
	}

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelectorOverlay::OnUICommand_Implementation(ESoUICommand Command)
{
	if (!CanHandleUICommand())
		return false;

	switch (Command)
	{
		case ESoUICommand::EUC_Left:
		case ESoUICommand::EUC_Right:
			return ButtonImagesTooltipsArray->Navigate(Command);

		case ESoUICommand::EUC_MainMenuEnter:
		{
			const ESoUICommand PressedCommand = ButtonImagesTooltipsArray->GetSelectedButtonCommand();
			return OnPressedButtonTooltipsCommand(PressedCommand);
		}

		default:
			return OnPressedButtonTooltipsCommand(Command);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelectorOverlay::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	if (!CanHandleUICommand())
		return false;

	switch (Command)
	{
		case UICommand_SelectAnother:
			return OnCommandSelectAnotherKey();

		case UICommand_Apply:
			return OnCommandApply();

		case UICommand_Cancel:
			return OnCommandCancel();

		default:
			break;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelectorOverlay::OnCommandSelectAnotherKey()
{
	if (!CanHandleUICommand())
		return false;

	// Choose another key
	SetIsSelectingKey(true);

	CurrentlySelectedButtonText->SetVisibility(ESlateVisibility::Visible);
	KeySelector->SetTextBlockVisibility(ESlateVisibility::Visible);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelectorOverlay::OnCommandApply()
{
	if (!CanHandleUICommand())
		return false;

	if (!IsSelectedKeyValid())
		return false;

	// Close overlay
	Close();
	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsApply));

	ApplySelectedKeyEvent.Broadcast(KeySelector->GetSelectedKey());
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelectorOverlay::OnCommandCancel()
{
	if (IsSelectingKey())
		return false;

	// Close overlay
	Close();
	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsRestore));

	CancelSelectedKeyEvent.Broadcast();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::Open_Implementation(bool bOpen)
{
	bOpened = bOpen;
	ErrorMessage->SetVisibility(ESlateVisibility::Collapsed);
	WarningMessage->SetVisibility(ESlateVisibility::Collapsed);
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bOpen)
	{
		const ESoInputDeviceType DeviceType = USoInputHelper::GetCurrentGamepadDeviceTypeFromSettings(this);
		GamepadDeviceType = (DeviceType == ESoInputDeviceType::Gamepad_PlayStation || DeviceType == ESoInputDeviceType::Gamepad_Switch) ?
							DeviceType : ESoInputDeviceType::Gamepad_Xbox;
	}
	else
	{
		SetIsSelectingKey(false);
	}

	OpenChangedEvent.Broadcast(bOpened);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::HandleIsSelectingKeyChanged(bool bIsSelectingKey)
{
	// We are trying another key, hide this message
	if (bIsSelectingKey)
	{
		ErrorMessage->SetVisibility(ESlateVisibility::Collapsed);
		WarningMessage->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		ButtonImagesTooltipsArray->Highlight();
		// Maybe some inputs are still pressed, clear so that we don't go into issues
		//if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		//	Character->ClearInputStates();
	}

	UpdateKeySelectorImage();
	RefreshButtonImagesTooltips();
	IsSelectingKeyChangedEvent.Broadcast(bIsSelectingKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::HandleKeySelected(const FInputChord& InSelectedKey)
{
	CurrentlySelectedButtonText->SetVisibility(ESlateVisibility::Visible);
	KeySelector->SetTextBlockVisibility(ESlateVisibility::Visible);

	// Display error message
	FText Error, Warning;
	if (IsSelectedKeyValid(Error, Warning))
	{
		ErrorMessage->SetVisibility(ESlateVisibility::Collapsed);
		WarningMessage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!Error.IsEmpty())
	{
		// Hide any warnings
		ErrorMessage->SetText(Error);
		ErrorMessage->SetVisibility(ESlateVisibility::Visible);
		WarningMessage->SetVisibility(ESlateVisibility::Collapsed);
		CurrentlySelectedButtonText->SetVisibility(ESlateVisibility::Collapsed);
		KeySelector->SetTextBlockVisibility(ESlateVisibility::Hidden);
	}
	else if (!Warning.IsEmpty())
	{
		WarningMessage->SetVisibility(ESlateVisibility::Visible);
		WarningMessage->SetText(Warning);
	}

	UpdateKeySelectorImage();
	RefreshButtonImagesTooltips();
	KeySelectedEvent.Broadcast(InSelectedKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelectorOverlay::IsSelectingKey() const
{
	return KeySelector ? KeySelector->IsSelectingKey() : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FKey USoUIInputKeySelectorOverlay::GetSelectedKey() const
{
	return KeySelector ? KeySelector->GetSelectedKey().Key : FKey();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::SetActionName(const FText& ActionNameText)
{
	if (ActionName)
		ActionName->SetText(ActionNameText);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::SetIsSelectingKey(bool bInIsSelectingKey)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("\n\n\nUSoUIInputKeySelectorOverlay::SetIsSelectingKey = %d"), bInIsSelectingKey);

	if (KeySelector)
	{
		// Focus back to us
		if (bInIsSelectingKey)
			KeySelector->SetPreviousFocusedWidget(FSlateApplication::Get().GetUserFocusedWidget(0));

		KeySelector->SetIsSelectingKey(bInIsSelectingKey);
		UpdateKeySelectorImage();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::RefreshButtonImagesTooltips()
{
	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	if (!IsSelectingKey())
	{
		if (IsSelectedKeyValid())
			CommandsToBeDisplayed.Add({ UICommand_Apply, FROM_STRING_TABLE_UI("settings_apply") });

		CommandsToBeDisplayed.Add({ UICommand_SelectAnother, FROM_STRING_TABLE_UI("settings_select_another") });
		CommandsToBeDisplayed.Add({ UICommand_Cancel, FROM_STRING_TABLE_UI("settings_cancel") });

	}

	ButtonImagesTooltipsArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed, true);
	if (!IsSelectingKey())
		ButtonImagesTooltipsArray->SelectFirstValidChild();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIInputKeySelectorOverlay::IsSelectedKeyValid(FText& OutError, FText& OutWarning, bool bNoOutput) const
{
	if (!KeySelector || !IsSelectedKeyValidEvent.IsBound())
		return false;

	const FInputChord SelectedKey = KeySelector->GetSelectedKey();
	return IsSelectedKeyValidEvent.Execute(SelectedKey, OutError, OutWarning, bNoOutput);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIInputKeySelectorOverlay::UpdateKeySelectorImage()
{
	if (!KeySelectorImageRepresentation)
		return;

	KeySelectorImageRepresentation->SetVisibility(ESlateVisibility::Collapsed);
	if (!IsSelectingKey() && IsSelectedKeyValid())
	{
		const FKey SelectedKey = GetSelectedKey();
		switch (KeySelectorImageVisibility)
		{
		case ESoInputOverlayKeyImageVisibility::All:
			// TODO
			break;

		case ESoInputOverlayKeyImageVisibility::Gamepad:
			if (USoInputHelper::IsGamepadKey(SelectedKey))
			{
				KeySelectorImageRepresentation->SetVisibility(ESlateVisibility::Visible);
				KeySelectorImageRepresentation->SetBrushFromTexture(USoGameSingleton::GetIconForInputKey(SelectedKey, GamepadDeviceType, false));
			}
			break;

		case ESoInputOverlayKeyImageVisibility::Keyboard:
			// TODO
			break;

		default:
			break;
		}
	}
}
