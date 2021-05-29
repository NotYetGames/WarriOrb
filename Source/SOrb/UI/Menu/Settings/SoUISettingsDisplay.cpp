// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISettingsDisplay.h"

#include "GameFramework/GameUserSettings.h"
#include "Components/TextBlock.h"

#include "Basic/Helpers/SoPlatformHelper.h"
#include "Basic/SoGameSingleton.h"
#include "Settings/SoGameSettings.h"

#include "UI/General/Buttons/SoUIScrollingButtonArray.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/SoUISlider.h"
#include "UI/General/SoUICheckbox.h"
#include "Basic/SoAudioManager.h"
#include "../ConfirmPanels/SoUIConfirmDisplaySettings.h"
#include "Localization/SoLocalization.h"

static constexpr int32 FrameLimitUnlimitedOption = INDEX_NONE;

static float FrameLimitOptionToFloat(int32 Option)
{
	// Unlimited
	if (Option == FrameLimitUnlimitedOption || Option <= 0)
		return 9999.f;

	return static_cast<float>(Option);
}

static int32 FloatToFrameLimitOption(float InNumber)
{
	// Unlimited
	if (InNumber > 1000.f)
		return FrameLimitUnlimitedOption;

	const int32 FrameLimit =  static_cast<int32>(InNumber);

	// Return sane default
	return FrameLimit <= 0 ? INDEX_NONE : FrameLimit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::NativeConstruct()
{
	Super::NativeConstruct();
	CurrentTooltipText->SetVisibility(ESlateVisibility::Collapsed);

	// Listen to events
	if (ConfirmDisplaySettings)
		ConfirmDisplaySettings->OnOpenChangedEvent().AddDynamic(this, &ThisClass::OnConfirmPanelOpenChanged);

	// First line
	if (DisplayMode)
	{
		DisplayMode->OnSelectedChildChangedEvent().AddLambda([this](int32 PreviousChild, int32 NewChild)
		{
			if (!bOpened)
				return;

			UpdateResolutionLineOptionsFromSelectedOptions();
			OnScrollingArrayChildChanged(PreviousChild, NewChild);
		});

		DisplayMode->SetSelectedIndex(DisplayModeOptions.Find(EWindowMode::ConvertIntToWindowMode(0)));
	}

	if (AspectRatio)
		AspectRatio->OnSelectedChildChangedEvent().AddLambda([this](int32 PreviousChild, int32 NewChild)
		{
			if (!bOpened)
				return;

			UpdateResolutionLineOptionsFromSelectedOptions();
			OnScrollingArrayChildChanged(PreviousChild, NewChild);
		});

	if (Resolution)
		Resolution->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnScrollingArrayChildChanged);

	// TODO make it CPP friendly
	if (ResolutionScale)
		ResolutionScale->OnValueChangedEvent().AddDynamic(this, &ThisClass::OnResolutionScaleValueChanged);

	if (FrameLimit)
		FrameLimit->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnScrollingArrayChildChanged);
	if (Shadows)
		Shadows->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnScrollingArrayChildChanged);
	if (Textures)
		Textures->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnScrollingArrayChildChanged);
	if (Effects)
		Effects->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnScrollingArrayChildChanged);
	if (PostEffects)
		PostEffects->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnScrollingArrayChildChanged);
	if (Antialiasing)
		Antialiasing->OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnScrollingArrayChildChanged);


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::NativeDestruct()
{
	if (DisplayMode)
		DisplayMode->OnSelectedChildChangedEvent().RemoveAll(this);
	if (AspectRatio)
		AspectRatio->OnSelectedChildChangedEvent().RemoveAll(this);
	if (Resolution)
		Resolution->OnSelectedChildChangedEvent().RemoveAll(this);
	if (FrameLimit)
		FrameLimit->OnSelectedChildChangedEvent().RemoveAll(this);
	if (Shadows)
		Shadows->OnSelectedChildChangedEvent().RemoveAll(this);
	if (Textures)
		Textures->OnSelectedChildChangedEvent().RemoveAll(this);
	if (Antialiasing)
		Antialiasing->OnSelectedChildChangedEvent().RemoveAll(this);
	if (ResolutionScale)
		ResolutionScale->OnValueChangedEvent().RemoveDynamic(this, &ThisClass::OnResolutionScaleValueChanged);
	if (ConfirmDisplaySettings)
		ConfirmDisplaySettings->OnOpenChangedEvent().RemoveDynamic(this, &ThisClass::OnConfirmPanelOpenChanged);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::CreateChildrenAndSetTexts(bool bFromSynchronizeProperties)
{
	// Set the texts of the widgets
	// TODO check USoPlatformHelper::SupportsWindowedMode()
	if (DisplayMode)
	{
		TArray<FText> DisplayModeTexts;
		for (const EWindowMode::Type Option : DisplayModeOptions)
		{
			switch (Option)
			{
				case EWindowMode::Fullscreen:
					DisplayModeTexts.Add(FROM_STRING_TABLE_UI("settings_fullscreen"));
					break;
				case EWindowMode::Windowed:
					DisplayModeTexts.Add(FROM_STRING_TABLE_UI("settings_windowed"));
					break;
				case EWindowMode::WindowedFullscreen:
					DisplayModeTexts.Add(FROM_STRING_TABLE_UI("settings_borderless"));
					break;
				default:
					DisplayModeTexts.Add(FROM_STRING_TABLE_UI("settings_unknown"));
					break;
			}
		}
		DisplayMode->CreateButtons(DisplayModeTexts);
	}

	if (AspectRatio)
	{
		TArray<FText> AspectRatioTexts;
		for (const ESoDisplayAspectRatio Option : AspectRatioOptions)
		{
			switch (Option)
			{
				case ESoDisplayAspectRatio::Ratio_4_3:
					AspectRatioTexts.Add(FROM_STRING_TABLE_UI("settings_ratio_4_3"));
					break;
				case ESoDisplayAspectRatio::Ratio_16_10:
					AspectRatioTexts.Add(FROM_STRING_TABLE_UI("settings_ratio_16_10"));
					break;
				case ESoDisplayAspectRatio::Ratio_16_9:
					AspectRatioTexts.Add(FROM_STRING_TABLE_UI("settings_ratio_16_9"));
					break;
				default:
					AspectRatioTexts.Add(FROM_STRING_TABLE_UI("settings_unknown"));
					break;
			}
		}
		AspectRatio->CreateButtons(AspectRatioTexts);
	}

	const TArray<FText> GeneralEpicSettingsTexts = GetTextsForQualityOptions(GeneralQualityOptions);
	const TArray<FText> LimitedMediumEpicSettingsTexts = GetTextsForQualityOptions(LimitedMediumQualityOptions);
	const TArray<FText> LimitedHighEpicSettingsTexts = GetTextsForQualityOptions(LimitedHighQualityOptions);
	if (Shadows)
		Shadows->CreateButtons(GeneralEpicSettingsTexts);

	if (Textures)
		Textures->CreateButtons(GeneralEpicSettingsTexts);

	if (Effects)
		Effects->CreateButtons(LimitedMediumEpicSettingsTexts);

	if (PostEffects)
		PostEffects->CreateButtons(GeneralEpicSettingsTexts);

	if (Antialiasing)
		Antialiasing->CreateButtons(GeneralEpicSettingsTexts);

	// Set the Lines order and text
	EmptyLinesAndTitles();
	for (const ESoUISettingsDisplayLine Option : LineOptions)
	{
		switch (Option)
		{
			case ESoUISettingsDisplayLine::DisplayMode:
				AddLineWithText(DisplayMode, FROM_STRING_TABLE_UI("settings_display_mode"));
				break;
			case ESoUISettingsDisplayLine::AspectRatio:
				AddLineWithText(AspectRatio, FROM_STRING_TABLE_UI("settings_aspect_ratio"));
				break;
			case ESoUISettingsDisplayLine::Resolution:
				AddLineWithText(Resolution, FROM_STRING_TABLE_UI("settings_resolution"));
				break;
			case ESoUISettingsDisplayLine::ResolutionScale:
				AddLineWithText(ResolutionScale, FROM_STRING_TABLE_UI("settings_resolution_scale"));
				break;
			case ESoUISettingsDisplayLine::VSync:
				AddLineWithText(VSync, FROM_STRING_TABLE_UI("settings_vsync"));
				break;
			case ESoUISettingsDisplayLine::FrameLimit:
				AddLineWithText(FrameLimit, FROM_STRING_TABLE_UI("settings_frame_limit"));
				break;
			case ESoUISettingsDisplayLine::Shadows:
				AddLineWithText(Shadows, FROM_STRING_TABLE_UI("settings_shadows"));
				break;
			case ESoUISettingsDisplayLine::Textures:
				AddLineWithText(Textures, FROM_STRING_TABLE_UI("settings_textures"));
				break;
			case ESoUISettingsDisplayLine::Effects:
				AddLineWithText(Effects, FROM_STRING_TABLE_UI("settings_effects"));
				break;
			case ESoUISettingsDisplayLine::PostEffects:
				AddLineWithText(PostEffects, FROM_STRING_TABLE_UI("settings_post_effects"));
				break;
			case ESoUISettingsDisplayLine::AntiAliasing:
				AddLineWithText(Antialiasing, FROM_STRING_TABLE_UI("settings_antialising"));
				break;
			case ESoUISettingsDisplayLine::VolumetricFog:
				AddLineWithText(VolumetricFog, FROM_STRING_TABLE_UI("settings_volumetric_fog"));
				break;

			case ESoUISettingsDisplayLine::ForceWeakHardwareOptimization:
				AddLineWithText(ForceWeakHardwareOptimization, FROM_STRING_TABLE_UI("settings_force_weak_hardware_opt"));
				break;

			default:
				checkNoEntry();
				break;
		}
	}

	if (LineOptions.Num() != NumLines())
		UE_LOG(LogSoUISettings, Error, TEXT("Display CreateChildrenAndSetTexts: LineOptions.Num(%d) != NumLines(%d)"), LineOptions.Num(), NumLines());

	Super::CreateChildrenAndSetTexts(bFromSynchronizeProperties);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FText> USoUISettingsDisplay::GetTextsForQualityOptions(const TArray<ESoUISettingsEpicQualityOption>& InQualityOptions)
{
	TArray<FText> Texts;
	for (const ESoUISettingsEpicQualityOption Option : InQualityOptions)
	{
		switch (Option)
		{
			case ESoUISettingsEpicQualityOption::Low:
				Texts.Add(FROM_STRING_TABLE_UI("settings_quality_low"));
				break;
			case ESoUISettingsEpicQualityOption::Medium:
				Texts.Add(FROM_STRING_TABLE_UI("settings_quality_medium"));
				break;
			case ESoUISettingsEpicQualityOption::High:
				Texts.Add(FROM_STRING_TABLE_UI("settings_quality_high"));
				break;
			case ESoUISettingsEpicQualityOption::Epic:
				Texts.Add(FROM_STRING_TABLE_UI("settings_quality_epic"));
				break;
			default:
				Texts.Add(FROM_STRING_TABLE_UI("settings_unknown"));
				break;
		}
	}
	return Texts;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::Open_Implementation(bool bOpen)
{
	Super::Open_Implementation(bOpen);

	if (bOpened)
		// Reset value from the system
		ResetSelectedOptionsToUserSettings();
	else
		// Simulate reset buttons pressed on window closed;
		OnCommandResetSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsDisplay::HandleOnUICommand(ESoUICommand Command)
{
	if (IsConfirmMode())
	{
		// Handle in confirm panel
		ISoUIEventHandler::Execute_OnUICommand(ConfirmDisplaySettings, Command);
		return true;
	}

	if (Super::HandleOnUICommand(Command))
		return true;

	return OnPressedButtonTooltipsCommand(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	Super::OnPressedChild(SelectedChild, SoUserWidget);
	UpdateLinesVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsDisplay::NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput)
{
	if (!Super::NavigateLeftRightOnSelectedLine(Command, bFromPressedInput))
		return false;

	// Handled in super, I hope
	if (IsOnButtonTooltips())
		return true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnPostSetSelectedIndex(int32 OldIndex, int32 NewIndex)
{
	Super::OnPostSetSelectedIndex(OldIndex, NewIndex);

	// Set line tooltips
	const int32 SelectedLineIndex = GetSelectedLineIndex();
	if (LineOptions.IsValidIndex(SelectedLineIndex))
	{
		const ESoUISettingsDisplayLine CurrentType = LineOptions[SelectedLineIndex];
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
void USoUISettingsDisplay::RefreshButtonImagesTooltips(bool bForceShowAll)
{
	if (!ButtonImagesTooltipsArray)
		return;

	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	if (bForceShowAll || DidDefaultSettingsChange())
	{
		CommandsToBeDisplayed.Add({ UICommand_ApplySettings, FROM_STRING_TABLE_UI("settings_apply") });
		CommandsToBeDisplayed.Add({ UICommand_ResetSettings, FROM_STRING_TABLE_UI("settings_reset") });
	}

	ButtonImagesTooltipsArray->UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsDisplay::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	switch (Command)
	{
	case UICommand_ApplySettings:
		OnCommandApplySettings();
		break;

	case UICommand_ResetSettings:
		OnCommandResetSettings();
		break;

	default:
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnCommandApplySettings()
{
	if (DidDefaultSettingsChange())
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsApply));

	ApplyUserSettingsFromSelectedOptions();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnCommandResetSettings()
{
	if (DidDefaultSettingsChange())
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::SettingsRestore));

	ResetSelectedOptionsToUserSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoUISettingsEpicQualityOption USoUISettingsDisplay::GetEffectsQualityOption() const
{
	if (!Effects)
		return ESoUISettingsEpicQualityOption::Num;

	const int32 SelectedElementIndex = Effects->GetSelectedIndex();
	if (LimitedMediumQualityOptions.IsValidIndex(SelectedElementIndex))
	{
		return LimitedMediumQualityOptions[SelectedElementIndex];;
	}

	return ESoUISettingsEpicQualityOption::Num;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoUISettingsEpicQualityOption USoUISettingsDisplay::GetShadowQualityOption() const
{
	if (!Shadows)
		return ESoUISettingsEpicQualityOption::Num;

	const int32 SelectedElementIndex = Shadows->GetSelectedIndex();
	if (GeneralQualityOptions.IsValidIndex(SelectedElementIndex))
	{
		return GeneralQualityOptions[SelectedElementIndex];
	}

	return ESoUISettingsEpicQualityOption::Num;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::ApplyUserSettingsFromSelectedOptions()
{
	if (IsConfirmMode())
		return;

	bool bOpenConfirmPanel = false;
	if (DisplayMode)
	{
		const int32 SelectedElementIndex = DisplayMode->GetSelectedIndex();
		if (DisplayModeOptions.IsValidIndex(SelectedElementIndex))
		{
			const EWindowMode::Type SelectedDisplayMode = DisplayModeOptions[SelectedElementIndex];
			bOpenConfirmPanel = UserSettings->GetLastConfirmedFullscreenMode() != SelectedDisplayMode;

			// For Borderless, enforce native screen resolution from the main desktop monitor.
			// Will be automatically enforced in UpdateResolutionQuality();
			UserSettings->SetFullscreenMode(SelectedDisplayMode);
		}
	}

	if (AspectRatio)
	{
		//const int32 SelectedElementIndex = AspectRatio->GetSelectedIndex();
	}

	if (Resolution)
	{
		const int32 SelectedElementIndex = Resolution->GetSelectedIndex();
		if (ResolutionOptions.IsValidIndex(SelectedElementIndex))
		{
			const FSoDisplayResolution SelectedResolution = ResolutionOptions[SelectedElementIndex];
			bOpenConfirmPanel = bOpenConfirmPanel ||
				SelectedResolution.Resolution != UserSettings->GetLastConfirmedScreenResolution();

			UserSettings->SetScreenResolution(SelectedResolution.Resolution);
		}

		// RequestResolutionChange is called in ApplySettings bellow
	}

	if (ResolutionScale)
	{
		// Can be affected by other options
		if (bResolutionScaleChanged)
		{
			UserSettings->SetScreenPercentageNormalized(ResolutionScale->GetValueNormalized());
		}
	}

	if (VSync)
	{
		// Note won't work in Editor env as it trips up the GPU profiler. This is set in Config/BaseEngine.ini (inside the editor folder)
		// Besides r.VSync there is also r.VSyncEditor, both are false by default
#if PLATFORM_XBOXONE
		UserSettings->SetVSyncEnabled(true);
#else
		UserSettings->SetVSyncEnabled(VSync->IsChecked());
#endif
	}

	if (FrameLimit)
	{
		// Disabled by default
		float FrameLimitRate = 0.f;

		// Only set frame limit when VSync is not enabled
		if (!UserSettings->IsVSyncEnabled())
		{
			const int32 SelectedElementIndex = FrameLimit->GetSelectedIndex();
			if (FrameLimitOptions.IsValidIndex(SelectedElementIndex))
			{
				const int32 LimitOption = FrameLimitOptions[SelectedElementIndex];
				FrameLimitRate = FrameLimitOptionToFloat(LimitOption);
			}
		}
		UserSettings->SetFrameRateLimit(FrameLimitRate);
	}

	if (Shadows)
	{
		const ESoUISettingsEpicQualityOption Quality = GetShadowQualityOption();
		if (Quality != ESoUISettingsEpicQualityOption::Num)
		{
			UserSettings->SetShadowQuality(static_cast<int32>(Quality));
		}
	}

	if (Textures)
	{
		const int32 SelectedElementIndex = Textures->GetSelectedIndex();
		if (GeneralQualityOptions.IsValidIndex(SelectedElementIndex))
		{
			const ESoUISettingsEpicQualityOption Quality = GeneralQualityOptions[SelectedElementIndex];
			UserSettings->SetTextureQuality(static_cast<int32>(Quality));
		}
	}

	if (Effects)
	{
		const ESoUISettingsEpicQualityOption Quality = GetEffectsQualityOption();
		if (Quality != ESoUISettingsEpicQualityOption::Num)
		{
			UserSettings->SetVisualEffectQuality(static_cast<int32>(Quality));
		}
	}

	if (PostEffects)
	{
		const int32 SelectedElementIndex = PostEffects->GetSelectedIndex();
		if (GeneralQualityOptions.IsValidIndex(SelectedElementIndex))
		{
			const ESoUISettingsEpicQualityOption Quality = GeneralQualityOptions[SelectedElementIndex];
			UserSettings->SetPostProcessingQuality(static_cast<int32>(Quality));
		}
	}

	if (Antialiasing)
	{
		const int32 SelectedElementIndex = Antialiasing->GetSelectedIndex();
		if (GeneralQualityOptions.IsValidIndex(SelectedElementIndex))
		{
			const ESoUISettingsEpicQualityOption Quality = GeneralQualityOptions[SelectedElementIndex];
			UserSettings->SetAntiAliasingQuality(static_cast<int32>(Quality));
		}
	}

	if (VolumetricFog)
	{
		UserSettings->SetVolumetricFogEnabled(VolumetricFog->IsChecked());
	}

	if (ForceWeakHardwareOptimization)
	{
		UserSettings->SetForceWeakHardwareOptimization(ForceWeakHardwareOptimization->IsChecked());
	}

	if (bOpenConfirmPanel)
	{
		// Display overlay and let the timer tick
		// NOTE: We only need to preview the settings here
		UserSettings->PreviewVideoModeSettings();
		OpenConfirmPanel();
	}
	else
	{
		// Refresh the selected options after applying
		UserSettings->ApplyDisplaySettings(true);
		ResetSelectedOptionsToUserSettings();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::ResetSelectedOptionsToUserSettings()
{
	// Try to reset to the current settings, if the user settings were dirty (user settings did not match live settings)
	// this will  (try) fix that. Settings get dirty because the user sets them in the command line, console, etc.
	// NOTE: that if the user sets some options by some other priority (console or command line override)
	// changes in the UI will not applied as this has lower priority. See https://docs.unrealengine.com/latest/INT/Programming/Development/Tools/ConsoleManager/
	// NOTE: In some cases like in an Editor env some settings like VSync will be always false or true
	// Depending if they set r.VSync in the Engine.ini (SystemSettings section).
	UserSettings->ResetToCurrentSettings();

	// Update lines
	UpdateLinesFromUserSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::UpdateLinesVisibility()
{
	if (!DisplayMode)
		return;

	// From the User settings
	EWindowMode::Type WindowDisplayMode = UserSettings->GetFullscreenMode();
	if (DisplayMode)
	{
		// From the displayed  options
		const int32 SelectedElementIndex = DisplayMode->GetSelectedIndex();
		if (DisplayModeOptions.IsValidIndex(SelectedElementIndex))
			WindowDisplayMode = DisplayModeOptions[SelectedElementIndex];
	}

	// Consoles usually have fixed resolution
	const bool bHasFixedResolution = USoPlatformHelper::HasFixedResolution();
	const bool bIsBorderless = WindowDisplayMode == EWindowMode::WindowedFullscreen;
	if (AspectRatio)
		AspectRatio->SetIsEnabled(!bIsBorderless && !bHasFixedResolution);
	if (Resolution)
		Resolution->SetIsEnabled(!bIsBorderless && !bHasFixedResolution);

	// Only enable it if shadow quality is high or epic
	if (VolumetricFog)
		VolumetricFog->SetIsEnabled(static_cast<int32>(GetShadowQualityOption()) >= 2);

	// Consoles usually have fixed VSync
	if (VSync)
	{
		const bool bHasFixedVSync = USoPlatformHelper::HasFixedVSync();
		VSync->SetIsEnabled(!bHasFixedVSync);

		const bool bIsVSync = VSync->IsChecked();
		if (FrameLimit)
			FrameLimit->SetIsEnabled(!bIsVSync);
	}

	// True on some consoles
	const bool bHasFixedWeakHardware = USoPlatformHelper::HasFixedWeakHardware();
	if (ForceWeakHardwareOptimization)
		ForceWeakHardwareOptimization->SetIsEnabled(!bHasFixedWeakHardware);

	EmptyAndRefreshButtonImagesTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::UpdateLinesFromUserSettings()
{
	UserSettings->ValidateSettings();

	// NOTE: we do not check if the ints are valid because if they are not they are simple ignored anyways
	const EWindowMode::Type CurrentWindowMode = UserSettings->GetFullscreenMode();
	DisplayMode->SetSelectedIndex(static_cast<int32>(CurrentWindowMode));

	// Update the resolution options first so what we have to select from something
	const FSoDisplayResolution CurrentResolution(UserSettings->GetScreenResolution());
	UpdateResolutionLineOptions(CurrentResolution, CurrentResolution.AspectRatio, CurrentWindowMode, UserSettings->GetDesktopResolution());

	AspectRatio->SetSelectedIndex(AspectRatioOptions.Find(CurrentResolution.AspectRatio));
	UpdateResolutionScaleFromUserSettings();

	const bool bIsVSyncEnabled = UserSettings->IsVSyncEnabled();
	VSync->SetIsChecked(bIsVSyncEnabled);

	if (bIsVSyncEnabled)
		FrameLimit->SetSelectedIndex(INDEX_NONE);
	else
		UpdateFrameLimitFromUserSettings();

	Shadows->SetSelectedIndex(GetIndexForGeneralQuality(UserSettings->GetShadowQuality()));
	Textures->SetSelectedIndex(GetIndexForGeneralQuality(UserSettings->GetTextureQuality()));
	Effects->SetSelectedIndex(GetIndexForLimitedMediumQuality(UserSettings->GetVisualEffectQuality()));
	PostEffects->SetSelectedIndex(GetIndexForGeneralQuality(UserSettings->GetPostProcessingQuality()));
	Antialiasing->SetSelectedIndex(GetIndexForGeneralQuality(UserSettings->GetAntiAliasingQuality()));
	VolumetricFog->SetIsChecked(UserSettings->IsVolumetricFogEnabled());
	ForceWeakHardwareOptimization->SetIsChecked(UserSettings->IsWeakHardwareOptimizationForced());

	ReloadDefaultSettings();
	UpdateLinesVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::UpdateResolutionLineOptionsFromSelectedOptions()
{
	if (Resolution == nullptr || AspectRatio == nullptr)
		return;

	int32 SelectedAspectRatioIndex;
	int32 SelectedDisplayModeIndex;
	int32 SelectedResolutionIndex;
	if (!AspectRatio->GetSelectedIndexChecked(SelectedAspectRatioIndex))
		return;
	if (!DisplayMode->GetSelectedIndexChecked(SelectedDisplayModeIndex))
		return;
	if (!Resolution->GetSelectedIndexChecked(SelectedResolutionIndex))
		return;

	const FSoDisplayResolution CurrentResolution = ResolutionOptions[SelectedResolutionIndex].Resolution;
	const ESoDisplayAspectRatio CurrentAspectRatio = AspectRatioOptions[SelectedAspectRatioIndex];
	const EWindowMode::Type CurrentDisplayMode = DisplayModeOptions[SelectedDisplayModeIndex];
	UpdateResolutionLineOptions(CurrentResolution, CurrentAspectRatio, CurrentDisplayMode, UserSettings->GetDesktopResolution());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::UpdateResolutionLineOptions(const FSoDisplayResolution& CurrentResolution,
	ESoDisplayAspectRatio CurrentAspectRatio,
	EWindowMode::Type CurrentDisplayMode, const FIntPoint& MaxResolutionIntPoint)
{
	if (Resolution == nullptr)
		return;

	const FSoDisplayResolution MaxResolution(MaxResolutionIntPoint);
	if (USoPlatformHelper::HasFixedResolution())
	{
		// Console most likeley fixed resolution and it is the maximum
		ResolutionOptions.Empty();
		ResolutionOptions.Add(MaxResolution);
	}
	else
	{
		// Desktop?
		// TODO check UKismetSystemLibrary::GetConvenientWindowedResolutions and
		// UKismetSystemLibrary::GetSupportedFullscreenResolutions and UGameEngine::DetermineGameWindowResolution
		// Create display options depending on the aspect ratio

		// Try first to query from the system the supported resolutions.
		bool bCouldQuerySupportedResolutions = false;
		TArray<FSoDisplayResolution> SupportedResolutions;

		// On both fullscreen and windowed mode we get both resolutions
		USoPlatformHelper::GetSupportedFullscreenResolutions(SupportedResolutions);

		// Try also the windowed options
		if (CurrentDisplayMode == EWindowMode::Windowed)
			USoPlatformHelper::GetConvenientWindowedResolutions(SupportedResolutions);

		// Combine preset values from the system with the preset ones.
		TSet<FSoDisplayResolution> AllResolutions;
		ResolutionOptions.Empty();

		// Populate with the resolutions from the system
		for (const FSoDisplayResolution& SupportedResolution : SupportedResolutions)
			if (SupportedResolution.AspectRatio == CurrentAspectRatio)
				// Is the correct aspect ratio
				AllResolutions.Add(SupportedResolution);

		// Populate with preset values
		TArray<FSoDisplayResolution> PresetResolutionOptions;
		switch (CurrentAspectRatio)
		{
			case ESoDisplayAspectRatio::Ratio_4_3:
				PresetResolutionOptions = Resolution_4_3_PresetOptions;
				break;
			case ESoDisplayAspectRatio::Ratio_16_9:
				PresetResolutionOptions = Resolution_16_9_PresetOptions;
				break;
			case ESoDisplayAspectRatio::Ratio_16_10:
				PresetResolutionOptions = Resolution_16_10_PresetOptions;
				break;
			default:
				break;
		}

		// Filter out resolution options larger then resolution of our main display
		for (const FSoDisplayResolution& ResolutionOption : PresetResolutionOptions)
			if (ResolutionOption <= MaxResolution)
				AllResolutions.Add(ResolutionOption);

		// Add CurrentResolution.
		// Maybe it was overridden by console or command line or config file
		if (CurrentResolution.AspectRatio == CurrentAspectRatio)
			AllResolutions.Add(CurrentResolution);

		// Resolutions must be unique and sorted.
		ResolutionOptions = AllResolutions.Array();
		ResolutionOptions.Sort([](const FSoDisplayResolution& A, const FSoDisplayResolution& B)
		{
			return A < B;
		});
	}

	// Display the resolution options
	TArray<FText> ResolutionTexts;
	for (const FSoDisplayResolution& ResolutionOption : ResolutionOptions)
	{
		FFormatOrderedArguments Arguments;
		Arguments.Add(FText::AsNumber(ResolutionOption.Resolution.X));
		Arguments.Add(FText::AsNumber(ResolutionOption.Resolution.Y));
		ResolutionTexts.Add(FText::Format(FROM_STRING_TABLE_UI("settings_resolution_entry"), Arguments));
	}
	Resolution->CreateButtons(ResolutionTexts);

	if (CurrentAspectRatio != CurrentResolution.AspectRatio)
	{
		// Different aspect ratio
		// Highest possible
		Resolution->SetSelectedIndex(ResolutionOptions.Num() - 1);
	}
	else
	{
		// Same aspect ratio
		Resolution->SetSelectedIndex(ResolutionOptions.Find(CurrentResolution));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::UpdateFrameLimitFromUserSettings()
{
	if (FrameLimit == nullptr)
		return;

	// Apply preset options
	TSet<int32> AllFrameLimits(FrameLimit_PresetOptions);

	// Add CurrentFrameLimit
	const int32 CurrentFrameLimit = FloatToFrameLimitOption(UserSettings->GetFrameRateLimit());
	AllFrameLimits.Add(CurrentFrameLimit);

	// Add refresh rates that are greater than 60 Hz
	TSet<int32> RefreshRates;
	USoPlatformHelper::GetDisplaysRefreshRates(RefreshRates);
	for (const int32 Refresh : RefreshRates)
		if (Refresh > 60.f)
			AllFrameLimits.Add(Refresh);

	// Do no include unlimited in sorting as it will be the first we want it to be last
	AllFrameLimits.Remove(FrameLimitUnlimitedOption);

	// Frame limit options must be unique and sorted.
	FrameLimitOptions = AllFrameLimits.Array();
	FrameLimitOptions.Sort([](const int32 A, const int32 B)
	{
		return A < B;
	});

	// Unlimited should be last
	FrameLimitOptions.Add(FrameLimitUnlimitedOption);

	// Display the frame limit options
	TArray<FText> FrameLimitTexts;
	for (const int32 Option : FrameLimitOptions)
	{
		if (Option == INDEX_NONE)
			FrameLimitTexts.Add(FROM_STRING_TABLE_UI("settings_fps_unlimited"));
		else
			FrameLimitTexts.Add(FText::AsNumber(Option));
	}
	FrameLimit->CreateButtons(FrameLimitTexts);

	// Select the current one
	FrameLimit->SetSelectedIndex(FrameLimitOptions.Find(CurrentFrameLimit));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::UpdateResolutionScaleFromUserSettings()
{
	if (!ResolutionScale)
		return;

	float CurrentPercentNormalized; // from 0 to 1
	float CurrentPercentValue;
	float MinPercentValue;
	float MaxPercentValue;
	UserSettings->GetScreenPercentageInformation(CurrentPercentNormalized, CurrentPercentValue, MinPercentValue, MaxPercentValue);

	ResolutionScale->SetRangeTextDisplayValues(MinPercentValue, MaxPercentValue);
	ResolutionScale->SetValueNormalized(CurrentPercentNormalized);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::ReloadDefaultSettings()
{
	bResolutionScaleChanged = false;
	DefaultSettings.Empty();
	for (int32 LineIndex = 0; LineIndex < LineOptions.Num(); LineIndex++)
	{
		if (USoUIButtonArray* ButtonArray = GetButtonArrayAtLineIndex(LineIndex))
		{
			DefaultSettings.Add(ButtonArray->GetSelectedIndex());
		}
		else if (USoUICheckbox* CheckBox = GetCheckboxAtLineIndex(LineIndex))
		{
			DefaultSettings.Add(CheckBox->IsChecked() ? 1 : 0);
		}
		else if (LineOptions[LineIndex] == ESoUISettingsDisplayLine::ResolutionScale)
		{
			DefaultSettings.Add(FMath::RoundToInt(ResolutionScale->GetValueNormalized() * 100));
		}
		else
		{
			// Default
			DefaultSettings.Add(INDEX_NONE);
		}
	}
	check(LineOptions.Num() == DefaultSettings.Num());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsDisplay::DidDefaultSettingsChange() const
{
	// Check if anything is different
	for (int32 LineIndex = 0; LineIndex < LineOptions.Num() && LineIndex < DefaultSettings.Num(); LineIndex++)
		if (DidDefaultSettingChangeForLine(LineIndex))
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsDisplay::DidDefaultSettingChangeForLine(int32 LineIndex) const
{
	if (!LineOptions.IsValidIndex(LineIndex))
		return false;

	const int32 CurrentValue = DefaultSettings[LineIndex];
	if (USoUIButtonArray* ButtonArray = GetButtonArrayAtLineIndex(LineIndex))
	{
		if (CurrentValue != ButtonArray->GetSelectedIndex())
			return true;
	}
	else if (USoUICheckbox* CheckBox = GetCheckboxAtLineIndex(LineIndex))
	{
		if (CurrentValue != (CheckBox->IsChecked() ? 1 : 0))
			return true;
	}
	else if (USoUISlider* Slider = GetSliderAtLineIndex(LineIndex))
	{
		if (CurrentValue != FMath::RoundToInt(Slider->GetValueNormalized() * 100))
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUISettingsDisplay::GetDefaultSettingForLine(int32 LineIndex) const
{
	if (!LineOptions.IsValidIndex(LineIndex))
		return INDEX_NONE;

	return DefaultSettings[LineIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnDisplayModeOrResolutionChanged()
{
	auto SetEnabledDisplayLines = [this](const bool bEnable)
	{
		// NOTE order of LineOptions and Lines Matches
		for (int32 OptionIndex = 0; OptionIndex < LineOptions.Num(); OptionIndex++)
		{
			//const ESoUISettingsDisplayLine Option = LineOptions[OptionIndex];
			SetIsLineEnabled(OptionIndex, bEnable);
		}
	};

	SetEnabledDisplayLines(!IsConfirmMode());
	EmptyAndRefreshButtonImagesTooltips();

	// Call BP
	ReceiveOnDisplayModeOrResolutionChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnResolutionScaleValueChanged(float NewValueNormalized)
{
	if (!bOpened)
		return;

	bResolutionScaleChanged = true;
	RefreshButtonImagesTooltips();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnScrollingArrayChildChanged(int32 PreviousChild, int32 NewChild)
{
	if (!bOpened)
		return;

	UpdateLinesVisibility();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsDisplay::IsConfirmMode() const
{
	return ::IsValid(ConfirmDisplaySettings) && ConfirmDisplaySettings->CanHandleUICommand();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettingsDisplay::OpenConfirmPanel()
{
	if (!ConfirmDisplaySettings)
		return false;

	ISoUIEventHandler::Execute_Open(ConfirmDisplaySettings, true);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettingsDisplay::OnConfirmPanelOpenChanged(bool bPanelOpened)
{
	OnDisplayModeOrResolutionChanged();

	// Handled
	if (bPanelOpened)
	{
		ButtonImagesTooltipsArray->Hide();
	}
	else
	{
		ButtonImagesTooltipsArray->Show();
		ResetSelectedOptionsToUserSettings();
	}
}
