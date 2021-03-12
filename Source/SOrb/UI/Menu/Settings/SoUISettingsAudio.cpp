// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISettingsAudio.h"

#include "Components/TextBlock.h"
#include "Basic/SoGameSingleton.h"
#include "Settings/SoGameSettings.h"
#include "Basic/SoAudioManager.h"

#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/SoUISlider.h"
#include "UI/General/SoUICheckbox.h"
#include "Localization/SoLocalization.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::NativeConstruct()
{
	Super::NativeConstruct();
	CurrentTooltipText->SetVisibility(ESlateVisibility::Collapsed);

	if (MasterAudio)
		MasterAudio->OnValueChangedEvent().AddDynamic(this, &Self::OnMasterValueChanged);
	if (SoundEffectsAudio)
		SoundEffectsAudio->OnValueChangedEvent().AddDynamic(this, &Self::OnSoundEffectsValueChanged);
	if (MusicAudio)
		MusicAudio->OnValueChangedEvent().AddDynamic(this, &Self::OnMusicValueChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::NativeDestruct()
{
	if (MasterAudio)
		MasterAudio->OnValueChangedEvent().RemoveDynamic(this, &Self::OnMasterValueChanged);
	if (SoundEffectsAudio)
		SoundEffectsAudio->OnValueChangedEvent().RemoveDynamic(this, &Self::OnSoundEffectsValueChanged);
	if (MusicAudio)
		MusicAudio->OnValueChangedEvent().RemoveDynamic(this, &Self::OnMusicValueChanged);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::Open_Implementation(bool bOpen)
{
	Super::Open_Implementation(bOpen);

	if (bOpen)
		ResetSelectedOptionsToUserSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	// Set the Lines order and text
	EmptyLinesAndTitles();
	for (const ESoUISettingsAudioLine Option : LineOptions)
	{
		switch (Option)
		{
		case ESoUISettingsAudioLine::MuteAudio:
			AddLineWithText(MuteAudio, FROM_STRING_TABLE_UI("settings_mute_audio"));
			break;

		case ESoUISettingsAudioLine::MuteAudioWhenUnfocused:
			AddLineWithText(MuteAudioWhenUnfocused, FROM_STRING_TABLE_UI("settings_mute_audio_unfocused"));
			break;

		case ESoUISettingsAudioLine::MuteVoiceGibberish:
			AddLineWithText(MuteVoiceGibberish, FROM_STRING_TABLE_UI("settings_mute_voice"));
			break;

		case ESoUISettingsAudioLine::MuteDialogueVoiceGibberish:
			AddLineWithText(MuteDialogueVoiceGibberish, FROM_STRING_TABLE_UI("settings_mute_dialogue_voice"));
			break;

		case ESoUISettingsAudioLine::MasterAudio:
			AddLineWithText(MasterAudio, FROM_STRING_TABLE_UI("settings_master_volume"));
			break;

		case ESoUISettingsAudioLine::SoundEffectsAudio:
			AddLineWithText(SoundEffectsAudio, FROM_STRING_TABLE_UI("settings_effects_volume"));
			break;

		case ESoUISettingsAudioLine::MusicAudio:
			AddLineWithText(MusicAudio, FROM_STRING_TABLE_UI("settings_music_volume"));
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	if (LineOptions.Num() != NumLines())
		UE_LOG(LogSoUISettings, Error, TEXT("Audio CreateChildrenAndSetTexts: LineOptions.Num(%d) != NumLines(%d)"), LineOptions.Num(), NumLines());

	Super::CreateChildrenAndSetTexts(bFromSynchronizeProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsAudio::HandleOnUICommand(ESoUICommand Command)
{
	if (Super::HandleOnUICommand(Command))
		return true;

	return OnPressedButtonTooltipsCommand(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	Super::OnPressedChild(SelectedChild, SoUserWidget);
	if (!IsValidLineIndex(SelectedChild))
		return;

	bool bHandled = true;
	const ESoUISettingsAudioLine SelectedLineOption = LineOptions[SelectedChild];
	if (SelectedLineOption == ESoUISettingsAudioLine::MuteAudio)
		UserSettings->SetIsAudioMuted(MuteAudio->IsChecked());
	else if (SelectedLineOption == ESoUISettingsAudioLine::MuteAudioWhenUnfocused)
		UserSettings->SetIsAudioMutedWhenUnfocused(MuteAudioWhenUnfocused->IsChecked());
	else if (SelectedLineOption == ESoUISettingsAudioLine::MuteVoiceGibberish)
		UserSettings->SetIsVoiceGibberishMuted(MuteVoiceGibberish->IsChecked());
	else if (SelectedLineOption == ESoUISettingsAudioLine::MuteDialogueVoiceGibberish)
		UserSettings->SetIsDialogueVoiceGibberishMuted(MuteDialogueVoiceGibberish->IsChecked());
	else
		bHandled = false;

	if (bHandled)
	{
		UserSettings->ApplyAudioSettings(true);
		ResetSelectedOptionsToUserSettings();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsAudio::NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput)
{
	if (!Super::NavigateLeftRightOnSelectedLine(Command, bFromPressedInput))
		return false;
	if (IsOnButtonTooltips())
		return true;

	ResetSelectedOptionsToUserSettings();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::OnPostSetSelectedIndex(int32 OldIndex, int32 NewIndex)
{
	Super::OnPostSetSelectedIndex(OldIndex, NewIndex);

	// Set line tooltips
	const int32 SelectedLineIndex = GetSelectedLineIndex();
	if (LineOptions.IsValidIndex(SelectedLineIndex))
	{
		const ESoUISettingsAudioLine CurrentType = LineOptions[SelectedLineIndex];
		if (LinesTooltips.Contains(CurrentType))
		{
			CurrentTooltipText->SetVisibility(ESlateVisibility::Visible);
			CurrentTooltipText->SetText(LinesTooltips[CurrentType]);
		}
		else
		{
			CurrentTooltipText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::RefreshButtonImagesTooltips(bool bForceShowAll)
{
	if (!ButtonImagesTooltipsArray)
		return;

	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	if (bForceShowAll || !AreDefaultAudioSettingsUsed())
		CommandsToBeDisplayed.Add({ UICommand_RevertToDefaults, FROM_STRING_TABLE_UI("settings_reset") });

	ButtonImagesTooltipsArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsAudio::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	switch (Command)
	{
	case UICommand_RevertToDefaults:
	{
		OnCommandRevertToDefaults();
		break;
	}
	default:
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsAudio::OnCommandRevertToDefaults()
{
	if (!AreDefaultAudioSettingsUsed())
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsRestore));

	UserSettings->SetAudioSettingsToDefault();
	UserSettings->ApplyAudioSettings(true);
	ResetSelectedOptionsToUserSettings();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::ResetSelectedOptionsToUserSettings()
{
	UserSettings->SetBlockConfigSave(true);

	UserSettings->ResetToCurrentSettings();
	UserSettings->ValidateSettings();

	MuteAudio->SetIsChecked(UserSettings->IsAudioMuted());
	MuteAudioWhenUnfocused->SetIsChecked(UserSettings->IsAudioMutedWhenUnfocused());
	MuteVoiceGibberish->SetIsChecked(UserSettings->IsVoiceGibberishMuted());
	MuteDialogueVoiceGibberish->SetIsChecked(UserSettings->IsDialogueVoiceGibberishMuted());

	//constexpr float MinPercentValue = 0.f;
	//constexpr float MaxPercentValue = 100.f;
	//MasterAudio->SetRangeTextDisplayValues(MinPercentValue, MaxPercentValue);
	//MusicAudio->SetRangeTextDisplayValues(MinPercentValue, MaxPercentValue);
	//SoundEffectsAudio->SetRangeTextDisplayValues(MinPercentValue, MaxPercentValue);

	MasterAudio->SetValueNormalized(UserSettings->GetVolumeMaster());
	MusicAudio->SetValueNormalized(UserSettings->GetVolumeMusic());
	SoundEffectsAudio->SetValueNormalized(UserSettings->GetVolumeSFX());

	UpdateLinesVisibility();
	EmptyAndRefreshButtonImagesTooltips();

	UserSettings->SetBlockConfigSave(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::UpdateLinesVisibility()
{
	const bool bIsAudioMuted = MuteAudio->IsChecked();

	// Disable sliders if audio is muted
	MasterAudio->SetIsEnabled(!bIsAudioMuted);
	SoundEffectsAudio->SetIsEnabled(!bIsAudioMuted);
	MusicAudio->SetIsEnabled(!bIsAudioMuted);
	MuteAudioWhenUnfocused->SetIsEnabled(!bIsAudioMuted);
	MuteVoiceGibberish->SetIsEnabled(!bIsAudioMuted);
	MuteDialogueVoiceGibberish->SetIsEnabled(!bIsAudioMuted);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsAudio::AreDefaultAudioSettingsUsed() const
{
	return UserSettings->AreDefaultAudioSettingsUsed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::OnMasterValueChanged(float NewValueNormalized)
{
	if (!bOpened)
		return;

	UserSettings->SetVolumeMaster(NewValueNormalized);
	UserSettings->ApplyAudioSettings(true);
	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::OnSoundEffectsValueChanged(float NewValueNormalized)
{
	if (!bOpened)
		return;

	UserSettings->SetVolumeSFX(NewValueNormalized);
	UserSettings->ApplyAudioSettings(true);
	RefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsAudio::OnMusicValueChanged(float NewValueNormalized)
{
	if (!bOpened)
		return;

	UserSettings->SetVolumeMusic(NewValueNormalized);
	UserSettings->ApplyAudioSettings(true);
	RefreshButtonImagesTooltips();
}
