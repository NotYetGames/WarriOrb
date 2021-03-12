// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUISettingsController.h"

#include "GameFramework/InputSettings.h"
#include "Components/WidgetSwitcher.h"
#include "Components/BackgroundBlur.h"

#include "Basic/SoAudioManager.h"
#include "Basic/SoGameSingleton.h"

#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"
#include "Settings/Input/SoInputNames.h"

#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/Buttons/SoUIScrollingButtonArray.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/Commands/SoUICommandImage.h"
#include "UI/General/SoUICheckbox.h"
#include "UI/General/SoUISlider.h"
#include "UI/General/Commands/SoUICommandTooltip.h"

#include "Localization/SoLocalization.h"
#include "SoUISettingsControllerRemap.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::NativeConstruct()
{
	Super::NativeConstruct();
	BackgroundBlur->SetVisibility(ESlateVisibility::Collapsed);

	if (ControllerPlatform)
		ControllerPlatform->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnControllerPlatformChildChanged);

	if (ControllerPlatform)
		ControllerPlatform->SetSelectedIndex(ControllerPlatformOptions.Find(static_cast<ESoGamepadLayoutType>(0)));

	if (Vibration)
		Vibration->OnValueChangedEvent().AddDynamic(this, &ThisClass::OnVibrationValueChanged);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::NativeDestruct()
{
	if (Vibration)
		Vibration->OnValueChangedEvent().RemoveDynamic(this, &ThisClass::OnVibrationValueChanged);
	if (ControllerPlatform)
		ControllerPlatform->OnSelectedChildChangedEvent().RemoveAll(this);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::Open_Implementation(bool bOpen)
{
	UserSettings->SetBlockConfigSave(true);
	ResetSelectedOptionsToUserSettings();
	if (bOpen)
	{
		InitializeAllGamepadButtons();
	}
	else
	{
		// Close all child widgets
		BackgroundBlur->SetVisibility(ESlateVisibility::Collapsed);
		ISoUIEventHandler::Execute_Open(RemapController, false);
	}
	UserSettings->SetBlockConfigSave(false);

	Super::Open_Implementation(bOpen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsController::CanBeInterrupted_Implementation() const
{
	if (ISoUIEventHandler::Execute_IsOpened(RemapController))
		return ISoUIEventHandler::Execute_CanBeInterrupted(RemapController);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	RefreshLineOptions();
	if (ControllerPlatform)
	{
		TArray<FText> Texts;
		for (const ESoGamepadLayoutType Option : ControllerPlatformOptions)
		{
			switch (Option)
			{
			case ESoGamepadLayoutType::Xbox:
				Texts.Add(FROM_STRING_TABLE_UI("console_xbox_one"));
				break;
			case ESoGamepadLayoutType::PlayStation:
				Texts.Add(FROM_STRING_TABLE_UI("console_ps4"));
				break;
			case ESoGamepadLayoutType::Switch:
				Texts.Add(FROM_STRING_TABLE_UI("console_switch"));
				break;
			default:
				Texts.Add(FROM_STRING_TABLE_UI("unknown"));
				break;
			}
		}
		ControllerPlatform->CreateButtons(Texts);
	}

	// Set the Lines order and text
	EmptyLinesAndTitles();
	for (const ESoUISettingsControllerLine Option : LineOptions)
	{
		switch (Option)
		{
		case ESoUISettingsControllerLine::AutoDetectControllerType:
			AddLineWithText(AutoDetectControllerType, FROM_STRING_TABLE_UI("settings_auto_detect_controller"));
			break;

		case ESoUISettingsControllerLine::Layout:
			AddLineWithText(ControllerPlatform, FROM_STRING_TABLE_UI("settings_controller_platform"));
			break;

		case ESoUISettingsControllerLine::Vibration:
			AddLineWithText(Vibration, FROM_STRING_TABLE_UI("settings_gamepad_vibration"));
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	if (LineOptions.Num() != NumLines())
		UE_LOG(LogSoUISettings, Error, TEXT("Controller CreateChildrenAndSetTexts: LineOptions.Num(%d) != NumLines(%d)"), LineOptions.Num(), NumLines());

	Super::CreateChildrenAndSetTexts(bFromSynchronizeProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsController::ShouldChildHandleCommands() const
{
	return RemapController && ISoUIEventHandler::Execute_IsOpened(RemapController);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsController::HandleOnUICommand(ESoUICommand Command)
{
	// Forward to remap controller
	if (ShouldChildHandleCommands())
	{
		ISoUIEventHandler::Execute_OnUICommand(RemapController, Command);

		// Was closed
		if (!ShouldChildHandleCommands())
		{
			EmptyAndRefreshButtonImagesTooltips();
			InitializeAllGamepadButtons();
			BackgroundBlur->SetVisibility(ESlateVisibility::Collapsed);
		}

		return true;
	}

	if (Super::HandleOnUICommand(Command))
		return true;

	return OnPressedButtonTooltipsCommand(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	Super::OnPressedChild(SelectedChild, SoUserWidget);
	if (!IsValidLineIndex(SelectedChild))
		return;

	bool bHandled = false;
	const ESoUISettingsControllerLine SelectedLineOption = LineOptions[SelectedChild];
	if (SelectedLineOption == ESoUISettingsControllerLine::AutoDetectControllerType)
	{
		UserSettings->SetAutoDetectGamepadLayout(AutoDetectControllerType->IsChecked());
		bHandled = true;
	}

	if (bHandled)
	{
		UserSettings->ApplyInputSettings();
		ResetSelectedOptionsToUserSettings();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsController::NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput)
{
	if (!Super::NavigateLeftRightOnSelectedLine(Command, bFromPressedInput))
		return false;

	// Handled in super, I hope
	if (IsOnButtonTooltips())
		return true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::RefreshButtonImagesTooltips(bool bForceShowAll)
{
	if (!ButtonImagesTooltipsArray)
		return;

	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	CommandsToBeDisplayed.Add({ UICommand_RemapController, FROM_STRING_TABLE_UI("settings_controller_remap_single") });
	ButtonImagesTooltipsArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsController::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	switch (Command)
	{
	case UICommand_RemapController:
		OnCommandRemapController();
		break;

	default:
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::OnCommandRemapController()
{
	ISoUIEventHandler::Execute_Open(RemapController, true);
	BackgroundBlur->SetVisibility(ESlateVisibility::Visible);
	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsPressed));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::ResetSelectedOptionsToUserSettings()
{
	// NOTE: See note from USoUISettingsDisplay::ResetSelectedOptionsToUserSettings
	UserSettings->ResetToCurrentSettings();
	UserSettings->ValidateSettings();

	if (ControllerPlatform)
	{
		const int32 FoundIndex = ControllerPlatformOptions.Find(UserSettings->GetGamepadLayoutType());
		ControllerPlatform->SetSelectedIndex(FoundIndex);
	}
	if (Vibration)
		Vibration->SetValueNormalized(USoGameSettings::Get().GetVibrationScaleNormalized());

	UpdateUI();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::UpdateWidgetsVisibilities()
{
	if (ControllerPlatform)
	{
		ControllerPlatform->SetVisibility(
			LineOptions.Contains(ESoUISettingsControllerLine::Layout) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::RefreshLineOptions()
{
	LineOptions = { ESoUISettingsControllerLine::Vibration };

	if (!USoPlatformHelper::IsConsole())
		LineOptions.Add(ESoUISettingsControllerLine::Layout);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::UpdateUI()
{
	if (ControllerPlatform)
	{
		const int32 FoundIndex = ControllerPlatformOptions.Find(UserSettings->GetGamepadLayoutType());
		if (ControllerSwitcher)
			ControllerSwitcher->SetActiveWidgetIndex(FoundIndex);
		if (ControllerSwitcherLines)
			ControllerSwitcherLines->SetActiveWidgetIndex(FoundIndex);
	}
	if (AutoDetectControllerType)
	{
		AutoDetectControllerType->SetIsChecked(UserSettings->IsAutoDetectGamepadLayout());
	}

	UpdateAllImagesFromSelectedDeviceType();
	UpdateLinesVisibility();
	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoGamepadLayoutType USoUISettingsController::GetSelectedGamepadLayoutType() const
{
	const int32 SelectedElementIndex = ControllerPlatform->GetSelectedIndex();
	if (ControllerPlatformOptions.IsValidIndex(SelectedElementIndex))
	{
		return ControllerPlatformOptions[SelectedElementIndex];
	}

	return ESoGamepadLayoutType::Xbox;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::InitializeAllGamepadButtons()
{
	InitializeGamepadButton(Gamepad_RightShoulder, FSoInputKey::Gamepad_RightShoulder());
	InitializeGamepadButton(Gamepad_RightTrigger, FSoInputKey::Gamepad_RightTrigger());
	InitializeGamepadButton(Gamepad_LeftTrigger, FSoInputKey::Gamepad_LeftTrigger());
	InitializeGamepadButton(Gamepad_LeftShoulder, FSoInputKey::Gamepad_LeftShoulder());
	InitializeGamepadButton(Gamepad_FaceButton_Top, FSoInputKey::Gamepad_FaceButton_Top());
	InitializeGamepadButton(Gamepad_FaceButton_Left, FSoInputKey::Gamepad_FaceButton_Left());
	InitializeGamepadButton(Gamepad_FaceButton_Right, FSoInputKey::Gamepad_FaceButton_Right());
	InitializeGamepadButton(Gamepad_FaceButton_Bottom, FSoInputKey::Gamepad_FaceButton_Bottom());
	InitializeGamepadButton(Gamepad_Special_Left, FSoInputKey::Gamepad_Special_Left());
	InitializeGamepadButton(Gamepad_Special_Right, FSoInputKey::Gamepad_Special_Right(), true);
	InitializeGamepadButton(Gamepad_DPad_Up, FSoInputKey::Gamepad_DPad_Up());
	InitializeGamepadButton(Gamepad_DPad_Down, FSoInputKey::Gamepad_DPad_Down());
	InitializeGamepadButton(Gamepad_DPad_Left, FSoInputKey::Gamepad_DPad_Left());
	InitializeGamepadButton(Gamepad_DPad_Right, FSoInputKey::Gamepad_DPad_Right());

	// Handle Gamepad_LeftThumbstick because it is an axis
	bool bFound = false;

	// Test MoveLeft
	FInputActionKeyMapping KeyMapping;
	const TArray<FInputActionKeyMapping> ActionMappings = UserSettings->GetGamepadInputActionMappingsForActionName(FSoInputActionName::MoveLeft, false);
	if (USoInputHelper::GetFirstInputActionKeyMapping(ActionMappings, KeyMapping))
	{
		const FName ThumbStickDirection = KeyMapping.Key.GetFName();
		const FName ThumbStick = FSoInputKey::GetThumbStickForStickDirection(ThumbStickDirection);
		if (ThumbStick != NAME_None)
		{
			OverrideGamepadButtonTextWith(Gamepad_LeftThumbstick, FROM_STRING_TABLE_UI("settings_move_left_right"));
			OverrideGamepadButtonImageWithTexturesOf(Gamepad_LeftThumbstick, ThumbStick);
			bFound = true;
		}
	}

	if (!bFound)
	{
		// Empty :(
		InitializeEmptyGamepadButton(Gamepad_LeftThumbstick, FSoInputKey::Gamepad_LeftThumbstick());
	}


	// Handle Gamepad_RightThumbstick
	OverrideGamepadButtonTextWith(Gamepad_RightThumbstick, FROM_STRING_TABLE_UI("select_spells"));
	OverrideGamepadButtonImageWithTexturesOf(Gamepad_RightThumbstick, FSoInputKey::Gamepad_RightThumbstick());

	UpdateAllImagesFromSelectedDeviceType();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::InitializeGamepadButton(USoUICommandTooltip* Button, FName GamepadKeyName, bool bAlsoCheckUICommand)
{
	verify(Button);
	verify(GamepadKeyName != NAME_None);

	// Get action names for gamepad key
	const TArray<FName>& ActionNames = UserSettings->GetActionNamesForGamepadKey(GamepadKeyName);
	const TArray<FName>& UIActionNames = UserSettings->GetUIActionNamesForGamepadKey(GamepadKeyName);
	const bool bIsActionName = ActionNames.Num() > 0;
	const bool bIsUIActionName = UIActionNames.Num() > 0;
	if (!bIsActionName && !bIsUIActionName)
	{
		UE_LOG(LogSoUISettings, Error, TEXT("InitializeGamepadButton: Can't get any ActionName for GamepadKeyName = %s"),*GamepadKeyName.ToString());
		return;
	}

	// Get correct actionName
	FName ActionName = NAME_None;
	if (bIsActionName)
		ActionName = ActionNames[0];
	if (bAlsoCheckUICommand && bIsUIActionName)
		ActionName = UIActionNames[0];

	const bool bIsConfigurable = FSoInputConfigurableActionName::IsGamepadActionName(ActionName);
	if ((!bAlsoCheckUICommand && !bIsActionName)
		|| (bAlsoCheckUICommand && !bIsUIActionName)
		|| (!bAlsoCheckUICommand && !bIsConfigurable))
	{
		InitializeEmptyGamepadButton(Button, GamepadKeyName);
		return;
	}

	// Key is taken by an action
	if (bAlsoCheckUICommand)
	{
		// UI
		const ESoUICommand Command = FSoInputActionName::ActionNameToUICommand(ActionName);
		Button->InitializeFromUICommand(USoGameSingleton::GetTextForInputActionName(ActionName), Command, false);
	}
	else
	{
		// Normal
		const ESoInputActionNameType ActionNameType = FSoInputActionName::ActionNameToActionNameType(ActionName);
		Button->InitializeFromInputActionNameType(USoGameSingleton::GetTextForInputActionName(ActionName), ActionNameType, false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::InitializeEmptyGamepadButton(USoUICommandTooltip* Button, FName GamepadKeyName)
{
	if (!Button)
		return;

	// Set to empty stuff
	static FText EmptyLine = FROM_STRING_TABLE_UI("empty_line");

	Button->SetPaddingBetween(10.f);
	OverrideGamepadButtonTextWith(Button, EmptyLine);
	OverrideGamepadButtonImageWithTexturesOf(Button, GamepadKeyName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::OverrideGamepadButtonTextWith(USoUICommandTooltip* Button, FText Text)
{
	if (Button)
		Button->SetText(Text);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::OverrideGamepadButtonImageWithTexturesOf(USoUICommandTooltip* Button, FName GamepadKeyName)
{
	if (!Button)
		return;

	FSoInputGamepadKeyTextures Textures;
	if (USoGameSingleton::GetGamepadKeyTexturesFromKeyName(GamepadKeyName, false, Textures))
	{
		Button->SetOverrideGamepadTextures(Textures);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::UpdateAllImagesFromSelectedDeviceType()
{
	const ESoGamepadLayoutType GamepadUIType = GetSelectedGamepadLayoutType();
	const ESoInputDeviceType DeviceType = USoInputHelper::ConvertInputGamepadUITypeToInputDeviceType(GamepadUIType);

	UpdateImageFromSelectedDeviceType(Gamepad_RightShoulder, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_RightTrigger, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_LeftTrigger, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_LeftShoulder, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_FaceButton_Top, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_FaceButton_Left, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_FaceButton_Right, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_FaceButton_Bottom, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_Special_Left, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_Special_Right, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_LeftThumbstick, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_RightThumbstick, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_DPad_Up, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_DPad_Down, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_DPad_Left, DeviceType);
	UpdateImageFromSelectedDeviceType(Gamepad_DPad_Right, DeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::UpdateImageFromSelectedDeviceType(USoUICommandTooltip* Button, ESoInputDeviceType DeviceType)
{
	if (!Button || DeviceType == ESoInputDeviceType::Keyboard)
		return;

	Button->SetDeviceType(DeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::UpdateLinesVisibility()
{
	if (ControllerPlatform && AutoDetectControllerType)
		ControllerPlatform->SetIsEnabled(!AutoDetectControllerType->IsChecked());

	if (AutoDetectControllerType && !LineOptions.Contains(ESoUISettingsControllerLine::AutoDetectControllerType))
		AutoDetectControllerType->SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::OnControllerPlatformChildChanged(int32 PreviousChild, int32 NewChild)
{
	if (!bOpened)
		return;

	if (ControllerPlatformOptions.IsValidIndex(NewChild))
	{
		const ESoGamepadLayoutType SelectedOption = ControllerPlatformOptions[NewChild];
		UserSettings->SetGamepadLayoutType(SelectedOption);
		UserSettings->ApplyInputSettings();
		UpdateUI();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsController::OnVibrationValueChanged(float NewValueNormalized)
{
	if (!bOpened)
		return;

	USoGameSettings::Get().SetVibrationScaleNormalized(NewValueNormalized);
	USoGameSettings::Get().ApplyGameSettings(true);
	if (ForceFeedbackToPlayOnChange)
	{
		static constexpr bool bIgnoreTimeDilation = true;
		static constexpr bool bPlayWhilePaused = true;
		USoPlatformHelper::PlayForceFeedback(
			this,
			ForceFeedbackToPlayOnChange,
			NAME_None,
			false,
			bIgnoreTimeDilation,
			bPlayWhilePaused
		);
	}

	RefreshButtonImagesTooltips();
}
