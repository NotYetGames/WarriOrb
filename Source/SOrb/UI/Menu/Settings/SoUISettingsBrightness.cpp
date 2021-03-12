// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUISettingsBrightness.h"

#include "GameFramework/InputSettings.h"

#include "UI/General/SoUISlider.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"
#include "Localization/SoLocalization.h"
#include "Settings/SoGameSettings.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::NativeConstruct()
{
	Super::NativeConstruct();

	if (Brightness)
		Brightness->OnValueChangedEvent().AddDynamic(this, &ThisClass::OnBrightnessValueChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::NativeDestruct()
{
	if (Brightness)
		Brightness->OnValueChangedEvent().RemoveDynamic(this, &ThisClass::OnBrightnessValueChanged);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::Open_Implementation(bool bOpen)
{
	if (bOpen)
	{
		// Register initial value
		InitialBrightnessNormalized = USoGameSettings::Get().GetBrightnessNormalized();
	}
	else
	{
		// Reset to initial value
		if (InitialBrightnessNormalized > 0.f && DidInitialValuesChange())
		{
			USoGameSettings::Get().SetBrightnessNormalized(InitialBrightnessNormalized);
			USoGameSettings::Get().ApplyGameSettings(true);
		}
	}

	ResetSelectedOptionsToCurrentSettings();

	Super::Open_Implementation(bOpen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	// Set the Lines order and text
	EmptyLinesAndTitles();
	for (const ESoUISettingsBrightnessLine Option : LineOptions)
	{
		switch (Option)
		{
		case ESoUISettingsBrightnessLine::Brightness:
			AddLineWithText(Brightness, FROM_STRING_TABLE_UI("settings_brightness"));
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	if (LineOptions.Num() != NumLines())
		UE_LOG(LogSoUISettings, Error, TEXT("Brightness CreateChildrenAndSetTexts: LineOptions.Num(%d) != NumLines(%d)"), LineOptions.Num(), NumLines());

	Super::CreateChildrenAndSetTexts(bFromSynchronizeProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBrightness::HandleOnUICommand(ESoUICommand Command)
{
	if (Super::HandleOnUICommand(Command))
		return true;

	if (!OnPressedButtonTooltipsCommand(Command))
		return false;

	ResetSelectedOptionsToCurrentSettings();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBrightness::NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput)
{
	if (!Super::NavigateLeftRightOnSelectedLine(Command, bFromPressedInput))
		return false;

	if (IsOnButtonTooltips())
		return true;

	ResetSelectedOptionsToCurrentSettings();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::RefreshButtonImagesTooltips(bool bForceShowAll)
{
	if (!ButtonImagesTooltipsArray)
		return;

	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	if (bForceShowAll || DidInitialValuesChange())
		CommandsToBeDisplayed.Add({ UICommand_Apply, FROM_STRING_TABLE_UI("settings_apply") });

	if (bForceShowAll || CanRevertToDefault())
		CommandsToBeDisplayed.Add({ UICommand_ResetToDefault, FROM_STRING_TABLE_UI("settings_reset") });

	ButtonImagesTooltipsArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBrightness::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	switch (Command)
	{
	case UICommand_Apply:
	{
		OnCommandApply();
		break;
	}
	case UICommand_ResetToDefault:
	{
		OnCommandResetToDefault();
		break;
	}

	default:
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::ResetSelectedOptionsToCurrentSettings()
{
	USoGameSettings::Get().SetBlockConfigSave(true);

	if (Brightness)
		Brightness->SetValueNormalized(USoGameSettings::Get().GetBrightnessNormalized());

	EmptyAndRefreshButtonImagesTooltips();

	USoGameSettings::Get().SetBlockConfigSave(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBrightness::DidInitialValuesChange() const
{
	return !FMath::IsNearlyEqual(InitialBrightnessNormalized, Brightness->GetValueNormalized(), KINDA_SMALL_NUMBER);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsBrightness::CanRevertToDefault() const
{
	return !USoGameSettings::Get().IsEqualToDefaultNormalizedBrightness(Brightness->GetValueNormalized());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::OnCommandApply()
{
	if (DidInitialValuesChange())
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsApply));

	InitialBrightnessNormalized = USoGameSettings::Get().GetBrightnessNormalized();
	ResetSelectedOptionsToCurrentSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::OnCommandResetToDefault()
{
	if (CanRevertToDefault())
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsRestore));

	USoGameSettings::Get().ResetBrightnessToDefault();
	USoGameSettings::Get().ApplyGameSettings(true);
	InitialBrightnessNormalized = USoGameSettings::Get().GetBrightnessNormalized();
	ResetSelectedOptionsToCurrentSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsBrightness::OnBrightnessValueChanged(float NewValueNormalized)
{
	if (!bOpened)
		return;

	USoGameSettings::Get().SetBrightnessNormalized(NewValueNormalized);
		USoGameSettings::Get().ApplyGameSettings(true);
	RefreshButtonImagesTooltips();
}
