// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISettingsGame.h"

#include "GameFramework/GameUserSettings.h"
#include "Components/TextBlock.h"

#include "Settings/SoGameSettings.h"
#include "Basic/SoGameInstance.h"

#include "UI/General/SoUICheckbox.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/Buttons/SoUIScrollingButtonArray.h"
#include "Localization/SoLocalization.h"
#include "Basic/Helpers/SoPlatformHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	// We don't need this
	if (ButtonImagesTooltipsArray)
		ButtonImagesTooltipsArray->SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::NativeConstruct()
{
	Super::NativeConstruct();
	CurrentTooltipText->SetVisibility(ESlateVisibility::Collapsed);

	// Listen to events
	if (Language)
		Language->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnLanguageChildChanged);

	if (CharacterSkin)
		CharacterSkin->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnCharacterSkinChanged);

	if (GameSpeed)
		GameSpeed->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnGameSpeedChildChanged);

	if (MercifulBounceMode)
		MercifulBounceMode->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnMercifulBounceModeChildChanged);

	ResetSelectedOptionsToUserSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::NativeDestruct()
{
	if (Language)
		Language->OnSelectedChildChangedEvent().RemoveAll(this);

	if (CharacterSkin)
		CharacterSkin->OnSelectedChildChangedEvent().RemoveAll(this);

	if (GameSpeed)
		GameSpeed->OnSelectedChildChangedEvent().RemoveAll(this);

	if (MercifulBounceMode)
		MercifulBounceMode->OnSelectedChildChangedEvent().RemoveAll(this);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::Open_Implementation(bool bOpen)
{
	Super::Open_Implementation(bOpen);
	if (bOpen)
		ResetSelectedOptionsToUserSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	RefreshLineOptions();
	LanguageOptions = USoLocalizationHelper::GetAllConfigurableCultures();

	// Set the text of the widgets
	if (Language)
	{
		TArray<FText> LanguageTexts;
		for (ESoSupportedCulture Culture : LanguageOptions)
		{
			LanguageTexts.Add(USoLocalizationHelper::GetDisplayTextFromSupportedCulture(Culture, true));
		}

		Language->CreateButtons(LanguageTexts);
	}

	if (CharacterSkin)
	{
		TArray<FText> SkinTexts;
		SkinTexts.Add(FROM_STRING_TABLE_UI("character_skin_classic"));
		SkinTexts.Add(FROM_STRING_TABLE_UI("character_skin_mummy"));
		SkinTexts.Add(FROM_STRING_TABLE_UI("character_skin_pumpkin"));
		CharacterSkin->CreateButtons(SkinTexts);
	}

	if (GameSpeed)
	{
		TArray<FText> GameSpeedTexts;
		for (int32 SpeedNormalized : GameSpeedOptions)
		{
			GameSpeedTexts.Add(
				FText::FromString(
					FString::Printf(TEXT("%.2fx"), FromNormalizedGameSpeed(SpeedNormalized))
				)
			);
		}
		GameSpeed->CreateButtons(GameSpeedTexts);
	}

	if (MercifulBounceMode)
	{
		TArray<FText> BounceModeTexts;
		BounceModeTexts.Add(FROM_STRING_TABLE_UI("settings_merciful_bounce_never"));
		BounceModeTexts.Add(FROM_STRING_TABLE_UI("settings_merciful_bounce_on_easy"));
		BounceModeTexts.Add(FROM_STRING_TABLE_UI("settings_merciful_bounce_always"));
		MercifulBounceMode->CreateButtons(BounceModeTexts);
	}

	// Set the Lines order and text
	EmptyLinesAndTitles();
	for (const ESoUIGameSettingsLine Option : LineOptions)
	{
		switch (Option)
		{
		case ESoUIGameSettingsLine::Language:
			AddLineWithText(Language, FROM_STRING_TABLE_UI("settings_language"));
			break;

		case ESoUIGameSettingsLine::CharacterSkin:
			AddLineWithText(CharacterSkin, FROM_STRING_TABLE_UI("character_skin_title"));
			break;

		case ESoUIGameSettingsLine::GameSpeed:
			AddLineWithText(GameSpeed, FROM_STRING_TABLE_UI("settings_game_speed"));
			break;

		case ESoUIGameSettingsLine::MercifulBounceMode:
			AddLineWithText(MercifulBounceMode, FROM_STRING_TABLE_UI("settings_merciful_bounce"));
			break;

		case ESoUIGameSettingsLine::ShowFPS:
			AddLineWithText(DisplayFPS, FROM_STRING_TABLE_UI("settings_display_fps"));
			break;

		case ESoUIGameSettingsLine::ShowTime:
			AddLineWithText(DisplayTime, FROM_STRING_TABLE_UI("settings_display_play_time"));
			break;

		case ESoUIGameSettingsLine::ShowDamageTexts:
			AddLineWithText(DisplayDamageTexts, FROM_STRING_TABLE_UI("settings_display_damage"));
			break;

		case ESoUIGameSettingsLine::ShowEnemyHealthBar:
			AddLineWithText(DisplayEnemyHealth, FROM_STRING_TABLE_UI("settings_display_enemy_hp"));
			break;

		case ESoUIGameSettingsLine::ShowFloatingVOTexts:
			AddLineWithText(DisplayFloatingVOTexts, FROM_STRING_TABLE_UI("settings_display_floating_vo"));
			break;

		case ESoUIGameSettingsLine::PauseGameWhenUnfocused:
			AddLineWithText(PauseGameWhenUnfocused, FROM_STRING_TABLE_UI("settings_pause_game_unfocused"));
			break;

		case ESoUIGameSettingsLine::PauseGameGameOnIdle:
			AddLineWithText(PauseGameWhenIdle, FROM_STRING_TABLE_UI("settings_pause_game_idle"));
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	if (LineOptions.Num() != NumLines())
		UE_LOG(LogSoUISettings, Error, TEXT("Game CreateChildrenAndSetTexts: LineOptions.Num(%d) != NumLines(%d)"), LineOptions.Num(), NumLines());

	Super::CreateChildrenAndSetTexts(bFromSynchronizeProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	Super::OnPressedChild(SelectedChild, SoUserWidget);
	if (!IsValidLineIndex(SelectedChild))
		return;

	bool bHandled = true;
	const ESoUIGameSettingsLine SelectedLineOption = LineOptions[SelectedChild];
	switch (SelectedLineOption)
	{
	case ESoUIGameSettingsLine::ShowFPS:
		UserSettings->SetDisplayFPSEnabled(DisplayFPS->IsChecked());
		break;

	case ESoUIGameSettingsLine::ShowTime:
		UserSettings->SetDisplayTimeEnabled(DisplayTime->IsChecked());
		break;

	case ESoUIGameSettingsLine::ShowDamageTexts:
		UserSettings->SetDisplayDamageTexts(DisplayDamageTexts->IsChecked());
		break;

	case ESoUIGameSettingsLine::ShowEnemyHealthBar:
		UserSettings->SetDisplayEnemyHealthBar(DisplayEnemyHealth->IsChecked());
		break;

	case ESoUIGameSettingsLine::ShowFloatingVOTexts:
		UserSettings->SetDisplayFloatingVOLines(DisplayFloatingVOTexts->IsChecked());
		break;

	case ESoUIGameSettingsLine::PauseGameWhenUnfocused:
		UserSettings->SetPauseGameWhenUnfocused(PauseGameWhenUnfocused->IsChecked());
		break;

	case ESoUIGameSettingsLine::PauseGameGameOnIdle:
		UserSettings->SetPauseGameOnIdle(PauseGameWhenIdle->IsChecked());
		break;

	default:
		bHandled = false;
		break;
	}

	if (bHandled)
	{
		UserSettings->ApplyGameSettings(true);
		ResetSelectedOptionsToUserSettings();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::OnPostSetSelectedIndex(int32 OldIndex, int32 NewIndex)
{
	Super::OnPostSetSelectedIndex(OldIndex, NewIndex);

	// Set line tooltips
	if (LineOptions.IsValidIndex(GetSelectedLineIndex()))
	{
		const ESoUIGameSettingsLine CurrentType = LineOptions[GetSelectedLineIndex()];
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
void USoUISettingsGame::UpdateWidgetsVisibilities()
{
	if (CharacterSkin)
	{
		CharacterSkin->SetVisibility(
			LineOptions.Contains(ESoUIGameSettingsLine::CharacterSkin) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}
	if (DisplayFPS)
	{
		DisplayFPS->SetVisibility(
			LineOptions.Contains(ESoUIGameSettingsLine::ShowFPS) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}
	if (PauseGameWhenUnfocused)
	{
		PauseGameWhenUnfocused->SetVisibility(
			LineOptions.Contains(ESoUIGameSettingsLine::PauseGameWhenUnfocused) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}
	if (GameSpeed)
	{
		GameSpeed->SetVisibility(
			LineOptions.Contains(ESoUIGameSettingsLine::GameSpeed) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::RefreshLineOptions()
{
	LineOptions = {
		ESoUIGameSettingsLine::Language
	};

	if (!USoPlatformHelper::IsDemo())
		LineOptions.Add(ESoUIGameSettingsLine::CharacterSkin);

	LineOptions.Append({
		ESoUIGameSettingsLine::GameSpeed,
		ESoUIGameSettingsLine::MercifulBounceMode
	});

	if (!USoPlatformHelper::IsConsole())
		LineOptions.Add(ESoUIGameSettingsLine::ShowFPS);

	LineOptions.Append({
		ESoUIGameSettingsLine::ShowTime,
		ESoUIGameSettingsLine::ShowDamageTexts,
		ESoUIGameSettingsLine::ShowEnemyHealthBar,
		ESoUIGameSettingsLine::ShowFloatingVOTexts
	});

	if (!USoPlatformHelper::IsConsole())
		LineOptions.Add(ESoUIGameSettingsLine::PauseGameWhenUnfocused);

	LineOptions.Add(ESoUIGameSettingsLine::PauseGameGameOnIdle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::ResetSelectedOptionsToUserSettings()
{
	UserSettings->SetBlockConfigSave(true);

	UserSettings->ResetToCurrentSettings();
	UserSettings->ValidateSettings();

	if (Language)
	{
		// Set the current language
		const ESoSupportedCulture CurrentCulture = USoLocalizationHelper::GetCurrentLanguageNameType();
		const int32 CurrentCultureIndex = LanguageOptions.Find(CurrentCulture);
		if (LanguageOptions.IsValidIndex(CurrentCultureIndex))
		{
			Language->SetSelectedIndex(CurrentCultureIndex);
		}
		else
		{
			Language->SetSelectedIndex(0);
		}
	}

	if (CharacterSkin)
		CharacterSkin->SetSelectedIndex(static_cast<int32>(UserSettings->GetCharacterSkinType()));

	if (GameSpeed)
		GameSpeed->SetSelectedIndex(GetIndexOptionForGameSpeed(UserSettings->GetGameSpeed()));

	if (MercifulBounceMode)
		MercifulBounceMode->SetSelectedIndex(static_cast<int32>(UserSettings->GetBounceModeType()), false);

	if (DisplayFPS)
		DisplayFPS->SetIsChecked(UserSettings->IsDisplayFPSEnabled());

	if (DisplayTime)
		DisplayTime->SetIsChecked(UserSettings->IsDisplayTimeEnabled());

	if (DisplayDamageTexts)
		DisplayDamageTexts->SetIsChecked(UserSettings->IsDisplayDamageTextsEnabled());

	if (DisplayFloatingVOTexts)
		DisplayFloatingVOTexts->SetIsChecked(UserSettings->IsDisplayFloatingVOLinesEnabled());

	if (PauseGameWhenUnfocused)
		PauseGameWhenUnfocused->SetIsChecked(UserSettings->CanPauseGameWhenUnfocused());

	if (PauseGameWhenIdle)
		PauseGameWhenIdle->SetIsChecked(UserSettings->CanPauseGameOnIdle());

	if (DisplayEnemyHealth)
		DisplayEnemyHealth->SetIsChecked(UserSettings->IsDisplayEnemyHealthBarEnabled());

	UserSettings->SetBlockConfigSave(false);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::OnLanguageChildChanged(int32 PreviousChild, int32 NewChild)
{
	if (!bOpened)
		return;

	if (!LanguageOptions.IsValidIndex(NewChild))
		return;

	const ESoSupportedCulture NewLanguage = LanguageOptions[NewChild];
	USoLocalizationHelper::SetCurrentCultureEverywhere(NewLanguage, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::OnCharacterSkinChanged(int32 PreviousChild, int32 NewChild)
{
	if (!bOpened)
		return;

	UserSettings->SetCharacterSkinType(static_cast<ESoCharacterSkinType>(NewChild));
	UserSettings->ApplyGameSettings(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::OnGameSpeedChildChanged(int32 PreviousChild, int32 NewChild)
{
	if (!bOpened)
		return;

	if (!GameSpeedOptions.IsValidIndex(NewChild))
		return;

	const float NewGameSpeed = FromNormalizedGameSpeed(GameSpeedOptions[NewChild]);
	UserSettings->SetGameSpeed(NewGameSpeed);
	UserSettings->ApplyGameSettings(true);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsGame::OnMercifulBounceModeChildChanged(int32 PreviousChild, int32 NewChild)
{
	if (!bOpened)
		return;

	UserSettings->SetBounceModeType(static_cast<ESoBounceModeType>(NewChild));
	UserSettings->ApplyGameSettings(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISettingsGame::GetIndexOptionForGameSpeed(float Value) const
{
	const int32 NormalizedValue = ToNormalizedGameSpeed(Value);

	// Found Valid Index
	const int32 FoundIndex = GameSpeedOptions.Find(NormalizedValue);
	if (GameSpeedOptions.IsValidIndex(FoundIndex))
		return FoundIndex;

	// NOTE: assumes array is sorted
	// Get the closest one (lower bound)
	for (int32 Index = 0; Index < GameSpeedOptions.Num(); Index++)
	{
		// It Means the previous value is the correct one
		if (GameSpeedOptions[Index] > NormalizedValue)
			return Index > 0 ? Index - 1 : 0;
	}

	// Sane default I guess
	return 0;
}
