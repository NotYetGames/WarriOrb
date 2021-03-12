// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoGameSettings.h"

#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/GameModeBase.h"
#include "Widgets/SWindow.h"
#include "Engine/Engine.h"
#include "Misc/ConfigCacheIni.h"

#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Character/SoPlayerController.h"
#include "UI/SoUIHelper.h"
#include "Basic/SoGameMode.h"
#include "Input/SoInputHelper.h"
#include "Input/SoInputNames.h"
#include "Input/SoInputSettings.h"
#include "SoAudioSettingsTypes.h"
#include "Basic/SoAudioManager.h"
#include "Basic/SoGameSingleton.h"

#include "NYLoadingScreenSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoGameSettings, All, All);

// Manually set Value was UE_GAMEUSERSETTINGS_VERSION = 5. Double it just in case of future updates
// NOTE: if the Config/DefaultGameUserSettings.ini contains the "Version" value you also must update it there
static constexpr int32 SO_GAMEUSERSETTINGS_VERSION = 13;

// Stupid compiler, bad compiler :|
constexpr float USoGameSettings::MaxScreenPercentage;
constexpr float USoGameSettings::MinScreenPercentage;
constexpr float USoGameSettings::MinBrightness;
constexpr float USoGameSettings::MaxBrightness;
constexpr float USoGameSettings::DefaultBrightness;
constexpr float USoGameSettings::MinVibrationScale;
constexpr float USoGameSettings::MaxVibrationScale;
constexpr float USoGameSettings::DefaultVibrationScale;
constexpr float USoGameSettings::DefaultGameSpeed;

const TArray<FInputActionKeyMapping> USoGameSettings::EmptyInputActionKeysArray;
const TArray<FInputAxisKeyMapping> USoGameSettings::EmptyInputAxisKeysArray;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameSettings::USoGameSettings()
{
	ResetVariablesToDefault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameSettings::~USoGameSettings()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetToDefaults()
{
	SetDisplaySettingsToDefault();
	SetAudioSettingsToDefault();
	SetGameSettingsToDefault();
	SetInputSettingsToDefault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ResetVariablesToDefault()
{
	ResetDisplaySettingsVariables();
	ResetAudioSettingsVariables();
	ResetGameSettingsVariables();
	ResetInputSettingsVariables();
	WorldContextObject = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetCacheSaveRequestsMode(bool bActive)
{
	bCacheSaveRequests = bActive;
	if (!bActive && bAnySaveRequestCached)
		SaveSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ResetToCurrentSettings()
{
	ResetToCurrentDisplaySettings();
	ResetToCurrentAudioSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	ValidateSettings();
	ApplyDisplaySettings(bCheckForCommandLineOverrides, false);
	ApplyAudioSettings(bCheckForCommandLineOverrides, false);
	ApplyGameSettings(bCheckForCommandLineOverrides, false);
	ApplyInputSettings(false);
	SaveSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
	RefreshInputSettingsMappings();

	const UNYLoadingScreenSettings* LoadingScreenSettings = GetDefault<UNYLoadingScreenSettings>();

	// Custom event period
	if (LoadingScreenSettings->IsHalloweenEventPeriod())
	{
		// Set the new char skin for this event, and save the previous one
		if (!LoadingScreenSettings->IsDateTimeInHalloweenEventPeriod(ModifiedDateTimePreviousCharacterSkinType))
		{
			ModifiedDateTimePreviousCharacterSkinType = FDateTime::Now();
			PreviousCharacterSkinType = CharacterSkinType;
			CharacterSkinType = ESoCharacterSkinType::Pumpkin;
		}
	}

	// Restore the previous Char skin
	else if (LoadingScreenSettings->IsDateTimeInHalloweenEventPeriod(ModifiedDateTimePreviousCharacterSkinType))
	{
		// Restore to the previous one
		ModifiedDateTimePreviousCharacterSkinType = FDateTime::MinValue();
		CharacterSkinType = PreviousCharacterSkinType;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SaveSettings()
{
	if (bBlockConfigSave)
		return;

	if (bCacheSaveRequests)
	{
		bAnySaveRequestCached = true;
		return;
	}

	// Always be in the same order
	ModifiedInputActionMappings.Sort();
	ModifiedInputAxisMappings.Sort();
	Super::SaveSettings();
	bAnySaveRequestCached = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ValidateSettings()
{
	// Reset game speed to normal
	if (GameSpeedVersion < FSoGameSpeedVersion::Initial)
	{
		GameSpeed = 1.f;
		SaveSettings();
	}

	ValidateDisplaySettings();
	ValidateGameSettings();
	ValidateAudioSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Game settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameSettings* USoGameSettings::GetInstance()
{
	return GEngine ? CastChecked<USoGameSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::BeforeGameModeStartPlay()
{
	// Reset for PIE mode
	SetToDefaults();

	LoadSettings(true);

	// First time starting?
#if WITH_EDITOR
	const bool bLocalFirstTimeGameStarted = false;
#else
	const bool bLocalFirstTimeGameStarted = IsFirstTimeGameStarted();
#endif // WITH_EDITOR

	// First time running the game
	if (bLocalFirstTimeGameStarted)
	{
		// Set resolution to primary display resolution
		if (IsWindowModeFullScreen())
		{
			SetScreenResolutionToPrimaryDisplayResolution();
			ApplySettings(true);
		}

		// Run benchmarks only if the user didn't stop us from doing it
		const bool bNoHardwareBenchmark = FParse::Param(FCommandLine::Get(), TEXT("NoHardwareBenchmark"));
		if (!bNoHardwareBenchmark)
		{
			bShouldDoHardwareBenchmark = true;
		}
	}

	// Set keyboard preset the first time
	if (!bKeyboardPresetAlreadySelected)
	{
		if (WARRIORB_USE_ARROW_PRESET_AS_DEFAULT)
		{
			SetInputToPresetKeyboardArrows();
		}
		else
		{
			SetKeyboardInputBindingsToDefault();
		}

		bKeyboardPresetAlreadySelected = true;
	}

	// We set the game as started from now on
	bIsFirstTimeGameStarted = false;

	// Force fullscreen
	if (USoPlatformHelper::IsSteamBigPicture())
	{
		UE_LOG(LogSoGameSettings, Log, TEXT("BeforeGameModeStartPlay: User is in Steam Big Picture, Forcing fullscreen!"));
		SetFullscreenMode(EWindowMode::WindowedFullscreen);
		ConfirmVideoMode();
	}

	ApplySettings(true);
	PrintToLog();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::PrintToLog() const
{
	FDisplayMetrics DisplayMetrics;
	if (USoPlatformHelper::GetDisplaysMetrics(DisplayMetrics))
		DisplayMetrics.PrintToLog();

	UE_LOG(LogSoGameSettings, Log, TEXT("DefaultGameUserSettingsIni: %s"), *GetDefaultConfigFilename());
	UE_LOG(LogSoGameSettings, Log, TEXT("GameUserSettingsIni: %s"), *GGameUserSettingsIni);
	UE_LOG(LogSoGameSettings, Log, LINE_TERMINATOR TEXT("%s") LINE_TERMINATOR, *ToString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ApplyGameSettings(bool bCheckForCommandLineOverrides, bool bSaveSettings)
{
	if (bCheckForCommandLineOverrides)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CollectGameAnalytics")))
		{
			bCollectGameAnalytics = true;
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("NoCollectGameAnalytics")))
		{
			bCollectGameAnalytics = false;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("WaitForAnalyticsToSend")))
		{
			bWaitForAnalyticsToSend = true;
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("NoWaitForAnalyticsToSend")))
		{
			bWaitForAnalyticsToSend = false;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("PauseGameWhenUnfocused")))
		{
			bPauseGameWhenUnfocused = true;
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("NoPauseGameWhenUnfocused")))
		{
			bPauseGameWhenUnfocused = false;
		}
	}

	if (GEngine && GEngine->IsInitialized())
	{
		// Game Speed
		USoPlatformHelper::SetGlobalTimeDilation(WorldContextObject, GameSpeed);

		// Brightness
		USoPlatformHelper::SetDisplayGamma(Brightness);

		// Vibration
		USoPlatformHelper::SetForceFeedbackScale(WorldContextObject, VibrationScale);
	}

	if (bSaveSettings)
		SaveSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::OnLanguageSelected()
{
	bLanguageAlreadySelected = true;
	SaveSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetGameSettingsToDefault()
{
	ResetGameSettingsVariables();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ResetGameSettingsVariables()
{
	// language
	bLanguageAlreadySelected = false;

	// Video Demo
	bVideoDemoDisableMainMenu = false;
	bVideoDemoStartVideoOnIdle = false;
	VideoDemoIdleTimeSecondsStartVideo = 30.f;

	bDisplayFPS = false;
	bDisplayTime = false;
	bDisplayDamageTexts = true;
	bDisplayEnemyHealthBar = true;
	bDisplayFloatingVOLines = true;
	bDisplayStat = false;
	bPauseGameWhenUnfocused = true;
	IdleTimeSecondsBeforeGamePause = 120.f;
	bPauseGameOnIdle = true;
	bIsFirstTimeGameStarted = true;
	BounceModeType = ESoBounceModeType::Always;
	CharacterSkinType = ESoCharacterSkinType::Classic;
	ModifiedDateTimeCharacterSkinType = FDateTime::MinValue();
	ModifiedDateTimePreviousCharacterSkinType = FDateTime::MinValue();

	// Analytics
	bCollectGameAnalytics = false;
	bWaitForAnalyticsToSend = true;
	UnixTimeModifiedCollectGameAnalytics = INDEX_NONE;
	AnalyticsWaitSeconds = 0.5;
	AnalyticsProcessEventsSeconds = 2.0;

	// Saves
	bAutoSaveAndLoad = true;
	bAutoBackupDeletedSaves = true;
	bAutoBackupBeforeSave = true;

	// Configurable custom maps
	bCustomMapsEnabled = true;

	GameSpeed = DefaultGameSpeed;
	Brightness = DefaultBrightness;
	VibrationScale = DefaultVibrationScale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ValidateGameSettings()
{
	if (UnixTimeModifiedCollectGameAnalytics < INDEX_NONE)
		UnixTimeModifiedCollectGameAnalytics = 0;

	if (AnalyticsWaitSeconds < KINDA_SMALL_NUMBER)
		AnalyticsWaitSeconds = 0.5;

	if (AnalyticsProcessEventsSeconds < KINDA_SMALL_NUMBER)
		AnalyticsProcessEventsSeconds = 2.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetTemporaryGameSpeed(float NewGameSpeed)
{
	USoPlatformHelper::SetGlobalTimeDilation(WorldContextObject, FMath::Abs(NewGameSpeed));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::IsTemporaryGameSpeedChanged() const
{
	return !FMath::IsNearlyEqual(GameSpeed, GetTemporaryGameSpeed());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoGameSettings::GetTemporaryGameSpeed() const
{
	return USoPlatformHelper::GetGlobalTimeDilation(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Display settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::MoveToMonitor(int32 MonitorNumber, FMonitorInfo& OutMonitor)
{
	// On Consoles this is useless
#if !PLATFORM_DESKTOP
	return false;
#endif

	// References (they seem wrong as they do not take in account different screen sizes or different platforms):
	// - https://answers.unrealengine.com/questions/294650/is-it-possible-to-choose-the-default-screen-in-a-m.html
	// - http://www.froyok.fr/blog/2018-01-ue4-specify-default-monitor-at-launch

	if (MonitorNumber < 0)
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("MoveToMonitor: MonitorNumber < 0"));
		return false;
	}

	if (GEngine == nullptr)
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("MoveToMonitor: GEngine == nullptr"));
		return false;
	}
	if (GEngine->GameViewport == nullptr)
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("MoveToMonitor: GEngine->GameViewport == nullptr"));
		return false;
	}
	if (!GEngine->GameViewport->GetWindow().IsValid())
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("MoveToMonitor: !GEngine->GameViewport->GetWindow().IsValid()"));
		return false;
	}

	FDisplayMetrics DisplayMetrics;
	if (!USoPlatformHelper::GetDisplaysMetrics(DisplayMetrics))
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("MoveToMonitor: Can't get FDisplayMetrics"));
		return false;
	}

	int32 MonitorIndex = INDEX_NONE;
	if (MonitorNumber == 0)
	{
		// Start monitor on the main screen
		for (int32 Index = 0; Index < DisplayMetrics.MonitorInfo.Num(); Index++)
		{
			if (DisplayMetrics.MonitorInfo[Index].bIsPrimary)
			{
				MonitorIndex = Index;
				break;
			}
		}
	}
	else
	{
		// Normalize
		MonitorIndex = MonitorNumber - 1;
	}

	if (!DisplayMetrics.MonitorInfo.IsValidIndex(MonitorIndex))
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("MoveToMonitor: MonitorIndex is not a valid index in DisplayMetrics of Num = %d"), DisplayMetrics.MonitorInfo.Num());
		return false;
	}

	// NOTE: WindowPosX and WindowPosY are used only if Game is ran with -SAVEWINPOS=1
	// Offset to the new monitor
	// NOTE: on windows this seems to be the right order but on linux it is not :|
	//WindowPosX = (MonitorIndex * DisplayMetrics.PrimaryDisplayWidth) - CurrentMonitorWidth;
	//WindowPosX = DisplayMetrics.PrimaryDisplayWorkAreaRect.Left;
	//WindowPosY = DisplayMetrics.PrimaryDisplayWorkAreaRect.Top;
	//const int32 CurrentMonitorWidth = DisplayMetrics.MonitorInfo[MonitorIndex].NativeWidth;

	// Figure out where to move, previous method by offsetting was inaccurate
	const FMonitorInfo Monitor = DisplayMetrics.MonitorInfo[MonitorIndex];
	WindowPosX = Monitor.WorkArea.Left;
	WindowPosY = Monitor.WorkArea.Top;

	// Screen we are moving to has a smaller resolution than our current one
	if (static_cast<int32>(ResolutionSizeX) > Monitor.NativeWidth || static_cast<int32>(ResolutionSizeY) > Monitor.NativeHeight)
	{
		UE_LOG(
			LogSoGameSettings,
			Log,
			TEXT("MoveToMonitor: Changing Resolution of game because current resolution is larger than the monitor resolution. CurrentResolution = (%d, %d), MonitorResolution = (%d, %d)"),
			ResolutionSizeX, ResolutionSizeY, Monitor.NativeWidth,Monitor.NativeHeight
		);
		SetScreenResolution(FIntPoint(Monitor.NativeWidth, Monitor.NativeHeight));
		ConfirmVideoMode();
		ApplyDisplaySettings(false);
	}

	// Move
	TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
	const FVector2D Position(static_cast<float>(WindowPosX), static_cast<float>(WindowPosY));

	// Huge hack, because linux does not seem to work properly if it is not windowed mode
#if PLATFORM_LINUX
	const EWindowMode::Type CurrentWindowMode = Window->GetWindowMode();
	Window->SetWindowMode(EWindowMode::Windowed);
#endif

	// Move
	Window->MoveWindowTo(Position);

#if PLATFORM_LINUX
	// Set back to original
	Window->SetWindowMode(CurrentWindowMode);
#endif

	const FVector2D InitialDesiredScreenPosition = Window->GetInitialDesiredPositionInScreen();
	UE_LOG(
		LogSoGameSettings,
		Log,
		TEXT("MoveToMonitor: MonitorIndex = %d, DisplayMetrics.PrimaryDisplayWidth = %d, WindowPos = (%d, %d), InitialDesiredScreenPosition = %s"),
		MonitorIndex, DisplayMetrics.PrimaryDisplayWidth, WindowPosX, WindowPosY, *InitialDesiredScreenPosition.ToString()
	);

	OutMonitor = DisplayMetrics.MonitorInfo[MonitorIndex];
	return true;
}

void USoGameSettings::ResetToCurrentDisplaySettings()
{
	// Super class is only Display settings related
	Super::ResetToCurrentSettings();
	if (GEngine)
	{
		// From `t.MaxFPS` console variable.
		FrameRateLimit = GEngine->GetMaxFPS();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ApplyDisplaySettings(bool bCheckForCommandLineOverrides, bool bSaveSettings)
{
	EnsureCustomQualityLimits();
	ApplyResolutionSettings(bCheckForCommandLineOverrides);
	ApplyNonResolutionSettings();

	if (bSaveSettings)
		SaveSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetDisplaySettingsToDefault()
{
	// Super class is only Display settings related
	Super::SetToDefaults();

	ResetDisplaySettingsVariables();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ResetDisplaySettingsVariables()
{
	// On Main Display
	WindowPosX = 0;
	WindowPosX = 0;
	bVolumetricFog = false;
	bForceWeakHardwareOptimizations = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ValidateDisplaySettings()
{
	// Super class is only Display settings related
	Super::ValidateSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetScreenResolutionToPrimaryDisplayResolution()
{
	const FIntPoint& DesktopResolution = GetDesktopResolution();
	UE_LOG(LogSoGameSettings, Log, TEXT("Setting ScreenResolution to DesktopResolution = %d X %d"), DesktopResolution.X, DesktopResolution.Y);
	SetScreenResolution(DesktopResolution);
	ConfirmVideoMode();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RunAndApplyHardwareBenchmarkIfRequested()
{
	if (bShouldDoHardwareBenchmark)
	{
		RunAndApplyHardwareBenchmark();
		bShouldDoHardwareBenchmark = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RunAndApplyHardwareBenchmark()
{
	// Skip hardware benchmark on non desktop platforms because it kills their hardware
#if !PLATFORM_DESKTOP
	UE_LOG(LogSoGameSettings, Log, TEXT("RunAndApplyHardwareBenchmark disabled because we are not in a desktop platform"));
	return;
#endif // !PLATFORM_DESKTOP

	UE_LOG(LogSoGameSettings, Log, TEXT("Autodetecting quality settings"));

	static constexpr int32 WorkScale = 10;
	static constexpr float CPUMultiplier = 1.f;
	static constexpr float GPUMultiplier = 1.f;

	// From RunHardwareBenchmark
	ScalabilityQuality = Scalability::BenchmarkQualityLevels(WorkScale, CPUMultiplier, GPUMultiplier);

	// 100 is average
	// < 100 - slower
	// > 100 - faster
	LastCPUBenchmarkResult = ScalabilityQuality.CPUBenchmarkResults;
	LastGPUBenchmarkResult = ScalabilityQuality.GPUBenchmarkResults;

	LastCPUBenchmarkSteps = ScalabilityQuality.CPUBenchmarkSteps;
	LastGPUBenchmarkSteps = ScalabilityQuality.GPUBenchmarkSteps;
	LastGPUBenchmarkMultiplier = GPUMultiplier;

	// Only if GPU result is over average (-20%)
	// This matches the ScalabilitySettings perf index threshold for the shadow quality
	// because low and medium shadow quality disables volumetric fog (r.VolumetricFog=0)
	// In the scalability settings the thershold for enabling volumetric fog is 42
	bVolumetricFog = FMath::Min(LastCPUBenchmarkResult, LastGPUBenchmarkResult) > 60.f;

	// Check our limits
	EnsureCustomQualityLimits();

	// Save
	ApplyHardwareBenchmarkResults();
	ApplySettings(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::GetScreenPercentageInformation(float& CurrentPercentNormalized, float& CurrentPercentValue,
	float& MinPercentValue, float& MaxPercentValue) const
{
	CurrentPercentValue = GetScreenPercentage();
	// Minimum the same as the resolution quality as it is automatically calculated and it is something sane.
	MinPercentValue = MinScreenPercentage;
	MaxPercentValue = MaxScreenPercentage;
	CurrentPercentNormalized = (CurrentPercentValue - MinPercentValue) / (MaxPercentValue - MinPercentValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoGameSettings::GetScreenPercentage() const
{
	static const auto ScreenPercentage = IConsoleManager::Get().FindTConsoleVariableDataFloat(ScreenPercentageVariable);
	return ScreenPercentage->GetValueOnGameThread();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetScreenPercentageValue(float NewScreenPercentageValue)
{
	NewScreenPercentageValue = FMath::Clamp(NewScreenPercentageValue, MinScreenPercentage, MaxScreenPercentage);

	// Update ScreenPercentage (r.ScreenPercentage)
	static IConsoleVariable* ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(ScreenPercentageVariable);
	ScreenPercentage->Set(NewScreenPercentageValue, ECVF_SetByGameSetting);

	// Update ResolutionQuality (sg.ResolutionQuality)
	SetResolutionScaleValueEx(NewScreenPercentageValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetScreenPercentageNormalized(float NewScaleNormalized)
{
	SetScreenPercentageValue(FMath::Lerp(MinScreenPercentage, MaxScreenPercentage, NewScaleNormalized));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::IsVersionValid()
{
	return Version == SO_GAMEUSERSETTINGS_VERSION;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::UpdateVersion()
{
	Version = SO_GAMEUSERSETTINGS_VERSION;
	GameSpeedVersion = FSoGameSpeedVersion::LatestVersion;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ResetToCurrentAudioSettings()
{
	VolumeMaster = USoAudioManager::GetVolumeMaster();
	VolumeMusic = USoAudioManager::GetVolumeMusic();
	VolumeSFX = USoAudioManager::GetVolumeSFX();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ApplyAudioSettings(bool bCheckForCommandLineOverrides, bool bSaveSettings)
{
	// if (!GEngine->IsInitialized())
	// {
		// UE_LOG(LogSoGameSettings, Warning, TEXT("USoGameSettings:ApplyAudioSettings Engine is not initialized"));
		// return;
	// }

	if (bMuteAudio)
		USoAudioManager::MuteAudio();
	else
		USoAudioManager::UnMuteAudio();

	USoAudioManager::SetVolumeMaster(VolumeMaster);
	USoAudioManager::SetVolumeMusic(VolumeMusic);
	USoAudioManager::SetVolumeSFX(VolumeSFX);

	if (bSaveSettings)
		SaveSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::AreDefaultAudioSettingsUsed() const
{
	// Compare against default
	const FSoAudioSettings& Default = GetDefaultAudioSettings();
	const FSoAudioSettings Current{bMuteAudio, bMuteAudioWhenUnfocused, bMuteVoiceGibberish, bMuteDialogueVoiceGibberish, VolumeMaster, VolumeMusic, VolumeSFX};

	return Default == Current;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetAudioSettingsToDefault()
{
	ResetAudioSettingsVariables();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ResetAudioSettingsVariables()
{
	const FSoAudioSettings& Default = GetDefaultAudioSettings();
	bMuteAudio = Default.bMuteAudio;
	bMuteAudioWhenUnfocused = Default.bMuteAudioWhenUnfocused;
	bMuteVoiceGibberish = Default.bMuteVoiceGibberish;
	bMuteDialogueVoiceGibberish = Default.bMuteDialogueVoiceGibberish;
	VolumeMaster = Default.VolumeMaster;
	VolumeMusic = Default.VolumeMusic;
	VolumeSFX = Default.VolumeSFX;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ValidateAudioSettings()
{
	VolumeMaster = FMath::Clamp(VolumeMaster, 0.f, 1.f);
	VolumeMusic = FMath::Clamp(VolumeMusic, 0.f, 1.f);
	VolumeSFX = FMath::Clamp(VolumeSFX, 0.f, 1.f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ApplyInputSettings(bool bSaveSettings, bool bForce)
{
	RefreshInputSettingsMappings();
	RebuildInputKeymapsForAllPlayers();

	if (ASoPlayerController* Controller = ASoPlayerController::GetInstance(WorldContextObject))
		Controller->NotifyUserSettingsGamepadUITypeChanged(bForce);

	if (bSaveSettings)
		SaveSettings();

	USoUIHelper::ForceReloadAllUICommandImages();

	OnInputSettingsApplied.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetInputSettingsToDefault()
{
	ResetInputSettingsVariables();
	SetInputBindingsToDefault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::ResetInputSettingsVariables()
{
	bAutoDetectGamepadUIType = false;
	bKeyboardPresetAlreadySelected = false;
	bUseKeyboardArrowPresetAsDefault = false;
	GamepadUIType = ESoGamepadLayoutType::Xbox;
	ModifiedInputActionMappings.Empty();
	ModifiedInputAxisMappings.Empty();
	InputActionMappings.Empty();
	InputAxisMappings.Empty();
	GamepadKeyToGameActionNameMap.Empty();
	GamepadKeyToUIActionNameMap.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetInputBindingsToDefault()
{
	ModifiedInputActionMappings.Empty();
	ModifiedInputAxisMappings.Empty();
	RefreshInputSettingsMappings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetKeyboardInputBindingsToDefault()
{
	bUseKeyboardArrowPresetAsDefault = false;

	for (int32 Index = ModifiedInputActionMappings.Num() - 1; Index >= 0; Index--)
	{
		const FSoInputActionKeyMapping& Mapping = ModifiedInputActionMappings[Index];
		if (Mapping.IsKeyboard())
			ModifiedInputActionMappings.RemoveAt(Index);
	}
	for (int32 Index = ModifiedInputAxisMappings.Num() - 1; Index >= 0; Index--)
	{
		const FSoInputAxisKeyMapping& Mapping = ModifiedInputAxisMappings[Index];
		if (Mapping.IsKeyboard())
			ModifiedInputAxisMappings.RemoveAt(Index);
	}

	RefreshInputSettingsMappings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetGamepadInputBindingsToDefault()
{
	for (int32 Index = ModifiedInputActionMappings.Num() - 1; Index >= 0; Index--)
	{
		const FSoInputActionKeyMapping& Mapping = ModifiedInputActionMappings[Index];
		if (Mapping.IsGamepad())
			ModifiedInputActionMappings.RemoveAt(Index);
	}
	for (int32 Index = ModifiedInputAxisMappings.Num() - 1; Index >= 0; Index--)
	{
		const FSoInputAxisKeyMapping& Mapping = ModifiedInputAxisMappings[Index];
		if (Mapping.IsGamepad())
			ModifiedInputAxisMappings.RemoveAt(Index);
	}

	RefreshInputSettingsMappings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::TryAutoSetGamepadLayout()
{
	if (IsAutoDetectGamepadLayout())
	{
		return AutoSetGamepadLayoutFromConnectedGamepad();
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::AutoSetGamepadLayoutFromConnectedGamepad()
{
	if (!USoPlatformHelper::IsAnyGamepadConnected())
		return false;

	ASoPlayerController* Controller = ASoPlayerController::GetInstance(WorldContextObject);
	if (!Controller)
		return false;

	// Default is first
	FString GamepadName = USoPlatformHelper::GetFirstGamepadName();

	// Try from input joystick
	const int32 LastJoystickIndexUsed = Controller->GetLastJoystickIndexUsed();
	if (USoPlatformHelper::IsJoystickIndexAGamepad(LastJoystickIndexUsed))
	{
		GamepadName = USoPlatformHelper::GetGamepadName(LastJoystickIndexUsed);
	}

	// Prefer the first valid
	const int32 NumGamepads = USoPlatformHelper::NumGamepads();
	UE_LOG(
		LogSoGameSettings,
		Log,
		TEXT("AutosetGamepadUIFromConnectedGamepad: NumGamepads = %d, FirstGamepadName = %s"),
		NumGamepads, *GamepadName
	);

	if (!GamepadName.IsEmpty())
	{
		const ESoGamepadLayoutType PreviousGamepadLayout = GetGamepadLayoutType();
		const ESoGamepadLayoutType NewGamepadLayout = USoPlatformHelper::GetGamepadLayoutTypeFromName(GamepadName);
		if (PreviousGamepadLayout != NewGamepadLayout)
		{
			SetGamepadLayoutType(NewGamepadLayout);
			UE_LOG(
				LogSoGameSettings,
				Log,
				TEXT("AutosetGamepadUIFromConnectedGamepad: Changing gamepad layout to %s because of GamepadName = %s"),
				*USoPlatformHelper::GamepadLayoutToString(NewGamepadLayout), *GamepadName
			);
			ApplyInputSettings();
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoGamepadLayoutType USoGameSettings::GetGamepadLayoutType() const
{
	if (USoPlatformHelper::HasHardcodedGamepad())
	{
		return USoInputHelper::ConvertInputDeviceTypeToInputGamepadUIType(USoPlatformHelper::GetHardcodedGamepadType());
	}

	return GamepadUIType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetAutoDetectGamepadLayout(bool InValue)
{
	bAutoDetectGamepadUIType = InValue;
	if (IsAutoDetectGamepadLayout())
	{
		AutoSetGamepadLayoutFromConnectedGamepad();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RefreshInputSettingsMappings()
{
	InputActionMappings.Empty();
	InputAxisMappings.Empty();
	GamepadKeyToGameActionNameMap.Empty();
	GamepadKeyToUIActionNameMap.Empty();
	const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (InputSettings == nullptr)
		return;

	// Sanity check
	CheckAndRepairModifiedInputSettings();

	// NOTE: InputSettings->*Mappings are the default ones

	// Action mappings
	InputActionMappings = GenerateInputActionMappings();

	// Axis mappings
	for (const FInputAxisKeyMapping& DefaultAxis : InputSettings->AxisMappings)
	{
		TArray<FSoInputAxisKeyMapping> ModifiedMappings = GetModifiedInputAxisMappingsForDefault(DefaultAxis);
		if (ModifiedMappings.Num() > 0)
		{
			// Add overridden (modified) axis
			for (const FSoInputAxisKeyMapping& ModifiedAxis : ModifiedMappings)
				InputAxisMappings.FindOrAdd(ModifiedAxis.Modified.AxisName).Add(ModifiedAxis.Modified);
		}
		else
		{
			// Normal not overridden, Not Yet
			InputAxisMappings.FindOrAdd(DefaultAxis.AxisName).Add(DefaultAxis);
		}
	}

	// Build GamepadKeyToActionNameMap
	for (const auto& Elem : InputActionMappings)
	{
		const FName ActionName = Elem.Key;

		// Normal actions
		if (FSoInputActionName::IsGameActionName(ActionName))
		{
			for (const FInputActionKeyMapping& KeyMapping : Elem.Value)
			{
				// Must be gamepad button
				const FName KeyName = KeyMapping.Key.GetFName();
				if (!USoInputHelper::IsGamepadKey(KeyMapping.Key) || !FSoInputKey::IsGamepadButton(KeyName))
					continue;

				GamepadKeyToGameActionNameMap.FindOrAdd(KeyName).Add(ActionName);
			}
		}

		// UI actions
		if (FSoInputActionName::IsUIActionName(ActionName))
		{
			for (const FInputActionKeyMapping& KeyMapping : Elem.Value)
			{
				// Must be gamepad button
				const FName KeyName = KeyMapping.Key.GetFName();
				if (!USoInputHelper::IsGamepadKey(KeyMapping.Key) || !FSoInputKey::IsGamepadButton(KeyName))
					continue;

				GamepadKeyToUIActionNameMap.FindOrAdd(KeyName).Add(ActionName);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TMap<FName, TArray<FInputActionKeyMapping>> USoGameSettings::GenerateInputActionMappings() const
{
	const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	verify(InputSettings);

	TMap<FName, TArray<FInputActionKeyMapping>> LocalInputActionMappings;
	for (const FInputActionKeyMapping& DefaultAction : InputSettings->ActionMappings)
	{
		// NOTE: InputSettings->ActionMappings are the default ones
		TArray<FSoInputActionKeyMapping> ModifiedMappings = GetModifiedInputActionMappingsForDefault(DefaultAction);
		if (ModifiedMappings.Num() > 0)
		{
			// Add overridden (modified) keys
			for (const FSoInputActionKeyMapping& ModifiedKey : ModifiedMappings)
				LocalInputActionMappings.FindOrAdd(ModifiedKey.Modified.ActionName).Add(ModifiedKey.Modified);
		}
		else
		{
			// Normal not overridden, Not Yet
			LocalInputActionMappings.FindOrAdd(DefaultAction.ActionName).Add(DefaultAction);
		}
	}

	return LocalInputActionMappings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::SetInputToPreset(const FSoInputPreset& Preset)
{
	if (Preset.KeyRebinds.Num() == 0)
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("SetInputToPreset: Got Empty preset"));
		return false;
	}

	check(Preset.IsKeyboard());
	bool bReturnStatus = true;
	for (const FSoInputPresetKeyRebind& PresetKey : Preset.KeyRebinds)
	{
		const FInputActionKeyMapping NewMapping = PresetKey.ToUnrealActionMapping();

		const TArray<FInputActionKeyMapping>& AllMappings = GetInputActionMappingsForActionName(NewMapping.ActionName);
		bReturnStatus &= AllMappings.Num() > 0;

		// Rebind KeyMapping to new Preset key
		for (const FInputActionKeyMapping& KeyMapping : AllMappings)
		{
			const bool bIsInvalidKey = !KeyMapping.Key.IsValid();
			if (bIsInvalidKey || USoInputHelper::IsKeyboardKey(KeyMapping.Key))
				bReturnStatus &= RebindInputKey(KeyMapping, NewMapping, false, false);
		}
	}

	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::SetInputToPresetKeyboardArrows()
{
	const USoInputSettings* CustomInputSettings = GetDefault<USoInputSettings>();
	const bool bStatus = SetInputToPreset(CustomInputSettings->GetKeyboardDefaultArrowsPreset());
	bUseKeyboardArrowPresetAsDefault = bStatus;

	return bStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::RebindInputKeyToInputChord(const FInputActionKeyMapping& OldKeyMapping, const FInputChord& NewInputChord,
	bool bCheckExistingBinding, bool bUnbindExisting)
{
	// NOTE: we ignore the meta modified (alt, ctrl, shift) + keys
	FInputActionKeyMapping NewKeyMapping = OldKeyMapping;
	NewKeyMapping.Key = NewInputChord.Key;
	return RebindInputKey(OldKeyMapping, NewKeyMapping, bCheckExistingBinding, bUnbindExisting);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::RebindInputKey(const FInputActionKeyMapping& OldKeyMapping, const FInputActionKeyMapping& NewKeyMapping,
	bool bCheckExistingBinding, bool bUnbindExisting)
{
	// Does not even exist
	if (!InputActionMappings.Contains(OldKeyMapping.ActionName))
	{
		UE_LOG(
			LogSoGameSettings,
			Error,
			TEXT("RebindInputKey: ActionName = %s Does not even exist in the InputActionMappings"),
			*OldKeyMapping.ActionName.ToString()
		);
		return false;
	}

	// Key already used?
	if (bCheckExistingBinding)
	{
		const FInputChord NewInputChord = USoInputHelper::ActionMappingToInputChord(NewKeyMapping);
		TArray<FInputActionKeyMapping> ByActionMappings;
		if (IsInputChordAlreadyUsed(NewKeyMapping.ActionName, NewInputChord, ESoInputActionCategoryType::GameOrUI, ByActionMappings))
		{
			if (bUnbindExisting)
			{
				// Unbind all
				UE_LOG(
					LogSoGameSettings,
					Warning,
					TEXT("RebindInputKey: Unbinding all existing Action for Chord = %s"),
					*USoInputHelper::ChordToString(NewInputChord)
				);
				UnbindInputActionMappings(ByActionMappings);
			}
			else
			{
				FInputActionKeyMapping SwappedKeyMapping = ByActionMappings[0];
				SwappedKeyMapping.Key = OldKeyMapping.Key;
				RebindInputKey(ByActionMappings[0], SwappedKeyMapping, false, false);

				// Is the input key already was already in the settings Well that was easy
				//UE_LOG(
				//	LogSoGameSettings,
				//	Warning,
				//	TEXT("RebindInputKey: NewInputChord (%s) is already used. Ignoring this. We assume that it was a success as the rebind is already in the settings."),
				//	*USoInputHelper::ChordToString(NewInputChord)
				//);
				//return true;
			}
		}
	}

	// Find the default one for the OldKeyMapping
	TArray<FInputActionKeyMapping> DefaultKeys = GetDefaultInputActionMappingsForModified(OldKeyMapping);
	if (DefaultKeys.Num() != 1)
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("RebindInputKey: DefaultKeys.Num (%d) != 1"), DefaultKeys.Num());
		return false;
	}

	// Replace default action mapping with new modified one
	FSoInputActionKeyMapping ActionMapping;
	ActionMapping.Default = DefaultKeys[0];
	ActionMapping.Modified = NewKeyMapping;

	// Replace axis mappings
	bool bReturnStatus = true;
	const TSet<FName>& AxisToModify = USoInputHelper::GetAxisNamesLinkedToActionName(ActionMapping.Default.ActionName);
	for (const FName AxisNameToModify : AxisToModify)
	{
		if (AxisNameToModify != NAME_None)
		{
			FSoInputAxisKeyMapping AxisMapping;
			bReturnStatus = bReturnStatus && GetModifiedInputAxisMappingForActionMapping(ActionMapping, AxisNameToModify, AxisMapping);
			bReturnStatus = bReturnStatus && AddModifiedInputAxisMapping(AxisMapping);
		}
	}

	bReturnStatus = bReturnStatus && AddModifiedInputActionMapping(ActionMapping);

	// Check Sync
	if (bReturnStatus)
		EnsureActionNamesAreInSyncIfModified(ActionMapping);

	// Check unbound keys
	if (bReturnStatus)
		EnsureUnbindingDoesNotBreakGame(ActionMapping);

	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::UnbindInputActionMapping(const FInputActionKeyMapping& OldKeyMapping)
{
	FInputActionKeyMapping EmptyBinding = OldKeyMapping;
	EmptyBinding.Key = FKey{};

	// Invalidate key
	return RebindInputKey(OldKeyMapping, EmptyBinding, false, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::UnbindKeyboardInputActionMapping(const FInputActionKeyMapping& OldKeyMapping)
{
	// Can't unbind not a keyboard key
	if (!USoInputHelper::IsKeyboardKey(OldKeyMapping.Key))
		return false;

	return UnbindInputActionMapping(OldKeyMapping);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::UnbindAllKeyboardInputsForActionName(FName ActionName)
{
	TArray<FInputActionKeyMapping> ActionMappings = GetKeyboardInputActionMappingsForActionName(ActionName, false);
	for (auto& Binding : ActionMappings)
	{
		UnbindKeyboardInputActionMapping(Binding);
	}

	return ActionMappings.Num() > 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::UnbindInputActionMappings(const TArray<FInputActionKeyMapping>& ActionMappings)
{
	for (const FInputActionKeyMapping& DefaultMapping : ActionMappings)
	{
		UnbindInputActionMapping(DefaultMapping);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::EnsureActionNamesAreInSyncIfModified(const FSoInputActionKeyMapping& ActionMapping)
{
	// Is unbinding, oops can't do anything?
	if (ActionMapping.IsUnbinding())
		return;

	const FName ActionName = ActionMapping.GetActionName();
	const FName SyncWithActionName = FSoInputActionName::GetSyncedActionNameIfModified(ActionName);

	// Nothing to sync with
	if (SyncWithActionName == NAME_None)
		return;

	// Key was reset to default, remove our modifications
	if (IsInputActionMappingDefault(ActionMapping.Modified))
	{
		RemoveModifiedInputActionMappingsForActionName(SyncWithActionName);
		return;
	}

	// Get the synced action mappings
	TArray<FInputActionKeyMapping> SyncActionMappings = GetKeyboardInputActionMappingsForActionName(SyncWithActionName, true);
	if (SyncActionMappings.Num() == 0)
		return;

	// Use the first one as the old one
	const FInputActionKeyMapping OldKeyMapping = SyncActionMappings[0];

	// Use the parent modified as the sync one
	FInputActionKeyMapping NewKeyMapping = ActionMapping.Modified;
	NewKeyMapping.ActionName = SyncWithActionName;

	RebindInputKey(OldKeyMapping, NewKeyMapping, true, false);

	// Unbind the rest
	for (int32 Index = 1; Index < SyncActionMappings.Num(); Index++)
	{
		UnbindKeyboardInputActionMapping(SyncActionMappings[Index]);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::EnsureUnbindingDoesNotBreakGame(const FSoInputActionKeyMapping& ActionMapping)
{
	static const TArray<FKey> CandidatesInputKeys = {
		EKeys::A,
		EKeys::B,
		EKeys::C,
		EKeys::D,
		EKeys::E,
		EKeys::F,
		EKeys::G,
		EKeys::H,
		EKeys::I,
		EKeys::J,
		EKeys::K,
		EKeys::L,
		EKeys::M,
		EKeys::N,
		EKeys::O,
		EKeys::P,
		EKeys::Q,
		EKeys::R,
		EKeys::S,
		EKeys::T,
		EKeys::U,
		EKeys::V,
		EKeys::W,
		EKeys::X,
		EKeys::Y,
		EKeys::Z,

		EKeys::Zero,
		EKeys::One,
		EKeys::Two,
		EKeys::Three,
		EKeys::Four,
		EKeys::Five,
		EKeys::Six,
		EKeys::Seven,
		EKeys::Eight,
		EKeys::Nine,

		EKeys::Tab,
	};
	static const TSet<FName> AlwaysValidUIActionNames = {
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_Action0),
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_Action1),
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_Action2),
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_TopLeft),
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_TopRight),
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_ActionBack),

		// Ensure these are set
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_SpellSelect),
		FSoInputActionName::GetNameFromUICommand(ESoUICommand::EUC_SpellSelectAndCast)
	};

	// Was not unbound, can't use this method
	if (!ActionMapping.IsUnbinding())
		return;

	// Check UI action names are valid
	const FName ActionName = ActionMapping.GetActionName();;
	const ESoInputActionCategoryType ActionCategory = FSoInputActionName::GetCategoryForActionName(ActionName);
	if (ActionCategory == ESoInputActionCategoryType::UI)
	{
		// Must be invalid and in our always valid set
		if (AlwaysValidUIActionNames.Contains(ActionName))
		{
			for (const FKey KeyCandidate : CandidatesInputKeys)
			{
				TArray<FInputActionKeyMapping> ByActionMappings;
				if (!IsInputKeyAlreadyUsed(ActionName, KeyCandidate, ESoInputActionCategoryType::UI, ByActionMappings))
				{
					// Found a valid candidate, use this
					FInputActionKeyMapping NewCandidate = ActionMapping.Modified;
					NewCandidate.Key = KeyCandidate;

					UE_LOG(LogSoGameSettings,
						Warning,
						TEXT("EnsureUnbindingsDoesNotBreakGame: ActionName  = %s can't get unbound so we are assigning it the key = %s"),
						*ActionName.ToString(), *KeyCandidate.ToString());
					RebindInputKey(ActionMapping.Default, NewCandidate, false, false);
					break;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::GetDefaultInputActionMapping(const FInputActionKeyMapping& ModifiedMapping, FInputActionKeyMapping& OutDefault) const
{
	OutDefault = {};
	const TArray<FInputActionKeyMapping> DefaultKeys = GetDefaultInputActionMappingsForModified(ModifiedMapping);
	if (DefaultKeys.Num() != 1)
	{
		return false;
	}

	OutDefault = DefaultKeys[0];
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::SetInputActionMappingToDefault(const FInputActionKeyMapping& ModifiedMapping, bool bUnbindExisting, bool bUsePreset)
{
	// Already a default
	if (IsInputActionMappingDefault(ModifiedMapping))
	{
		UE_LOG(LogSoGameSettings,
			Warning,
			TEXT("SetInputActionMappingToDefault: ModifiedMapping = %s is already Default ¯\\_(ツ)_/¯"),
			*USoInputHelper::ActionMappingToString(ModifiedMapping));
		return false;
	}

	// Arrow preset
	TArray<FInputActionKeyMapping> DefaultPresetMappings;
	if (bUsePreset && bUseKeyboardArrowPresetAsDefault)
	{
		static const USoInputSettings* CustomInputSettings = GetDefault<USoInputSettings>();
		for (const FSoInputPresetKeyRebind& SoMapping : CustomInputSettings->GetKeyboardDefaultArrowsPreset().KeyRebinds)
		{
			const FInputActionKeyMapping Mapping = SoMapping.ToUnrealActionMapping();

			// Modified ActionName matches mapping from the input preset so this is the default I guess
			if (Mapping.ActionName == ModifiedMapping.ActionName)
			{
				DefaultPresetMappings.Add(Mapping);
			}
		}
	}

	// Can we find the default key?
	FInputActionKeyMapping DefaultMapping;
	if (DefaultPresetMappings.Num() == 0)
	{
		// Use the default values
		if (!GetDefaultInputActionMapping(ModifiedMapping, DefaultMapping))
		{
			UE_LOG(LogSoGameSettings, Error, TEXT("SetInputActionMappingToDefault: Can't find default mappings"));
			return false;
		}
	}
	else
	{
		// Use the preset values
		DefaultMapping = DefaultPresetMappings[0];
	}

	// Replace modified action with default
	return RebindInputKey(ModifiedMapping, DefaultMapping, true, bUnbindExisting);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::IsInputChordAlreadyUsed(FName ForActionName, const FInputChord& InputChord,
	ESoInputActionCategoryType FilterCategory, TArray<FInputActionKeyMapping>& OutByActionMappings) const
{
	// Key can have duplicates
	if (ForActionName != NAME_None && FSoInputActionName::CanIgnoreDuplicateBindingForActionName(ForActionName))
		return false;

	const ESoInputActionCategoryType ForActionNameCategoryType = FSoInputActionName::GetCategoryForActionName(ForActionName);
	if (ForActionNameCategoryType == ESoInputActionCategoryType::None)
	{
		UE_LOG(LogSoGameSettings, Error, TEXT("IsInputChordAlreadyUsed: ForActionName = %s got an invalid category type"), *ForActionName.ToString());
		return false;
	}

	// UI subcategory
	const bool bOnlyForACategory = ForActionNameCategoryType != ESoInputActionCategoryType::All;
	const ESoInputActionUICategoryType ForActionNameUICategoryType = FSoInputActionName::GetUICategoryForActionName(ForActionName);

	OutByActionMappings.Empty();

	//
	// NOTE: we Use the slower method here instead of iterating over the fast input map (InputActionMappings)
	// This is because we sometimes call this method before applying the input settings or calling RefreshInputSettingsMappings()
	// which we can't always do if for example we are changing the input preset
	//

	// Rebuild just for this function
	TMap<FName, TArray<FInputActionKeyMapping>> LocalInputActionMappings = GenerateInputActionMappings();

	for (const auto& Elem : LocalInputActionMappings)
	{
		const FName ActionName = Elem.Key;

		// Can ignore duplicates for this action name
		if (FSoInputActionName::CanIgnoreDuplicateBindingForActionName(ActionName))
			continue;

		// Does NOT Matches the input FilterCategory
		if (!FSoInputActionName::IsValidActionNameForCategory(ActionName, FilterCategory))
			continue;

		if (bOnlyForACategory)
		{
			// ActionName has the category different than the ForActionName
			if (ForActionNameCategoryType != FSoInputActionName::GetCategoryForActionName(ActionName))
				continue;

			// The subcategory UI does not match between ForActionName and ActionName, duplicates are allowed
			if (ForActionNameCategoryType == ESoInputActionCategoryType::UI && ForActionNameUICategoryType != FSoInputActionName::GetUICategoryForActionName(ActionName))
				continue;
		}

		// Loop over key mappings of ActionName
		for (const FInputActionKeyMapping& KeyMapping : Elem.Value)
		{
			if (USoInputHelper::DoesActionMappingMatchInputChord(KeyMapping, InputChord))
			{
				// We can't ignore duplicate
				if (!FSoInputActionName::CanIgnoreDuplicateBindingForActionName(KeyMapping.ActionName))
				{
					OutByActionMappings.Add(KeyMapping);
				}
			}
		}
	}

	return OutByActionMappings.Num() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::DoActionNamesHaveTheSameKey(FName FirstActionName, FName SecondActionName, bool bIsKeyboard)
{
	// Either one of them do not exist
	if (!InputActionMappings.Contains(FirstActionName))
		return false;
	if (!InputActionMappings.Contains(SecondActionName))
		return false;

	const TArray<FInputActionKeyMapping>& FirstActionMappings = InputActionMappings.FindChecked(FirstActionName);
	const TArray<FInputActionKeyMapping>& SecondActionMappings = InputActionMappings.FindChecked(SecondActionName);

	// Could this ever happen?
	if (FirstActionMappings.Num() == 0)
		return false;
	if (SecondActionMappings.Num() == 0)
		return false;

	auto IsValidDeviceType = [bIsKeyboard](const FKey Key) -> bool
	{
		// Must be keyboard
		if (bIsKeyboard && !USoInputHelper::IsKeyboardKey(Key))
			return false;

		// Must be gamepad
		if (!bIsKeyboard && !USoInputHelper::IsGamepadKey(Key))
			return false;

		return true;
	};

	// Loop over both options
	for (const FInputActionKeyMapping& First : FirstActionMappings)
	{
		if (!IsValidDeviceType(First.Key))
			continue;

		for (const FInputActionKeyMapping& Second : SecondActionMappings)
		{
			if (!IsValidDeviceType(Second.Key))
				continue;

			// Keys match
			if (First.Key == Second.Key)
				return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FInputActionKeyMapping> USoGameSettings::GetKeyboardInputActionMappingsForActionName(FName ActionName, bool bIncludeUnbound) const
{
	// Only get keyboard action mappings, if bIncludeUnbound is true we also include unbound keys
	const TArray<FInputActionKeyMapping>& AllMappings = GetInputActionMappingsForActionName(ActionName);

	TArray<FInputActionKeyMapping> GamepadMappings;
	for (const FInputActionKeyMapping& KeyMapping : AllMappings)
	{
		const bool bUseUnbound = bIncludeUnbound && !KeyMapping.Key.IsValid();
		const bool bIsKeyboardKey = USoInputHelper::IsKeyboardKey(KeyMapping.Key);
		if (bUseUnbound || bIsKeyboardKey)
			GamepadMappings.Add(KeyMapping);
	}

	return GamepadMappings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FInputActionKeyMapping> USoGameSettings::GetGamepadInputActionMappingsForActionName(FName ActionName, bool bIncludeUnbound) const
{
	// Only get gamepad action mappings, if bIncludeUnbound is true we also include unbound keys
	const TArray<FInputActionKeyMapping>& AllMappings = GetInputActionMappingsForActionName(ActionName);

	TArray<FInputActionKeyMapping> KeyboardMappings;
	for (const FInputActionKeyMapping& KeyMapping : AllMappings)
	{
		const bool bUseUnbound = bIncludeUnbound && !KeyMapping.Key.IsValid();
		const bool bIsGamepadKey = USoInputHelper::IsGamepadKey(KeyMapping.Key);
		if (bUseUnbound || bIsGamepadKey)
			KeyboardMappings.Add(KeyMapping);
	}

	return KeyboardMappings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TArray<FInputActionKeyMapping>& USoGameSettings::GetInputActionMappingsForActionName(FName ActionName) const
{
	const TArray<FInputActionKeyMapping>* Value = InputActionMappings.Find(ActionName);
	return Value == nullptr ? EmptyInputActionKeysArray : *Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TArray<FInputAxisKeyMapping>& USoGameSettings::GetInputAxisMappingForAxisName(FName AxisName) const
{
	const TArray<FInputAxisKeyMapping>* Value = InputAxisMappings.Find(AxisName);
	return Value == nullptr ? EmptyInputAxisKeysArray : *Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FSoInputActionKeyMapping> USoGameSettings::GetModifiedInputActionMappingsForDefault(const FInputActionKeyMapping& DefaultKeyMapping) const
{
	TArray<FSoInputActionKeyMapping> ModifiedMappings;
	for (const FSoInputActionKeyMapping& KeyMapping : ModifiedInputActionMappings)
		if (KeyMapping.Default == DefaultKeyMapping)
			ModifiedMappings.Add(KeyMapping);

	return ModifiedMappings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FInputActionKeyMapping> USoGameSettings::GetDefaultInputActionMappingsForModified(const FInputActionKeyMapping& ModifiedKeyMapping) const
{
	TArray<FInputActionKeyMapping> DefaultMappings;

	// Normal preset
	for (const FSoInputActionKeyMapping& KeyMapping : ModifiedInputActionMappings)
		if (KeyMapping.Modified == ModifiedKeyMapping)
			DefaultMappings.Add(KeyMapping.Default);

	// NOTE: order here is important, as we first check if the modified key exists, if it does not it is most likely a default one
	// Already a default?
	if (DefaultMappings.Num() == 0)
		if (IsInputActionMappingDefault(ModifiedKeyMapping))
			DefaultMappings.Add(ModifiedKeyMapping);

	return DefaultMappings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FSoInputAxisKeyMapping> USoGameSettings::GetModifiedInputAxisMappingsForDefault(const FInputAxisKeyMapping& DefaultAxisMapping) const
{
	TArray<FSoInputAxisKeyMapping> ModifiedMappings;
	for (const FSoInputAxisKeyMapping& AxisMapping : ModifiedInputAxisMappings)
		if (USoInputHelper::AreAxisMappingsEqual(AxisMapping.Default, DefaultAxisMapping))
			ModifiedMappings.Add(AxisMapping);

	return ModifiedMappings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::IsInputActionMappingDefault(const FInputActionKeyMapping& CheckKeyMapping, bool bUsePreset) const
{
	// Arrow preset
	if (bUsePreset && bUseKeyboardArrowPresetAsDefault)
	{
		const USoInputSettings* CustomInputSettings = GetDefault<USoInputSettings>();
		for (const FSoInputPresetKeyRebind& SoMapping : CustomInputSettings->GetKeyboardDefaultArrowsPreset().KeyRebinds)
		{
			const FInputActionKeyMapping Mapping = SoMapping.ToUnrealActionMapping();
			if (Mapping == CheckKeyMapping)
				return true;
		}
	}

	const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (InputSettings == nullptr)
		return false;

	// Is the key mapping in the UInputSettings which represents the Config/DefaultInput.ini
	for (const FInputActionKeyMapping& Mapping : InputSettings->ActionMappings)
		if (Mapping == CheckKeyMapping)
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::IsInputAxisMappingDefault(const FInputAxisKeyMapping& CheckAxisMapping) const
{
	const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
	if (InputSettings == nullptr)
		return false;

	// Is the key mapping in the UInputSettings which represents the Config/DefaultInput.ini
	for (const FInputAxisKeyMapping& Mapping : InputSettings->AxisMappings)
		if (USoInputHelper::AreAxisMappingsEqual(Mapping, CheckAxisMapping))
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::GetModifiedInputAxisMappingForActionMapping(const FSoInputActionKeyMapping& ActionMapping,
	FName AxisNameToModify, FSoInputAxisKeyMapping& OutAxisMapping) const
{
	// TODO this method seems a bit too much, what is wrong with just checking InputAxisMappings?
	// Something is wrong
	if (ValidateModifiedInputActionMapping(ActionMapping) != ESoInputIntegrity::Ok)
		return false;

	// No Axis name
	if (AxisNameToModify == NAME_None)
		return false;

	// Does not event exist
	if (!InputAxisMappings.Contains(AxisNameToModify))
	{
		UE_LOG(LogSoGameSettings,
			Error,
			TEXT("GetInputAxisMappingsFromInputActionMapping: AxisName = %s Does not even exist in the InputAxisMappings"),
			*AxisNameToModify.ToString());
		return false;
	}

	// Get the axis mapping from the default axis mappings
	auto GetFromDefaultSettings = [AxisNameToModify, &OutAxisMapping](const FInputActionKeyMapping& CompareKeyMapping) -> bool
	{
		const UInputSettings* InputSettings = UInputSettings::GetInputSettings();
		if (InputSettings == nullptr)
			return false;

		bool bFound = false;
		for (const FInputAxisKeyMapping& DefaultAxis : InputSettings->AxisMappings)
		{
			// AxisName and Key must match
			if (DefaultAxis.AxisName == AxisNameToModify && DefaultAxis.Key == CompareKeyMapping.Key)
			{
				if (bFound)
				{
					UE_LOG(LogSoGameSettings,
						Warning,
						TEXT("GetInputAxisMappingsFromInputActionMapping: Found another default axis for default CompareKeyMapping = %s. Ignoring"),
						*USoInputHelper::ActionMappingToString(CompareKeyMapping));
				}
				else
				{
					bFound = true;
					OutAxisMapping.Default = DefaultAxis;
					OutAxisMapping.Modified = DefaultAxis;
				}
			}
		}

		// Could not find any default axis mapping, something is wrong
		if (!bFound)
		{
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("GetInputAxisMappingsFromInputActionMapping: Could not find any default axis mapping for default CompareKeyMapping = %s"),
				*USoInputHelper::ActionMappingToString(CompareKeyMapping));
			return false;
		}

		return true;
	};

	// If the ActionMapping is default it must be that also the AxisMapping must be default
	if (IsInputActionMappingDefault(ActionMapping.Modified))
	{
		return GetFromDefaultSettings(ActionMapping.Modified);
	}

	// Try with the default action mapping
	const bool bFound = GetFromDefaultSettings(ActionMapping.Default);
	if (!bFound)
	{
		UE_LOG(LogSoGameSettings,
			   Error,
			   TEXT("GetInputAxisMappingsFromInputActionMapping: Could not find any axis mapping for ActionMapping=%s"),
			   *ActionMapping.ToString());
	}

	// New key for axis
	OutAxisMapping.Modified.Key = ActionMapping.Modified.Key;
	return bFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::AddModifiedInputActionMapping(const FSoInputActionKeyMapping& ActionMapping)
{
	// Something is wrong
	if (ValidateModifiedInputActionMapping(ActionMapping) != ESoInputIntegrity::Ok)
		return false;

	// New key is default, just remove current modified one
	if (IsInputActionMappingDefault(ActionMapping.Modified))
	{
		// They should be equal shouldn't they?
		if (!(ActionMapping.Default == ActionMapping.Modified))
		{
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("AddModifiedInputActionMapping: ActionMapping.Default (%s) != ActionMapping.Modified (%s)"),
				*USoInputHelper::ActionMappingToString(ActionMapping.Default), *USoInputHelper::ActionMappingToString(ActionMapping.Modified));
		}

		RemoveModifiedInputActionMappingsForDefault(ActionMapping.Modified);
		return true;
	}

	// Did we find at least one modified action
	bool bFound = false;

	// Replace all actions
	for (FSoInputActionKeyMapping& Mapping : ModifiedInputActionMappings)
	{
		if (Mapping.Default == ActionMapping.Default)
		{
			Mapping.Modified = ActionMapping.Modified;
			bFound = true;
		}
	}

	// Add it for the first time
	if (!bFound)
		ModifiedInputActionMappings.Add(ActionMapping);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::AddModifiedInputAxisMapping(const FSoInputAxisKeyMapping& AxisMapping)
{
	// Something is wrong
	if (ValidateModifiedInputAxisMapping(AxisMapping) != ESoInputIntegrity::Ok)
		return false;

	// New axis is default, just remove the current modified one
	if (IsInputAxisMappingDefault(AxisMapping.Modified))
	{
		// They should be equal shouldn't they?
		if (!USoInputHelper::AreAxisMappingsEqual(AxisMapping.Default, AxisMapping.Modified))
		{
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("AddModifiedInputAxisMapping: AxisMapping.Default (%s) !=  AxisMapping.Modified (%s)"),
				*USoInputHelper::AxisMappingToString(AxisMapping.Default), *USoInputHelper::AxisMappingToString(AxisMapping.Modified));
		}

		RemoveModifiedInputAxisMappingsForDefault(AxisMapping.Modified);
		return true;
	}

	// Did we find at least one modified axis
	bool bFound = false;

	// Replace all axis
	for (FSoInputAxisKeyMapping& Mapping : ModifiedInputAxisMappings)
	{
		if (USoInputHelper::AreAxisMappingsEqual(Mapping.Default, AxisMapping.Default))
		{
			Mapping.Modified = AxisMapping.Modified;
			bFound = true;
		}
	}

	// Add it for the first time
	if (!bFound)
		ModifiedInputAxisMappings.Add(AxisMapping);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RemoveModifiedInputActionMappingsForActionName(FName ActionName)
{
	for (int32 i = ModifiedInputActionMappings.Num() - 1; i >= 0; i--)
		if (ModifiedInputActionMappings[i].GetActionName() == ActionName)
			ModifiedInputActionMappings.RemoveAtSwap(i);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RemoveModifiedInputActionMappingsForDefault(const FInputActionKeyMapping& DefaultKeyMapping)
{
	for (int32 i = ModifiedInputActionMappings.Num() - 1; i >= 0; i--)
		if (ModifiedInputActionMappings[i].Default == DefaultKeyMapping)
			ModifiedInputActionMappings.RemoveAtSwap(i);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RemoveModifiedInputAxisMappingsForDefault(const FInputAxisKeyMapping& DefaultAxisMapping)
{
	for (int32 i = ModifiedInputAxisMappings.Num() - 1; i >= 0; i--)
		if (USoInputHelper::AreAxisMappingsEqual(ModifiedInputAxisMappings[i].Default, DefaultAxisMapping))
			ModifiedInputAxisMappings.RemoveAtSwap(i);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::GetAllInputActionMappings(TArray<FInputActionKeyMapping>& OutMappings) const
{
	// Add all the values to the array
	for (const auto& Elem : InputActionMappings)
		OutMappings.Append(Elem.Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::GetAllInputAxisMappings(TArray<FInputAxisKeyMapping>& OutMappings) const
{
	// Add all the values to the array
	for (const auto& Elem : InputAxisMappings)
		OutMappings.Append(Elem.Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputIntegrity USoGameSettings::ValidateModifiedInputActionMapping(const FSoInputActionKeyMapping& ActionMapping, bool bLog) const
{
	if (!IsInputActionMappingDefault(ActionMapping.Default))
	{
		if (bLog)
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("ValidateModifiedInputActionMapping: ActionMapping.Default = %s is not Default :("),
				*USoInputHelper::ActionMappingToString(ActionMapping.Default));

		return ESoInputIntegrity::NotDefault;
	}

	const ESoInputIntegrity ReturnStatus = ActionMapping.ValidateIntegrity();
	if (bLog)
	{
		switch (ReturnStatus)
		{
		case ESoInputIntegrity::NamesMismatch:
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("ValidateModifiedInputActionMapping: ActionMapping.Default.ActionName (%s) != ActionMapping.Modified.ActionName (%s)"),
				*ActionMapping.Default.ActionName.ToString(), *ActionMapping.Modified.ActionName.ToString());
			break;
		case ESoInputIntegrity::KeyCategoryMismatch:
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("ValidateModifiedInputActionMapping: ActionMapping.Default.Key (%s) not the same category (keyboard, gamepad) as ActionMapping.Modified.Key (%s)"),
				*ActionMapping.Default.Key.GetDisplayName().ToString(), *ActionMapping.Modified.Key.GetDisplayName().ToString());
			break;
		default:
			break;
		}
	}

	return ReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoInputIntegrity USoGameSettings::ValidateModifiedInputAxisMapping(const FSoInputAxisKeyMapping& AxisMapping, bool bLog) const
{
	if (!IsInputAxisMappingDefault(AxisMapping.Default))
	{
		if (bLog)
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("ValidateModifiedInputAxisMapping: AxisMapping.Default = %s is not Default :("),
				*USoInputHelper::AxisMappingToString(AxisMapping.Default));

		return ESoInputIntegrity::NotDefault;
	}

	const ESoInputIntegrity ReturnStatus = AxisMapping.ValidateIntegrity();
	if (bLog)
	{
		switch (ReturnStatus)
		{
		case ESoInputIntegrity::NamesMismatch:
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("ValidateModifiedInputAxisMapping: AxisMapping.Default.AxisName (%s) != AxisMapping.Modified.AxisName (%s)"),
				*AxisMapping.Default.AxisName.ToString(), *AxisMapping.Modified.AxisName.ToString());
			break;
		case ESoInputIntegrity::KeyCategoryMismatch:
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("ValidateModifiedInputAxisMapping: AxisMapping.Default.Key (%s) not the same category (keyboard, gamepad) as AxisMapping.Modified.Key (%s)"),
				*AxisMapping.Default.Key.GetDisplayName().ToString(), *AxisMapping.Modified.Key.GetDisplayName().ToString());
			break;
		default:
			break;
		}
	}

	return ReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::CheckAndRepairModifiedInputSettings()
{
	// First check if the defaults changed by us or by the user. Invalidate any modified keys if they did

	// ActionMappings
	TArray<FSoInputActionKeyMapping> RemovedActionMappings;
	TSet<FInputActionKeyMapping> DefaultActionMappings;
	for (int32 Index = ModifiedInputActionMappings.Num() - 1; Index >= 0; Index--)
	{
		const FSoInputActionKeyMapping& ModifiedMapping = ModifiedInputActionMappings[Index];
		const ESoInputIntegrity ReturnStatus = ValidateModifiedInputActionMapping(ModifiedMapping, false);

		switch (ReturnStatus)
		{
		case ESoInputIntegrity::NamesMismatch:
			UE_LOG(LogSoGameSettings,
				Error,
				TEXT("CheckAndRepairdModifiedInputSettings: ActionMapping.Default.ActionName (%s) != ActionMapping.Modified.ActionName (%s). "
					"The action names were modified manually?"
					"Removing modification at index = %d"),
				*ModifiedMapping.Default.ActionName.ToString(), *ModifiedMapping.Modified.ActionName.ToString(), Index);
			break;

		case ESoInputIntegrity::KeyCategoryMismatch:
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("CheckAndRepairdModifiedInputSettings: ActionMapping.Default.Key (%s) not the same category (keyboard, gamepad) as ActionMapping.Modified.Key (%s). "
					"The action keys were modified manually?"
					"Removing modification at index = %d"),
				*ModifiedMapping.Default.Key.GetDisplayName().ToString(), *ModifiedMapping.Modified.Key.GetDisplayName().ToString(), Index);
			break;

		case ESoInputIntegrity::NotDefault:
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("CheckAndRepairdModifiedInputSettings: Default action mapping = %s does not exist anymore, "
					"the game had it's defaults modified or you edited the user settings by yourself. "
					"Removing modification at index = %d"),
				*USoInputHelper::ActionMappingToString(ModifiedMapping.Default), Index);
			break;

		default:
			break;
		}

		// Can't have same default twice
		bool bFoundDuplicateDefault = false;
		if (DefaultActionMappings.Contains(ModifiedMapping.Default))
		{
			bFoundDuplicateDefault = true;
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("CheckAndRepairdModifiedInputSettings: Default action mapping = %s at index %d is a duplicate. "
					"The game had it's defaults modified or you edited the user settings by yourself. "
					"Removing modification at index = %d"),
				*USoInputHelper::ActionMappingToString(ModifiedMapping.Default), Index, Index);
		}
		else
		{
			DefaultActionMappings.Add(ModifiedMapping.Default);
		}

		if (ReturnStatus != ESoInputIntegrity::Ok)
		{
			RemovedActionMappings.Add(ModifiedMapping);
			ModifiedInputActionMappings.RemoveAtSwap(Index);
		}
		else if (bFoundDuplicateDefault)
		{
			// Do not remove any dependencies as this was a duplicate
			ModifiedInputActionMappings.RemoveAtSwap(Index);
		}
	}

	// AxisMappings
	TArray<FSoInputAxisKeyMapping> RemovedAxisMappings;
	TSet<FInputAxisKeyMapping> DefaultAxisMappings;
	for (int32 Index = ModifiedInputAxisMappings.Num() - 1; Index >= 0; Index--)
	{
		const FSoInputAxisKeyMapping& ModifiedMapping = ModifiedInputAxisMappings[Index];
		const ESoInputIntegrity ReturnStatus = ValidateModifiedInputAxisMapping(ModifiedMapping, false);

		switch (ReturnStatus)
		{
		case ESoInputIntegrity::NamesMismatch:
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("RefreshInputSettingsMappings: AxisMapping.Default.AxisName (%s) != AxisMapping.Modified.AxisName (%s). "
					"The axis names were modified manually? "
					"Removing modification at index = %d"),
				*ModifiedMapping.Default.AxisName.ToString(), *ModifiedMapping.Modified.AxisName.ToString(), Index);
			break;

		case ESoInputIntegrity::KeyCategoryMismatch:
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("CheckAndRepairdModifiedInputSettings: AxisMapping.Default.Key (%s) not the same category (keyboard, gamepad) as AxisMapping.Modified.Key (%s). "
					"The axis keys were modified manually?"
					"Removing modification at index = %d"),
				*ModifiedMapping.Default.Key.GetDisplayName().ToString(), *ModifiedMapping.Modified.Key.GetDisplayName().ToString(), Index);
			break;

		case ESoInputIntegrity::NotDefault:
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("RefreshInputSettingsMappings: Default axis mapping = %s does not exist anymore, "
					"the game had it's defaults modified or you edited the user settings by yourself. "
					"Removing modification at index = %d"),
				*USoInputHelper::AxisMappingToString(ModifiedMapping.Default), Index);
			break;

		default:
			break;
		}

		// Can't have same default twice
		bool bFoundDuplicateDefault = false;
		if (DefaultAxisMappings.Contains(ModifiedMapping.Default))
		{
			bFoundDuplicateDefault = true;
			UE_LOG(LogSoGameSettings,
				Warning,
				TEXT("CheckAndRepairdModifiedInputSettings: Default axis mapping = %s at index %d is a duplicate. "
					"The game had it's defaults modified or you edited the user settings by yourself. "
					"Removing modification at index = %d"),
				*USoInputHelper::AxisMappingToString(ModifiedMapping.Default), Index, Index);
		}
		else
		{
			DefaultAxisMappings.Add(ModifiedMapping.Default);
		}

		if (ReturnStatus != ESoInputIntegrity::Ok)
		{
			RemovedAxisMappings.Add(ModifiedMapping);
			ModifiedInputAxisMappings.RemoveAtSwap(Index);
		}
		else if (bFoundDuplicateDefault)
		{
			// Do not remove any dependencies as this was a duplicate
			ModifiedInputAxisMappings.RemoveAtSwap(Index);
		}
	}

	// Remove leftover axis names
	auto RemoveAxisForAction = [this](const FInputActionKeyMapping ToRemove)
	{
		for (int32 Index = ModifiedInputAxisMappings.Num() - 1; Index >= 0; Index--)
		{
			const FSoInputAxisKeyMapping& ModifiedMapping = ModifiedInputAxisMappings[Index];
			if (USoInputHelper::DoesActionMappingMatchAxisMapping(ToRemove, ModifiedMapping.Default) ||
				USoInputHelper::DoesActionMappingMatchAxisMapping(ToRemove, ModifiedMapping.Modified))
			{
				UE_LOG(LogSoGameSettings,
					Warning,
					TEXT("Removed leftover modified axis mapping = %s at index = %d"),
					*ModifiedMapping.ToString(), Index);
				ModifiedInputAxisMappings.RemoveAtSwap(Index);
			}
		}
	};
	for (const FSoInputActionKeyMapping& Removed : RemovedActionMappings)
	{
		RemoveAxisForAction(Removed.Default);
		RemoveAxisForAction(Removed.Modified);
	}

	// Remove leftover action names
	auto RemoveActionForAxis = [this](const FInputAxisKeyMapping ToRemove)
	{
		for (int32 Index = ModifiedInputActionMappings.Num() - 1; Index >= 0; Index--)
		{
			const FSoInputActionKeyMapping& ModifiedMapping = ModifiedInputActionMappings[Index];
			if (USoInputHelper::DoesActionMappingMatchAxisMapping(ModifiedMapping.Default, ToRemove) ||
				USoInputHelper::DoesActionMappingMatchAxisMapping(ModifiedMapping.Modified, ToRemove))
			{
				UE_LOG(LogSoGameSettings,
					Warning,
					TEXT("Removed leftover modified action mapping = %s at index = %d"),
					*ModifiedMapping.ToString(), Index);
				ModifiedInputActionMappings.RemoveAtSwap(Index);
			}
		}
	};
	for (const FSoInputAxisKeyMapping& Removed : RemovedAxisMappings)
	{
		RemoveActionForAxis(Removed.Default);
		RemoveActionForAxis(Removed.Modified);
	}

	// TODO check if Action matches the Axis
	// TODO multiple IsInputChordAlreadyUsed
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RebuildInputKeymapsForAllPlayers() const
{
	for (TObjectIterator<UPlayerInput> It; It; ++It)
	{
		RebuildInputKeymapsForPlayerInput(*It);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::RebuildInputKeymapsForPlayerInput(UPlayerInput* PlayerInput) const
{
	if (!IsValid(PlayerInput))
		return;

	// Load from default first
	PlayerInput->ForceRebuildingKeyMaps(true);

	// Override with our values
	PlayerInput->AxisMappings.Empty();
	PlayerInput->ActionMappings.Empty();
	GetAllInputAxisMappings(PlayerInput->AxisMappings);
	GetAllInputActionMappings(PlayerInput->ActionMappings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSettings::AreDefaultInputSettingsUsed() const
{
	return ModifiedInputActionMappings.Num() == 0 && ModifiedInputAxisMappings.Num() == 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSettings::SetCharacterSkinType(ESoCharacterSkinType InCharacterSkinType)
{
	CharacterSkinType = InCharacterSkinType;
	ModifiedDateTimeCharacterSkinType = FDateTime::Now();
	OnCharacterSkinChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoCharacterSkinType USoGameSettings::GetCharacterSkinType() const
{
	return CharacterSkinType;
}
