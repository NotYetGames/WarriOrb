// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIConfirmKeyboardSettings.h"

#include "Engine/Engine.h"

#include "Localization/SoLocalization.h"
#include "Settings/SoGameSettings.h"
#include "UI/General/Commands/SoUICommandTooltipArray.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/SoUIUserWidget.h"
#include "Components/TextBlock.h"
#include "Basic/SoAudioManager.h"
#include "Basic/SoGameSingleton.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmKeyboardSettings::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmKeyboardSettings::NativeConstruct()
{
	Super::NativeConstruct();

	ButtonsArray->OnNavigateOnPressedHandleChildEvent().BindLambda([this](int32 SelectedChild, USoUIUserWidget* SoUserWidget)
	{
		// SFX
		SoUserWidget->NavigateOnPressed(true);

		if (USoUIButtonImageArray* ButtonTooltips = GetButtonArrayAsButtonImagesArray())
			OnPressedButtonTooltipsCommand(ButtonTooltips->GetSelectedButtonCommand());
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmKeyboardSettings::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmKeyboardSettings::Open_Implementation(bool bOpen)
{
	Super::Open_Implementation(bOpen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIConfirmKeyboardSettings::OnUICommand_Implementation(ESoUICommand Command)
{
	if (!bOpened)
		return false;

	switch (Command)
	{
	case ESoUICommand::EUC_Left:
	case ESoUICommand::EUC_Right:
	case ESoUICommand::EUC_Up:
	case ESoUICommand::EUC_Down:
	case ESoUICommand::EUC_MainMenuEnter:
		ButtonsArray->Navigate(Command);
		return true;

	case ESoUICommand::EUC_MainMenuBack:
	case UICommand_Cancel:
		ISoUIEventHandler::Execute_Open(this, false);
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsClose));
		return true;

	default:
		return OnPressedButtonTooltipsCommand(Command);
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmKeyboardSettings::RefreshButtonsArray()
{
	// Restore to default
	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	CommandsToBeDisplayed.Add({ UICommand_Apply, FROM_STRING_TABLE_UI("settings_reset") });
	CommandsToBeDisplayed.Add({ UICommand_Cancel, FROM_STRING_TABLE_UI("settings_cancel") });

	UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIConfirmKeyboardSettings::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	switch (Command)
	{
		case UICommand_Apply:
			if (bOpenedForResetToWASD)
				OnApplyWASDPreset();
			else
				OnApplyArrowsPreset();
			break;

		case UICommand_Cancel:
			ISoUIEventHandler::Execute_Open(this, false);
			USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsClose));
			return true;

		default:
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmKeyboardSettings::OnApplyWASDPreset()
{
	auto& UserSettings = USoGameSettings::Get();
	UserSettings.SetKeyboardInputBindingsToDefault();
	UserSettings.ApplyInputSettings(true);

	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsApply));

	// Close this as we handled it
	ISoUIEventHandler::Execute_Open(this, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmKeyboardSettings::OnApplyArrowsPreset()
{
	auto& UserSettings = USoGameSettings::Get();
	UserSettings.SetInputToPresetKeyboardArrows();
	UserSettings.ApplyInputSettings(true);

	USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsApply));

	// Close this as we handled it
	ISoUIEventHandler::Execute_Open(this, false);
}
