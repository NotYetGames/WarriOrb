// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUISettingsControllerRemap.h"

#include "Components/Overlay.h"
#include "Components/PanelWidget.h"

#include "Character/SoCharacter.h"
#include "Character/SoPlayerController.h"

#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"

#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/Buttons/SoUIButtonImage.h"
#include "UI/SoUIHelper.h"
#include "KeySelector/SoUIInputKeySelectorOverlay.h"
#include "SoUISettingsKeyboard.h"
#include "Localization/SoLocalization.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "SoLocalization.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::SynchronizeProperties()
{
	InputOptions = FSoInputConfigurableActionName::GetGamepadOptions();
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	OnlyGamepadOverlay->SetVisibility(ESlateVisibility::Collapsed);

	// Events Hierarchy:
	// 1. SSoInputKeySelector
	// 2. USoUIInputKeySelector
	// 3. USoUIInputKeySelectorOverlay
	// 4. ThisClass
	if (KeySelectorOverlay)
	{
		KeySelectorOverlay->OnIsSelectedKeyValidEvent() = BIND_UOBJECT_DELEGATE(USoUIInputKeySelectorOverlay::FSoIsSelectedKeyValidEvent, IsSelectedKeyValid);

		KeySelectorOverlay->OnOpenChangedEvent().AddDynamic(this, &ThisClass::OnKeySelectorOverlayOpenChanged);
		KeySelectorOverlay->OnKeySelectedEvent().AddDynamic(this, &ThisClass::HandleKeySelected);
		KeySelectorOverlay->OnIsSelectingKeyChangedEvent().AddDynamic(this, &ThisClass::HandleIsSelectingKeyChanged);
		KeySelectorOverlay->OnApplySelectedKeyEvent().AddDynamic(this, &ThisClass::HandleApplySelectedKey);
	}

	SubscribeToDeviceChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::NativeDestruct()
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
void USoUISettingsControllerRemap::Open_Implementation(bool bOpen)
{
	// Reset on open
	if (bOpen)
	{
		USoPlatformHelper::DisallowMouse(this);
		UpdateCanEditGamepadButtons();
		CreateInputButtonsFromUserSettings(0);
	}
	else
	{
		// Close all the child widgets
		KeySelectorOverlay->Close();
		USoPlatformHelper::AllowMouse(this);
	}

	Super::Open_Implementation(bOpen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsControllerRemap::CanBeInterrupted_Implementation() const
{
	if (CurrentState == ESoUISettingsControllerRemapState::SelectingKey)
		return !KeySelectorOverlay->IsSelectingKey();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	EmptyLinesAndTitles();
	TArray<FText> GamepadButtonTexts;
	for (const FSoInputConfigurableActionName& Option : InputOptions)
	{
		AddTitleText(Option.DisplayText);
		GamepadButtonTexts.Add(Option.DisplayText);
	}

	if (USoUIButtonArray* ButtonsArray = GetWidgetsArrayAsButtons())
		ButtonsArray->CreateButtons(GamepadButtonTexts);

	Super::CreateChildrenAndSetTexts(bFromSynchronizeProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsControllerRemap::HandleOnUICommand(ESoUICommand Command)
{
	if (!UpdateCanEditGamepadButtons())
	{
		// Close this widget first
		if (Command == ESoUICommand::EUC_MainMenuBack)
			ISoUIEventHandler::Execute_Open(this, false);

		return true;
	}

	// Forward commands to the key selector
	if (CurrentState == ESoUISettingsControllerRemapState::SelectingKey)
		return ISoUIEventHandler::Execute_OnUICommand(KeySelectorOverlay, Command);

	if (OnPressedButtonTooltipsCommand(Command))
		return true;

	// Handle command in parent
	return Super::HandleOnUICommand(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::NavigateBackCommand()
{
	// Close this widget
	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsClose));
	ISoUIEventHandler::Execute_Open(this, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	// NOTE: don't call parent

	SetCurrentState(ESoUISettingsControllerRemapState::SelectingKey);
	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsValueSwitch));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsControllerRemap::NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::RefreshButtonImagesTooltips(bool bForceShowAll)
{
	if (!ButtonImagesTooltipsArray)
		return;

	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	if (CurrentState != ESoUISettingsControllerRemapState::SelectingKey)
	{
		CommandsToBeDisplayed.Add({ UICommand_ResetAllToDefaults, FROM_STRING_TABLE_UI("settings_reset_default") });
	}

	ButtonImagesTooltipsArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::OnLineChanged(int32 OldLineIndex, int32 NewLineIndex)
{
	RefreshButtonImagesTooltips();
	Super::OnLineChanged(OldLineIndex, NewLineIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsControllerRemap::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	if (CurrentState == ESoUISettingsControllerRemapState::SelectingKey)
	{
		// Forward commands to the key selector, check if we can handle another command
		return KeySelectorOverlay->OnPressedButtonTooltipsCommand(Command);;
	}

	switch (Command)
	{
	case UICommand_ResetAllToDefaults:
		OnCommandResetAllToDefaults();
		break;

	default:
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::OnCommandResetAllToDefaults()
{
	SetCurrentState(ESoUISettingsControllerRemapState::ResetAllToDefaults);
	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsRestore));

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::HandleIsSelectingKeyChanged(bool bIsSelectingKey)
{
	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::HandleKeySelected(const FInputChord& SelectedKey)
{
	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::OnKeySelectorOverlayOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		// Closed, return back to own state
		if (!KeySelectorOverlay->CanHandleUICommand())
			SetCurrentState(ESoUISettingsControllerRemapState::Browse);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::HandleApplySelectedKey(const FInputChord& SelectedKey)
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

	if (!GamepadMappingOptions.IsValidIndex(SelectedKeyLineIndex))
	{
		UE_LOG(LogSoUISettings, Error, TEXT("HandleApplySelectedKey: GamepadMappingOptions does not have selected index = %d"), SelectedKeyLineIndex);
		return;
	}

	// Try to rebind
	const FInputActionKeyMapping OldKeyMapping = GamepadMappingOptions[SelectedKeyLineIndex];
	const FName SelectedActionName = OldKeyMapping.ActionName;
	if (UserSettings->RebindInputKeyToInputChord(OldKeyMapping, SelectedKey, true, false))
	{
		// Update UI
		UserSettings->ApplyInputSettings(true, true);
		CreateInputButtonsFromUserSettings(SelectedKeyLineIndex);
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
bool USoUISettingsControllerRemap::IsSelectedKeyValid(const FInputChord& SelectedKey, FText& OutError, FText& OutWarning, bool bNoOutput)
{
	if (!USoInputHelper::IsAllowedInputGamepadKey(SelectedKey))
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

	if (FSoInputKey::GetThumbStickForStickDirection(SelectedKey.Key.GetFName()) != NAME_None)
	{
#if PLATFORM_SWITCH
		OutError = FROM_STRING_TABLE_UI("input_not_allowed_thumbsticks_switch");
#else
		OutError = FROM_STRING_TABLE_UI("input_not_allowed_thumbsticks");
#endif
		if (!bNoOutput)
			UE_LOG(LogSoUISettings, Error, TEXT("%s"), *OutError.ToString());

		return false;
	}
	if (SelectedKey.Key.GetFName() == FSoInputKey::Gamepad_Special_Right())
	{
#if PLATFORM_SWITCH
		OutError = FROM_STRING_TABLE_UI("input_not_allowed_options_menu_button_switch");
#else
		OutError = FROM_STRING_TABLE_UI("input_not_allowed_options_menu_button");
#endif
		if (!bNoOutput)
			UE_LOG(LogSoUISettings, Error, TEXT("%s"), *OutError.ToString());

		return false;
	}

	// Key conflicts are allowed but warn the user
	const FName SelectedActionName = InputOptions[SelectedButtonIndex].ActionName;
	if (USoUIHelper::AreThereAnyKeyConflicts(UserSettings, SelectedActionName, SelectedKey, OutWarning))
	{
		if (!bNoOutput)
			UE_LOG(LogSoUISettings, Error, TEXT("%s"), *OutWarning.ToString());
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsControllerRemap::UpdateCanEditGamepadButtons()
{
	// Stop user from modifying with keyboard
	if (!SoController->IsLastInputFromGamepad())
	{
		KeySelectorOverlay->Close();
		ContentContainer->SetIsEnabled(false);
		OnlyGamepadOverlay->SetVisibility(ESlateVisibility::Visible);
		SetCurrentState(ESoUISettingsControllerRemapState::Browse);
		return false;
	}

	OnlyGamepadOverlay->SetVisibility(ESlateVisibility::Collapsed);
	ContentContainer->SetIsEnabled(true);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::CreateInputButtonsFromUserSettings(int32 SelectLineIndex)
{
	USoUIButtonArray* ButtonsArray = GetWidgetsArrayAsButtons();
	if (ButtonsArray == nullptr)
		return;

	UserSettings->RefreshInputSettingsMappings();
	GamepadMappingOptions.Empty();
	UnboundLineIndices.Empty();

	// NOTE: we don't actually use the texts, only set them so that we can create the correct amount of children
	TArray<FText> GamepadButtonTexts;
	for (int32 Index = 0, Num = InputOptions.Num(); Index < Num; Index++)
	{
		const FSoInputConfigurableActionName& Option = InputOptions[Index];

		// Query the input settings for this configurable option
		const TArray<FInputActionKeyMapping> OptionMappings = UserSettings->GetGamepadInputActionMappingsForActionName(Option.ActionName, true);

		if (OptionMappings.Num() == 0)
		{
			// Something is wrong
			GamepadButtonTexts.Add(FROM_STRING_TABLE_UI("unknown"));
			GamepadMappingOptions.Add(FInputActionKeyMapping());
		}
		else
		{
			bool bAlreadyAdded = false;

			int32 MappingIndex = INDEX_NONE;
			for (const FInputActionKeyMapping& KeyMapping : OptionMappings)
			{
				MappingIndex++;

				// Multiple bindings for the same key?
				if (bAlreadyAdded)
				{
					UE_LOG(LogSoUISettings, Warning, TEXT("ActionName = %s also has an additional gamepad Key = %s. Ignoring "), *Option.ActionName.ToString(), *KeyMapping.Key.ToString());
					continue;
				}

				if (KeyMapping.Key.IsValid())
				{
					GamepadButtonTexts.Add(FSoInputKey::GetKeyDisplayName(KeyMapping.Key));
				}
				else
				{
					// Has another mapping, lets try that instead of making this unbound
					// Most likely the keyboard did unbind it
					if (MappingIndex + 1 < OptionMappings.Num())
						continue;

					UnboundLineIndices.Add(Index);
					GamepadButtonTexts.Add(FROM_STRING_TABLE_UI("input_unbound"));
				}

				GamepadMappingOptions.Add(KeyMapping);
				bAlreadyAdded = true;
			}
		}
	}

	ButtonsArray->CreateButtons(GamepadButtonTexts);
	if (InputOptions.Num() != NumLines())
	{
		// Here be dragons
		UE_LOG(LogSoUISettings,
			Fatal,
			TEXT("InputOptions.Num(%d) != GamepadButtons->GetElementNum(%) This would most likely make the input settings not work anymore"),
			InputOptions.Num(), NumLines());
	}
	if (InputOptions.Num() != GamepadMappingOptions.Num())
	{
		// Here be vampires
		UE_LOG(LogSoUISettings,
			Fatal,
			TEXT("InputOptions.Num(%d) != GamepadMappingOptions.Num(%) This would most likely make the input settings not work anymore"),
			InputOptions.Num(), GamepadMappingOptions.Num());
	}

	// Set images on buttons
	UpdateGamepadButtonsImages();

	// Select line
	if (SelectLineIndex != INDEX_NONE)
	{
		const int32 OldLineIndex = GetSelectedLineIndex();
		SetSelectedLineIndex(SelectLineIndex);
		//GamepadButtons->ActivateChildAt(SelectLineIndex);
	}

	// BP
	ReceiveOnCreateInputButtonsFromUserSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::SetCurrentState(ESoUISettingsControllerRemapState InCurrentState)
{
	const ESoUISettingsControllerRemapState PreviousState = CurrentState;
	CurrentState = InCurrentState;

	switch (CurrentState)
	{
		case ESoUISettingsControllerRemapState::Browse:
			// Do not change line index if we are coming from changing a key
			CreateInputButtonsFromUserSettings(
				PreviousState == ESoUISettingsControllerRemapState::SelectingKey ? INDEX_NONE : 0
			);
			break;

		case ESoUISettingsControllerRemapState::SelectingKey:
			if (KeySelectorOverlay)
			{
				// Set the correct action name
				SelectedKeyLineIndex = GetSelectedLineIndex();
				if (InputOptions.IsValidIndex(SelectedKeyLineIndex))
				{
					KeySelectorOverlay->SetActionName(InputOptions[SelectedKeyLineIndex].DisplayText);
					KeySelectorOverlay->Open(true);
				}
			}
			break;

		case ESoUISettingsControllerRemapState::ResetAllToDefaults:
			UserSettings->SetGamepadInputBindingsToDefault();
			UserSettings->ApplyInputSettings(true, true);
			CreateInputButtonsFromUserSettings(0);
			break;

		default:
			break;
	}

	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
{
	if (!bIsListeningToDevicesChanges || !bOpened)
		return;

	UpdateGamepadButtonsImages();
	UpdateCanEditGamepadButtons();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsControllerRemap::SubscribeToDeviceChanged()
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
void USoUISettingsControllerRemap::UnSubscribeFromDeviceChanged()
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
void USoUISettingsControllerRemap::UpdateGamepadButtonsImages()
{
	USoUIButtonArray* ButtonsArray = GetWidgetsArrayAsButtons();
	if (ButtonsArray == nullptr)
		return;

	const ESoInputDeviceType DeviceType = USoInputHelper::GetCurrentGamepadDeviceTypeFromSettings(this);
	const ESoInputDeviceType GamepadDeviceType = (DeviceType == ESoInputDeviceType::Gamepad_PlayStation || DeviceType == ESoInputDeviceType::Gamepad_Switch) ?
												 DeviceType : ESoInputDeviceType::Gamepad_Xbox;

	const int32 NumButtons = ButtonsArray->NumAllChildren();
	for (int32 Index = 0; Index < NumButtons && Index < GamepadMappingOptions.Num(); Index++)
	{
		USoUIButtonImage* ButtonImage = Cast<USoUIButtonImage>(ButtonsArray->GetContainerButtonAt(Index));
		if (!ButtonImage)
			continue;

		if (UnboundLineIndices.Contains(Index))
		{
			// Unbound
			ButtonImage->DisableImageMode();
		}
		else
		{
			UTexture2D* Texture = USoGameSingleton::GetIconForInputActionName(GamepadMappingOptions[Index].ActionName, GamepadDeviceType, false);
			ButtonImage->SetImageFromTexture(Texture, true);
		}
		ReceiveOnUpdateGamepadButton(Index, ButtonImage);
	}
}
