// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUISettingsKeyboard.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/InputSettings.h"
#include "Components/ScrollBox.h"
#include "Components/Overlay.h"

#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"
#include "Character/SoPlayerController.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"

#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/Buttons/SoUIButton.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/Menu/ConfirmPanels/SoUIConfirmKeyboardSettings.h"
#include "UI/Menu/Settings/KeySelector/SoUIInputKeySelectorOverlay.h"
#include "UI/SoUIHelper.h"
#include "Localization/SoLocalization.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::SynchronizeProperties()
{
	InputOptions = FSoInputConfigurableActionName::GetAllOptions();
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	OnlyKeyboardOverlay->SetVisibility(ESlateVisibility::Collapsed);

	// Events Hierarchy:
	// 1. SSoInputKeySelector
	// 2. USoUIInputKeySelector
	// 3. USoUIInputKeySelectorOverlay
	// 4. Self
	if (KeySelectorOverlay)
	{
		KeySelectorOverlay->OnIsSelectedKeyValidEvent() = BIND_UOBJECT_DELEGATE(USoUIInputKeySelectorOverlay::FSoIsSelectedKeyValidEvent, IsSelectedKeyValid);

		KeySelectorOverlay->OnOpenChangedEvent().AddDynamic(this, &ThisClass::OnKeySelectorOverlayOpenChanged);
		KeySelectorOverlay->OnKeySelectedEvent().AddDynamic(this, &ThisClass::HandleKeySelected);
		KeySelectorOverlay->OnIsSelectingKeyChangedEvent().AddDynamic(this, &ThisClass::HandleIsSelectingKeyChanged);
		KeySelectorOverlay->OnApplySelectedKeyEvent().AddDynamic(this, &ThisClass::HandleApplySelectedKey);
	}

	// Listen to events
	if (ConfirmKeyboardSettings)
	{
		ConfirmKeyboardSettings->OnOpenChangedEvent().AddDynamic(this, &ThisClass::OnConfirmPanelOpenChanged);
	}

	SubscribeToDeviceChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::NativeDestruct()
{
	if (KeySelectorOverlay)
	{
		KeySelectorOverlay->OnKeySelectedEvent().RemoveDynamic(this, &ThisClass::HandleKeySelected);
		KeySelectorOverlay->OnIsSelectingKeyChangedEvent().RemoveDynamic(this, &ThisClass::HandleIsSelectingKeyChanged);
		KeySelectorOverlay->OnApplySelectedKeyEvent().RemoveDynamic(this, &ThisClass::HandleApplySelectedKey);
		KeySelectorOverlay->OnOpenChangedEvent().RemoveDynamic(this, &ThisClass::OnKeySelectorOverlayOpenChanged);

	}
	UnSubscribeFromDeviceChanged();
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::Open_Implementation(bool bOpen)
{
	UserSettings->SetBlockConfigSave(true);

	// Force refresh
	UserSettings->ApplyInputSettings(true);

	// Reset on open
	if (bOpen)
	{
		UpdateCanEditKeyboardKeys();
		CreateInputButtonsFromUserSettings(0);
	}
	else
	{
		// Close all the child widgets
		ConfirmPanelClose();
		KeySelectorOverlay->Close();
	}

	UserSettings->SetBlockConfigSave(false);
	Super::Open_Implementation(bOpen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::CanBeInterrupted_Implementation() const
{
	if (CurrentState == ESoUISettingsKeyboardState::SelectingKey)
		return !KeySelectorOverlay->IsSelectingKey();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	EmptyLinesAndTitles();

	TArray<FText> KeyboardButtonTexts;
	for (const FSoInputConfigurableActionName& Option : InputOptions)
	{
		AddTitleText(Option.DisplayText);
		KeyboardButtonTexts.Add(FROM_STRING_TABLE_UI("unknown"));
	}

	if (USoUIButtonArray* ButtonsArray = GetWidgetsArrayAsButtons())
		ButtonsArray->CreateButtons(KeyboardButtonTexts);

	Super::CreateChildrenAndSetTexts(bFromSynchronizeProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::OnCommandResetAllToDefaults()
{
	if (CurrentState != ESoUISettingsKeyboardState::ResetAllToDefaults)
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsPressed));

	OpenConfirmPanel();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::OnCommandResetKeyToDefault()
{
	// The key is already default
	if (IsSelectedActionMappingDefault())
		return false;

	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsRestore));

	FInputActionKeyMapping ActionMapping;
	if (GetSelectedActionMapping(ActionMapping))
	{
		// This overwrites any existing ones
		UserSettings->SetInputActionMappingToDefault(ActionMapping, true, true);
		UserSettings->ApplyInputSettings(true);
	}

	// Keep the selected line
	CreateInputButtonsFromUserSettings(GetSelectedLineIndex());
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::HandleOnUICommand(ESoUICommand Command)
{
	if (!UpdateCanEditKeyboardKeys())
	{
		// Close this widget first
		if (Command == ESoUICommand::EUC_MainMenuBack)
			ISoUIEventHandler::Execute_Open(this, false);

		return true;
	}

	// Forward to the confirm preset panel
	if (IsConfirmMode())
		return ISoUIEventHandler::Execute_OnUICommand(ConfirmKeyboardSettings, Command);;

	// Forward commands to the key selector
	if (CurrentState == ESoUISettingsKeyboardState::SelectingKey)
		return ISoUIEventHandler::Execute_OnUICommand(KeySelectorOverlay, Command);;

	if (OnPressedButtonTooltipsCommand(Command))
		return true;

	// Handle command in parent
	return Super::HandleOnUICommand(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	// NOTE: don't call parent

	// Set the correct action name
	if (InputOptions.IsValidIndex(SelectedChild))
		KeySelectorOverlay->SetActionName(InputOptions[SelectedChild].DisplayText);

	SetCurrentState(ESoUISettingsKeyboardState::SelectingKey);
	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsValueSwitch));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::RefreshButtonImagesTooltips(bool bForceShowAll)
{
	if (IsOnButtonTooltips())
		return;

	if (!ButtonImagesTooltipsArray)
		return;

	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	if (CurrentState != ESoUISettingsKeyboardState::SelectingKey)
	{
		// TODO do we really need this?
		// Apply/Reset input settings to for all inputs
		//if (DidSettingsChange())
		//{
			//CommandTooltips->AddTooltipFromUICommand(FROM_STRING_TABLE_UI("input_apply", "Apply"), UICommand_Apply);
		//}

		// TODO fix glitch because of mouse
		// if (bForceShowAll || !IsSelectedActionMappingDefault())
		// CommandsToBeDisplayed.Add({ UICommand_ResetKeyToDefault, FROM_STRING_TABLE_UI("settings_reset_input_key") });

		CommandsToBeDisplayed.Add({ UICommand_ResetKeyToArrows, FROM_STRING_TABLE_UI("reset_keyboard_arrows") });
		CommandsToBeDisplayed.Add({ UICommand_ResetAllToWASD, FROM_STRING_TABLE_UI("reset_keyboard_wasd") });
	}

	ButtonImagesTooltipsArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::OnLineChanged(int32 OldLineIndex, int32 NewLineIndex)
{
	// Activate new button
	// do not, because highlight is enough?!
	// KeyboardButtons->SetIsActiveOnButton(OldLineIndex, false);
	// KeyboardButtons->SetIsActiveOnButton(NewLineIndex, true);
	RefreshButtonImagesTooltips();

	// Scroll into view
	if (USoUIButton* Button = TitlesArray->GetContainerButtonAt(NewLineIndex))
	{
		ScrollBox->ScrollWidgetIntoView(Button, true, EDescendantScrollDestination::IntoView);
	}

	Super::OnLineChanged(OldLineIndex, NewLineIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	if (CurrentState == ESoUISettingsKeyboardState::SelectingKey)
	{
		// Forward commands to the key selector, check if we can handle another command
		return KeySelectorOverlay->OnPressedButtonTooltipsCommand(Command);;
	}

	switch (Command)
	{
		case UICommand_ResetAllToWASD:
		{
			ConfirmKeyboardSettings->SetOpenedForResetToWASD(true);
			OnCommandResetAllToDefaults();
			break;
		}
		case UICommand_ResetKeyToArrows:
		{
			ConfirmKeyboardSettings->SetOpenedForResetToWASD(false);
			OnCommandResetAllToDefaults();
			break;
		}

		default:
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::UpdateCanEditKeyboardKeys()
{
	// Stop user from modifying with gamepad
	if (SoController->IsLastInputFromGamepad())
	{
		KeySelectorOverlay->Close();
		ContentContainer->SetIsEnabled(false);
		OnlyKeyboardOverlay->SetVisibility(ESlateVisibility::Visible);
		return false;
	}

	OnlyKeyboardOverlay->SetVisibility(ESlateVisibility::Collapsed);
	ContentContainer->SetIsEnabled(true);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
{
	if (!bIsListeningToDevicesChanges || !bOpened)
		return;

	UpdateCanEditKeyboardKeys();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::SubscribeToDeviceChanged()
{
	if (bIsListeningToDevicesChanges)
		return;

	if (SoController)
	{
		SoController->OnDeviceTypeChanged().AddDynamic(this, &ThisClass::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::UnSubscribeFromDeviceChanged()
{
	if (!bIsListeningToDevicesChanges)
		return;

	if (SoController)
	{
		SoController->OnDeviceTypeChanged().RemoveDynamic(this, &ThisClass::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::CreateInputButtonsFromUserSettings(const int32 SelectLineIndex)
{
	USoUIButtonArray* ButtonsArray = GetWidgetsArrayAsButtons();
	if (ButtonsArray == nullptr)
		return;

	UserSettings->RefreshInputSettingsMappings();
	UnboundKeyboardKeys.Empty();
	KeyboardMappingOptions.Empty();

	TArray<FText> KeyboardButtonTexts;
	for (int32 Index = 0, Num = InputOptions.Num(); Index < Num; Index++)
	{
		const FSoInputConfigurableActionName& Option = InputOptions[Index];

		// Query the input settings for this configurable option
		const TArray<FInputActionKeyMapping> OptionMappings = UserSettings->GetKeyboardInputActionMappingsForActionName(Option.ActionName, true);

		if (OptionMappings.Num() == 0)
		{
			// Something is wrong
			KeyboardButtonTexts.Add(FROM_STRING_TABLE_UI("unknown"));
			KeyboardMappingOptions.Add(FInputActionKeyMapping());
		}
		else
		{
			bool bAlreadyAdded = false;
			for (const FInputActionKeyMapping& KeyMapping : OptionMappings)
			{
				// Multiple bindings for the same key?
				if (bAlreadyAdded)
				{
					UE_LOG(LogSoUISettings, Warning, TEXT("ActionName = %s also has an additional keyboard Key = %s. Ignoring "), *Option.ActionName.ToString(), *KeyMapping.Key.ToString());
					continue;
				}

				if (KeyMapping.Key.IsValid())
				{
					KeyboardButtonTexts.Add(FSoInputKey::GetKeyDisplayName(KeyMapping.Key));
				}
				else
				{
					UnboundKeyboardKeys.Add(Index);
					KeyboardButtonTexts.Add(FROM_STRING_TABLE_UI("input_unbound"));
				}

				KeyboardMappingOptions.Add(KeyMapping);
				bAlreadyAdded = true;
			}
		}
	}

	ButtonsArray->CreateButtons(KeyboardButtonTexts);
	if (InputOptions.Num() != NumLines())
	{
		// Here be dragons
		UE_LOG(LogSoUISettings,
			   Fatal,
			   TEXT("InputOptions.Num(%d) != KeyboardButtons->GetElementNum(%) This would most likely make the input settings not work anymore"),
			   InputOptions.Num(), NumLines());
	}
	if (InputOptions.Num() != KeyboardMappingOptions.Num())
	{
		// Here be vampires
		UE_LOG(LogSoUISettings,
			Fatal,
			TEXT("InputOptions.Num(%d) != KeyboardMappingOptions.Num(%) This would most likely make the input settings not work anymore"),
			InputOptions.Num(), KeyboardMappingOptions.Num());
	}

	// Select line
	if (SelectLineIndex != INDEX_NONE)
	{
		SetSelectedLineIndex(SelectLineIndex);
	}

	// BP
	ReceiveOnCreateInputButtonsFromUserSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::IsSelectedActionMappingDefault() const
{
	FInputActionKeyMapping ActionMapping;
	if (GetSelectedActionMapping(ActionMapping))
		return UserSettings->IsInputActionMappingDefault(ActionMapping, true);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::GetSelectedActionMapping(FInputActionKeyMapping& ActionMapping) const
{
	ActionMapping = {};
	const int32 SelectedButtonIndex = GetSelectedLineIndex();
	if (!KeyboardMappingOptions.IsValidIndex(SelectedButtonIndex))
		return false;

	ActionMapping = KeyboardMappingOptions[SelectedButtonIndex];
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::DidSettingsChange() const
{
	return ChangedKeyboardKeys.Num() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::HandleIsSelectingKeyChanged(bool bIsSelectingKey)
{
	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::HandleKeySelected(const FInputChord& SelectedKey)
{
	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::HandleApplySelectedKey(const FInputChord& SelectedKey)
{
	// Check here, maybe it escaped us somehow?
	FText ErrorMessage, WarningMessage;
	if (!IsSelectedKeyValid(SelectedKey, ErrorMessage, WarningMessage))
	{
		UE_LOG(
			LogSoUISettings,
			Error,
			TEXT("HandleApplySelectedKey: SHOULD NOT HAVE REACHED HERE. ErrorMessage = %s, WarningMessage = %s"),
			*ErrorMessage.ToString(), *WarningMessage.ToString()
		);
		return;
	}

	const int32 SelectedButtonIndex = GetSelectedLineIndex();
	if (!KeyboardMappingOptions.IsValidIndex(SelectedButtonIndex))
	{
		UE_LOG(LogSoUISettings, Error, TEXT("HandleApplySelectedKey: KeyboardMappingOptions does not have selected index = %d"), SelectedButtonIndex);
		return;
	}

	// Try to rebind
	const FInputActionKeyMapping OldKeyMapping = KeyboardMappingOptions[SelectedButtonIndex];
	const FName SelectedActionName = OldKeyMapping.ActionName;
	if (UserSettings->RebindInputKeyToInputChord(OldKeyMapping, SelectedKey, true, true))
	{
		// Update UI
		UserSettings->ApplyInputSettings(true);
		CreateInputButtonsFromUserSettings(SelectedButtonIndex);
	}
	else
	{
		UE_LOG(
			LogSoUISettings,
			Warning,
			TEXT("HandleApplySelectedKey: Could not rebind input for ActionName = `%s` to Key = `%s` because reasons?"),
			 *SelectedActionName.ToString(), *SelectedKey.Key.ToString()
		);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::IsSelectedKeyValid(const FInputChord& SelectedKey, FText& OutError, FText& OutWarning, bool bNoOutput)
{
	if (!USoInputHelper::IsAllowedInputKeyboardKey(SelectedKey))
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Key"), FSoInputKey::GetKeyDisplayName(SelectedKey.Key));
		OutError = FText::Format(FROM_STRING_TABLE_UI("input_selected_key_not_allowed"), Arguments);
		if (!bNoOutput)
			UE_LOG(LogSoUISettings, Error, TEXT("%s"), *OutError.ToString());

		return false;
	}

	const int32 SelectedButtonIndex = GetSelectedLineIndex();
	if (!InputOptions.IsValidIndex(SelectedButtonIndex))
	{
		OutError = FText::FromString(
			FString::Printf(TEXT("ButtonIndex = %d does not exist in the InputOptions"), SelectedButtonIndex)
		);
		if (!bNoOutput)
			UE_LOG(LogSoUISettings, Error, TEXT("%s"), *OutError.ToString());

		return false;
	}

	// Key conflicts are allowed but warn the user
	const FName SelectedActionName = InputOptions[SelectedButtonIndex].ActionName;
	if (USoUIHelper::AreThereAnyKeyConflicts(UserSettings, SelectedActionName, SelectedKey, OutWarning))
	{
		if (!bNoOutput)
			UE_LOG(LogSoUISettings, Warning, TEXT("%s"), *OutWarning.ToString());
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::SetCurrentState(ESoUISettingsKeyboardState InCurrentState)
{
	CurrentState = InCurrentState;
	bool bOpenConfirmKeyboardSettings = false;
	bAllowChangingLines = true;
	ScrollBox->SetIsEnabled(true);

	switch (CurrentState)
	{
		case ESoUISettingsKeyboardState::Browse:
		{
			break;
		}

		case ESoUISettingsKeyboardState::SelectingKey:
		{
			bAllowChangingLines = false;
			ScrollBox->SetIsEnabled(false);
			KeySelectorOverlay->Open(true);
			break;
		}

		case ESoUISettingsKeyboardState::ResetAllToDefaults:
		{
			bOpenConfirmKeyboardSettings = true;
			ScrollBox->SetIsEnabled(false);
			break;
		}

		default:
			checkNoEntry();
	}

	// Refresh tooltips
	EmptyAndRefreshButtonImagesTooltips(true);

	// Open confirm keyboard settings only when resetting all defaults
	ISoUIEventHandler::Execute_Open(ConfirmKeyboardSettings, bOpenConfirmKeyboardSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::OnKeySelectorOverlayOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		// Closed, return back to own state
		if (!KeySelectorOverlay->CanHandleUICommand())
			SetCurrentState(ESoUISettingsKeyboardState::Browse);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::OnConfirmPanelOpenChanged(bool bPanelOpened)
{
	if (bPanelOpened)
	{
		ButtonImagesTooltipsArray->Hide();
	}
	else
	{
		ButtonImagesTooltipsArray->Show();
		CreateInputButtonsFromUserSettings(0);
		ScrollBox->SetIsEnabled(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsKeyboard::ConfirmPanelClose()
{
	SetCurrentState(ESoUISettingsKeyboardState::Browse);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::IsConfirmMode() const
{
	return ::IsValid(ConfirmKeyboardSettings) && ConfirmKeyboardSettings->CanHandleUICommand();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsKeyboard::OpenConfirmPanel()
{
	SetCurrentState(ESoUISettingsKeyboardState::ResetAllToDefaults);
	return true;
}
