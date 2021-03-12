// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerInput.h"

#include "Settings/Input/SoInputNames.h"
#include "Input/SoInputSettingsTypes.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Input/SoInputSettings.h"
#include "SoAudioSettingsTypes.h"

#include "SoGameSettings.generated.h"


class UPlayerInput;
class AGameModeBase;
class USoGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoSettingsNotify);

struct FSoGameSpeedVersion
{
	enum Type
	{
		BeforeVersion = 0,
		Initial, // speed 1.x

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
};


UENUM(BlueprintType)
enum class ESoBounceModeType : uint8
{
	// Disabled
	Never = 0,

	// Only enabled in easy mode
	OnEasy,

	// Always enabled
	Always,

	Num				UMETA(Hidden)
};


UENUM(BlueprintType)
enum class ESoCharacterSkinType : uint8
{
	Classic = 0,
	Mummy,
	Pumpkin,

	Num				UMETA(Hidden)
};

//
// Custom game user settings, default saved to Saved/<Platform>/GameUserSettings.ini
//
// InputSettings: https://answers.unrealengine.com/questions/542866/key-mapping-revert-to-default-keys.html
// - never modifies the UInputSettings class, those are considered defaults
// - modified keys are stored here
//
// Priorities (higher order takes precedence):
// 1. Value defined in this cpp file: Property = Value, see ResetVariablesToDefault()
// 2. Value from Config/DefaultGameUserSettings.ini (or any other default config file)
// 3. Value set by user in Saved/<Platform>/GameUserSettings.ini
//
// NOTE: some values are set directly in other configs
// - Gamma/Brightness - inside the GEngine
// - ForceFeedbackScale/VibrationScale - in PlayerController
// - Culture/Language - inside the config GGameUserSettingsIni the Internationalization section (See USoLocalizationHelper::SetCurrentCultureEverywhere)
//
UCLASS()
class SORB_API USoGameSettings : public UGameUserSettings
{
	GENERATED_BODY()
public:
	USoGameSettings();
	virtual ~USoGameSettings();

	/** UGameUserSettings Interface */
	void SetToDefaults() override;

	/** Resets all settings to the current system settings */
	void ResetToCurrentSettings() override;

	/** Applies all the settings and saves to disk */
	void ApplySettings(bool bCheckForCommandLineOverrides) override;

	/** Loads the user settings from persistent storage */
	void LoadSettings(bool bForceReload = false) override;

	/** Save the user settings to persistent storage (automatically happens as part of ApplySettings) */
	void SaveSettings() override;

	/** Validates and resets bad user settings to default. Deletes stale user settings file if necessary. */
	void ValidateSettings() override;

	// Own functions

	// Gets the current game settings singleton
	UFUNCTION(BlueprintPure, DisplayName = "Get So Game Settings", Category = Settings)
	static USoGameSettings* GetInstance();
	static USoGameSettings& Get()
	{
		auto* Instance = GetInstance();
		check(IsValid(Instance));
		return *Instance;
	}

	// Similar to SetToDefaults but only affects variables
	void ResetVariablesToDefault();

	void SetBlockConfigSave(bool bBlock) { bBlockConfigSave = bBlock; }

	/**
	 *  After the mode is activated it prevents the saves
	 *  Once it is deactivated it saves if there was any save request since the activation
	 *  Used on switch to reduce the amount of write access
	 */
	void SetCacheSaveRequestsMode(bool bActive);

	// Called before GameMode StartPlay
	void BeforeGameModeStartPlay();

	FString ToStringGameSettings() const
	{
		return FString::Printf(
			TEXT("\tbIsFirstTimeGameStarted = %d\n")
			TEXT("\tbDisplayFPS = %d, bDisplayTime = %d, bDisplayDamageTexts = %d, bDisplayEnemyHealthBar = %d, bDisplayFloatingVOLines = %d, bDisplayStat = %d\n")
			TEXT("\tbPauseGameWhenUnfocused = %d, bPauseGameOnIdle = %d, IdleTimeSecondsBeforeGamePause = %f\n")
			TEXT("\tBounceModeType = %d\n")
			TEXT("\tCharacterSkinType = %d\n")
			TEXT("\tbCollectGameAnalytics = %d, UnixTimeModifiedCollectGameAnalytics = %lld, bWaitForAnalyticsToSend = %d\n")
			TEXT("\tAnalyticsWaitSeconds = %s, AnalyticsProcessEventsSeconds = %s\n")
			TEXT("\tbAutoSaveAndLoad = %d, bAutoBackupDeletedSaves = %d, bAutoBackupBeforeSave = %d"),
			bIsFirstTimeGameStarted,
			bDisplayFPS, bDisplayTime, bDisplayDamageTexts, bDisplayEnemyHealthBar, bDisplayFloatingVOLines, bDisplayStat,
			bPauseGameWhenUnfocused, bPauseGameOnIdle, IdleTimeSecondsBeforeGamePause,
			static_cast<int32>(BounceModeType),
			static_cast<int32>(CharacterSkinType),
			bCollectGameAnalytics, UnixTimeModifiedCollectGameAnalytics, bWaitForAnalyticsToSend,
			*FString::SanitizeFloat(AnalyticsWaitSeconds), *FString::SanitizeFloat(AnalyticsProcessEventsSeconds),
			bAutoSaveAndLoad, bAutoBackupDeletedSaves, bAutoBackupBeforeSave
		);
	}

	FString ToStringScalabilitySettings() const
	{
		return FString::Printf(
			TEXT("Scalability:\n")
			TEXT("\tResolutionQuality = %s, ViewDistanceQuality = %d, AntiAliasingQuality = %d\n")
			TEXT("\tShadowQuality = %d, PostProcessQuality = %d, TextureQuality = %d\n")
			TEXT("\tEffectsQuality = %d, FoliageQuality = %d"),
			*FString::SanitizeFloat(ScalabilityQuality.ResolutionQuality), ScalabilityQuality.ViewDistanceQuality, ScalabilityQuality.AntiAliasingQuality,
			ScalabilityQuality.ShadowQuality, ScalabilityQuality.PostProcessQuality, ScalabilityQuality.TextureQuality,
			ScalabilityQuality.EffectsQuality, ScalabilityQuality.FoliageQuality
		);
	}

	FString ToStringDisplaySettings() const
	{
		const FString ScalabilityString = ToStringScalabilitySettings();
		return FString::Printf(
			TEXT("\tResolutionSizeX = %u, ResolutionSizeY = %u\n")
			TEXT("\tLastUserConfirmedResolutionSizeX = %u, LastUserConfirmedResolutionSizeY = %u\n")
			TEXT("\tbUseDesiredScreenHeight = %d, DesiredScreenWidth = %d, DesiredScreenHeight = %d\n")
			TEXT("\tFrameRateLimit = %s, MinResolutionScale = %s\n")
			TEXT("\tWindowPosX = %d, WindowPosY = %d\n")
			TEXT("\tFullscreenMode = %d, LastConfirmedFullscreenMode = %d, PreferredFullscreenMode = %d, IsWindowModeFullScreen = %d\n")
			TEXT("\tbVolumetricFog = %d\n")
			TEXT("\tbForceWeakHardwareOptimizations = %d\n")
			TEXT("\n\t%s"),
			ResolutionSizeX, ResolutionSizeY,
			LastUserConfirmedResolutionSizeX, LastUserConfirmedResolutionSizeY,
			bUseDesiredScreenHeight, DesiredScreenWidth, DesiredScreenHeight,
			*FString::SanitizeFloat(FrameRateLimit), *FString::SanitizeFloat(MinResolutionScale),
			WindowPosX, WindowPosY,
			FullscreenMode, LastConfirmedFullscreenMode, PreferredFullscreenMode, IsWindowModeFullScreen(),
			bVolumetricFog,
			bForceWeakHardwareOptimizations,
			*ScalabilityString
		);
	}

	FString ToStringAudioSettings() const
	{
		return FString::Printf(
			TEXT("\tAudioQualityLevel = %d, bMuteAudio = %d\n")
			TEXT("\tVolumeMaster = %s, VolumeMusic = %s, VolumeSFX = %s"),
			AudioQualityLevel, bMuteAudio,
			*FString::SanitizeFloat(VolumeMaster), *FString::SanitizeFloat(VolumeMusic), *FString::SanitizeFloat(VolumeSFX)
		);
	}

	FString ToString() const
	{
		return FString::Printf(
			TEXT("GameSettings:\n")
			TEXT("\tVersion = %u\n")
			TEXT("\n%s\n")
			TEXT("\n%s\n")
			TEXT("\n%s\n"),
			Version, *ToStringGameSettings(), *ToStringDisplaySettings(), *ToStringAudioSettings()
		);
	}

	// Sets the reference world object, used to get the UWorld
	UFUNCTION(BlueprintCallable, Category = ">Settings")
	void SetWorldContextObject(UObject* InWorldContextObject) { WorldContextObject = InWorldContextObject; }

	UObject* GetWorldContextObject() const { return WorldContextObject; }

	/** Logs out game metrics */
	void PrintToLog() const;

	// Apply only the game settings
	// NOTE: this does not reset all settings, use ApplySettings for that
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void ApplyGameSettings(bool bCheckForCommandLineOverrides, bool bSaveSettings = true);

	// Sets the custom game settings to default
	// NOTE: this does not reset all settings, use SetToDefaults for that
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetGameSettingsToDefault();
	void ResetGameSettingsVariables();

	// Validates and resets bad game settings
	// NOTE: this does not validate all settings, use ValidateSettings for that
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void ValidateGameSettings();

	////////////////////////////////////////////////////
	// Video Demo settings
	// Used  for conferences
	////////////////////////////////////////////////////

	UFUNCTION(BlueprintPure, Category = ">Settings|Demo")
	bool CanVideoDemoDisableMenu() const { return bVideoDemoDisableMainMenu; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Demo")
	void SetIsVideoDemoDisableMenu(bool bValue) { bVideoDemoDisableMainMenu = bValue; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Demo")
	bool CanVideoDemoStartVideoOnIdle() const { return bVideoDemoStartVideoOnIdle; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Demo")
	void SetVideoDemoStartVideoOnIdle(bool bValue) { bVideoDemoStartVideoOnIdle = bValue; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	float GetVideoDemoIdleTimeSecondsStartVideo() const { return VideoDemoIdleTimeSecondsStartVideo; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetVideoDemoIdleTimeSecondsStartVideo(float InValue) { VideoDemoIdleTimeSecondsStartVideo = InValue; }

	////////////////////////////////////////////////////
	// Game settings
	////////////////////////////////////////////////////

	// Called when the player selects the language the first time before anything else is displayed
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void OnLanguageSelected();

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool WasLanguageSelected() const { return bLanguageAlreadySelected; }

	// Is the game starting for the first time?
	bool IsFirstTimeGameStarted() const { return bIsFirstTimeGameStarted; }

	// Custom maps support
	bool AreCustomMapsEnabled() const { return bCustomMapsEnabled; }
	void SetAreCustomMapsEnabled(bool bValue) { bCustomMapsEnabled = bValue; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsDisplayFPSEnabled() const { return bDisplayFPS; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsDisplayTimeEnabled() const { return bDisplayTime; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsDisplayDamageTextsEnabled() const { return bDisplayDamageTexts; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsDisplayEnemyHealthBarEnabled() const { return bDisplayEnemyHealthBar; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsDisplayFloatingVOLinesEnabled() const { return bDisplayFloatingVOLines; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsDisplayedStatsEnabled() const { return bDisplayStat; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool CanPauseGameWhenUnfocused() const { return bPauseGameWhenUnfocused;  }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetPauseGameWhenUnfocused(bool bEnabled) { bPauseGameWhenUnfocused = bEnabled; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool CanPauseGameOnIdle() const { return bPauseGameOnIdle; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetPauseGameOnIdle(bool bEnabled) { bPauseGameOnIdle = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetBounceModeType(ESoBounceModeType InBounceMode) { BounceModeType = InBounceMode; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	ESoBounceModeType GetBounceModeType() const { return BounceModeType; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetCharacterSkinType(ESoCharacterSkinType InCharacterSkinType);

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	ESoCharacterSkinType GetCharacterSkinType() const;

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	float GetIdleTimeSecondsBeforeGamePause() const { return IdleTimeSecondsBeforeGamePause; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetDisplayFPSEnabled(bool bEnabled) { bDisplayFPS = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetDisplayTimeEnabled(bool bEnabled) { bDisplayTime = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetDisplayDamageTexts(bool bEnabled) { bDisplayDamageTexts = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetDisplayEnemyHealthBar(bool bEnabled)
	{
		bDisplayEnemyHealthBar = bEnabled;
		OnDisplayEnemyHealthBarChanged.Broadcast();
	}

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetDisplayFloatingVOLines(bool bEnabled) { bDisplayFloatingVOLines = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetDisplayStatsEnabled(bool bEnabled) { bDisplayStat = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Game")
	void SetCollectGameAnalyticsEnabled(bool bEnabled)
	{
		bCollectGameAnalytics = bEnabled;
		UnixTimeModifiedCollectGameAnalytics = FDateTime::UtcNow().ToUnixTimestamp();
	}

	//
	// Game Speed
	//

	// This sets the current game speed but does not store it for saving. This function is Instant.
	// To reverse just call ApplyGameSettings() to apply the game settings values;
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Speed")
	void SetTemporaryGameSpeed(float NewGameSpeed);

	// Was SetInstantTemporaryGameSpeed set?
	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Speed")
	bool IsTemporaryGameSpeedChanged() const;

	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Speed")
	float GetTemporaryGameSpeed() const;

	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Speed")
	void SetGameSpeed(float NewGameSpeed) { GameSpeed = FMath::Abs(NewGameSpeed); }

	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Speed")
	void ResetGameSpeedToDefault() { SetGameSpeed(DefaultGameSpeed); }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Speed")
	float GetGameSpeed() const { return GameSpeed; }

	// Is it changed from the default (DefaultGameSpeed)
	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Speed")
	bool IsGameSpeedChanged() const { return !FMath::IsNearlyEqual(GameSpeed, DefaultGameSpeed); }

	//
	// Brightness
	//

	// Set the Brightness. Note the valid range is between [MinBrightness, MaxBrightness]
	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Brightness")
	void SetBrightness(float NewBrightness)
	{
		Brightness = FMath::Clamp(NewBrightness, MinBrightness, MaxBrightness);
	}

	// Same as SetBrightness only the value of NewBrightnessNormalized is in range [0, 1]
	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Brightness")
	void SetBrightnessNormalized(float NewBrightnessNormalized)
	{
		// Get a valid value in range
		SetBrightness(FMath::Lerp(MinBrightness, MaxBrightness, FMath::Clamp(NewBrightnessNormalized, 0.f, 1.f)));
	}

	// Resets the Brightness to the default value (DefaultBrightness)
	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Brightness")
	void ResetBrightnessToDefault() { SetBrightness(DefaultBrightness); }

	// Value in range [MinBrightness, MaxBrightness]
	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Brightness")
	float GetBrightness() const { return Brightness; }

	// Value in range [0, 1]
	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Brightness")
	float GetBrightnessNormalized() const
	{
		return NormalizeBrightness(GetBrightness());
	}

	// Converts a normal value to a normalized value in the range [0, 1]
	static float NormalizeBrightness(float Value)
	{
		return (Value - MinBrightness) / (MaxBrightness - MinBrightness);
	}
	static bool IsEqualToDefaultNormalizedBrightness(float NormalizedValue)
	{
		static const float DefaultNormalized = NormalizeBrightness(DefaultBrightness);
		return FMath::IsNearlyEqual(NormalizedValue, DefaultNormalized, KINDA_SMALL_NUMBER);
	}

	//
	// Gamepad vibration
	//

	// Set the Player Controller. Note the valid range is between [MinVibrationScale, MaxVibrationScale]
	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Gamepad")
	void SetVibrationScale(float NewValue)
	{
		VibrationScale = FMath::Clamp(NewValue, MinVibrationScale, MaxVibrationScale);
	}

	// Same as SetVibrationScale only the value of NewValueNormalized is in range [0, 1]
	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Gamepad")
	void SetVibrationScaleNormalized(float NewValueNormalized)
	{
		// Get a valid value in range
		SetVibrationScale(
			FMath::Lerp(MinVibrationScale, MaxVibrationScale, FMath::Clamp(NewValueNormalized, 0.f, 1.f))
		);
	}

	// Resets the VibrationScale to the default value (DefaultVibrationScale)
	// NOTE: you must call ApplyGameSettings() for this setting to propagate
	UFUNCTION(BlueprintCallable, Category = ">Settings|Game|Gamepad")
	void ResetVibrationScaleToDefault() { SetVibrationScale(DefaultVibrationScale); }

	// Value in range [MinVibrationScale, MaxVibrationScale]
	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Gamepad")
	float GetVibrationScale() const { return VibrationScale; }

	// Value in range [0, 1]
	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Gamepad")
	float GetVibrationScaleNormalized() const
	{
		return NormalizeVibrationScale(GetVibrationScale());
	}

	// Converts a normal value to a normalized value in the range [0, 1]
	static float NormalizeVibrationScale(float Value)
	{
		return (Value - MinVibrationScale) / (MaxVibrationScale - MinVibrationScale);
	}
	static bool IsEqualToDefaultNormalizedVibrationScale(float NormalizedValue)
	{
		static const float DefaultNormalized = NormalizeVibrationScale(DefaultVibrationScale);
		return FMath::IsNearlyEqual(NormalizedValue, DefaultNormalized, KINDA_SMALL_NUMBER);
	}

	UFUNCTION(BlueprintPure, Category = ">Settings|Game|Gamepad")
	bool IsVibrationEnabled() const { return GetVibrationScale() > MinVibrationScale; }

	//
	// Analytics
	//

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool CanCollectGameAnalytics() const
	{
		return WARRIORB_COLLECT_ANALYTICS ? bCollectGameAnalytics : false;
	}
	int64 GetUnixTimeModifiedCollectGameAnalytics() const { return UnixTimeModifiedCollectGameAnalytics; }
	bool WasCollectAnalyticsSetByUser() const { return UnixTimeModifiedCollectGameAnalytics > 0;  }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool CanWaitForAnalyticsToSend() const { return bWaitForAnalyticsToSend; }

	void GetAnalyticsPollTimers(double& WaitSeconds, double& ProcessEventsSeconds) const
	{
		WaitSeconds = AnalyticsWaitSeconds;
		ProcessEventsSeconds = AnalyticsProcessEventsSeconds;
	}

	//
	// Saves
	//

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsAutoSaveAndLoadEnabled() const { return bAutoSaveAndLoad; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsAutoBackupDeletedSavesEnabled() const { return bAutoBackupDeletedSaves; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Game")
	bool IsAutoBackupBeforeSaveEnabled() const { return bAutoBackupBeforeSave; }


	////////////////////////////////////////////////////
	// Display settings
	////////////////////////////////////////////////////

	// Moves to the corresponding monitor number, if it exists
	// MonitorNumber starts at 1. Corresponds to numbers from the display settings of your OS
	// If MonitorNumber = 0, it will use the primary display as default
	bool MoveToMonitor(int32 MonitorNumber, FMonitorInfo& OutMonitor);

	// Resets all video setting to current system settings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void ResetToCurrentDisplaySettings();

	// Apply only the display settings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void ApplyDisplaySettings(bool bCheckForCommandLineOverrides, bool bSaveSettings = true);

	// Sets the display settings to default
	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void SetDisplaySettingsToDefault();
	void ResetDisplaySettingsVariables();

	// Validates and resets bad display settings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void ValidateDisplaySettings();

	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void SetVolumetricFogEnabled(bool bEnabled) { bVolumetricFog = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void SetForceWeakHardwareOptimization(bool bForce)
	{
		bForceWeakHardwareOptimizations = bForce;
		OnForceWeakHardwareOptimizationsChanged.Broadcast();
	}

	UFUNCTION(BlueprintPure, Category = ">Settings|Display")
	bool IsVolumetricFogEnabled() const { return bVolumetricFog; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Display")
	bool IsWeakHardwareOptimizationForced() const { return bForceWeakHardwareOptimizations; }

	// Is the window mode in any fullscreen configuration? (aka not windowed)
	bool IsWindowModeFullScreen() const
	{
		const EWindowMode::Type WindowMode = GetFullscreenMode();
		return WindowMode == EWindowMode::Fullscreen || WindowMode == EWindowMode::WindowedFullscreen;
	}

	// Sets the Screen Resolution to be the resolution of the primary display
	void SetScreenResolutionToPrimaryDisplayResolution();


	// calls RunAndApplyHardwareBenchmark once if bShouldDoHardwareBenchmark was set to true
	void RunAndApplyHardwareBenchmarkIfRequested();

	// Runs and apply the hardware benchmark.
	void RunAndApplyHardwareBenchmark();

	// Own Implementation of ScreenPercentage info/set/get
	// This is similar to the ResolutionScale info. But r.ScreenPercentage seems to be the accurate than sg.ResolutionQuality.
	// sg.ResolutionQuality seem to map directly to r.ScreenPercentage but it is clamped to 100.
	// See https://answers.unrealengine.com/questions/118155/difference-between-resolution-quality-and-screen-p.html

	// Returns the current resolution scale and the range
	UFUNCTION(BlueprintPure, Category = ">Settings|Display")
	void GetScreenPercentageInformation(float& CurrentPercentNormalized, float& CurrentPercentValue, float& MinPercentValue, float& MaxPercentValue) const;

	// Gets the screen percentage from the console variables
	UFUNCTION(BlueprintPure, Category = ">Settings|Display")
	float GetScreenPercentage() const;

	// Sets the current screen percentage
	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void SetScreenPercentageValue(float NewScreenPercentageValue);

	// Sets the current screen percentage as a normalized 0..1 value between MinScaleValue and MaxScreenPercentage
	UFUNCTION(BlueprintCallable, Category = ">Settings|Display")
	void SetScreenPercentageNormalized(float NewScaleNormalized);

	// Makes sure the our settings are in the range we desire for our game
	void EnsureCustomQualityLimits()
	{
		// NOTE: do not force here, shrug
		SetScreenPercentageValue(GetScreenPercentage());
	}

	//
	// 0:low, 1:med, 2:high, 3:epic, 4:cinematic

	////////////////////////////////////////////////////
	// Audio settings
	////////////////////////////////////////////////////

	// Resets all audio settings to current system settings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void ResetToCurrentAudioSettings();

	// Apply only the audio settings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void ApplyAudioSettings(bool bCheckForCommandLineOverrides, bool bSaveSettings = true);

	// Sets the audio settings to default
	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetAudioSettingsToDefault();
	void ResetAudioSettingsVariables();

	/** Are the default audio setting used? */
	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	bool AreDefaultAudioSettingsUsed() const;

	// Validates and resets bad audio settings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void ValidateAudioSettings();

	UFUNCTION(BlueprintPure, Category = ">Settings|Audio")
	bool IsAudioMuted() const { return bMuteAudio; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetIsAudioMuted(bool bMute) { bMuteAudio = bMute; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Audio")
	float GetVolumeMaster() const { return VolumeMaster; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetVolumeMaster(float InVolume) { VolumeMaster = InVolume; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Audio")
	float GetVolumeMusic() const { return VolumeMusic; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetVolumeMusic(float InVolume) { VolumeMusic = InVolume; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Audio")
	float GetVolumeSFX() const { return VolumeSFX; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetVolumeSFX(float InVolume) { VolumeSFX = InVolume; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Audio")
	bool IsAudioMutedWhenUnfocused() const { return bMuteAudioWhenUnfocused; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetIsAudioMutedWhenUnfocused(bool bMute) { bMuteAudioWhenUnfocused = bMute; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Audio")
	bool IsVoiceGibberishMuted() const { return bMuteVoiceGibberish; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetIsVoiceGibberishMuted(bool bMute) { bMuteVoiceGibberish = bMute; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Audio")
	bool IsDialogueVoiceGibberishMuted() const { return bMuteDialogueVoiceGibberish; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Audio")
	void SetIsDialogueVoiceGibberishMuted(bool bMute) { bMuteDialogueVoiceGibberish = bMute; }


	////////////////////////////////////////////////////
	// Input settings
	////////////////////////////////////////////////////

	// Apply only the input settings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void ApplyInputSettings(bool bSaveSettings = true, bool bForce = false);

	// Resets the input settings used by the user to the default ones
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void SetInputSettingsToDefault();
	void ResetInputSettingsVariables();

	// Sets the default input bindings
	// NOTE: modifies both keyboard + gamepad
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void SetInputBindingsToDefault();

	// Sets the default input bindings for keyboard
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void SetKeyboardInputBindingsToDefault();

	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void SetGamepadInputBindingsToDefault();

	// Sets GamepadUIType
	// or bAutoDetectGamepadUIType = true
	bool TryAutoSetGamepadLayout();

	// Auto detect magic
	// Returns true if GamepadUI changed
	bool AutoSetGamepadLayoutFromConnectedGamepad();

	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	ESoGamepadLayoutType GetGamepadLayoutType() const;

	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void SetGamepadLayoutType(ESoGamepadLayoutType UIType) { GamepadUIType = UIType; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	bool IsAutoDetectGamepadLayout() const
	{
		// Disabled as this is too flaky
		return false; //bAutoDetectGamepadUIType;
	}

	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void SetAutoDetectGamepadLayout(bool InValue);

	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	bool UseKeyboardArrowPresetAsDefault() const { return bUseKeyboardArrowPresetAsDefault; }

	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void SetUseKeyboardArrowPresetAsDefault(bool InValue) { bUseKeyboardArrowPresetAsDefault = InValue; }

	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	FORCEINLINE bool HasActionNamesForGamepadKey(FName GamepadKeyName) const
	{
		return GamepadKeyToGameActionNameMap.Contains(GamepadKeyName);
	}

	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	FORCEINLINE bool HasUIActionNamesForGamepadKey(FName GamepadKeyName) const
	{
		return GamepadKeyToUIActionNameMap.Contains(GamepadKeyName);
	}

	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	const TArray<FName>& GetActionNamesForGamepadKey(FName GamepadKeyName) const
	{
		static const TArray<FName> EmptyArray;
		if (const TArray<FName> *ArrayPtr = GamepadKeyToGameActionNameMap.Find(GamepadKeyName))
			return *ArrayPtr;

		return EmptyArray;
	}

	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	const TArray<FName>& GetUIActionNamesForGamepadKey(FName GamepadKeyName) const
	{
		static const TArray<FName> EmptyArray;
		if (const TArray<FName> *ArrayPtr = GamepadKeyToUIActionNameMap.Find(GamepadKeyName))
			return *ArrayPtr;

		return EmptyArray;
	}

	// Refreshes the cached InputActionMappings and InputAxisMappings from the InputSettings
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void RefreshInputSettingsMappings();

	// Set the input settings to the provided preset
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	bool SetInputToPreset(const FSoInputPreset& Preset);

	// Helper function
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	bool SetInputToPresetKeyboardArrows();

	// Rebind OldKeyMapping to the NewInputChord
	bool RebindInputKeyToInputChord(const FInputActionKeyMapping& OldKeyMapping, const FInputChord& NewInputChord, bool bCheckExistingBinding, bool bUnbindExisting);

	// Rebind OldKeyMapping to the NewKeyMapping
	bool RebindInputKey(const FInputActionKeyMapping& OldKeyMapping, const FInputActionKeyMapping& NewKeyMapping, bool bCheckExistingBinding, bool bUnbindExisting);

	// // Add from vanilla, no old modified
	// bool AddInputBindingForActionName(FName ActionName, const FInputActionKeyMapping& NewKeyMapping, bool bCheckExistingBinding, bool bUnbindExisting)
	// {
	// 	FInputActionKeyMapping OldKeyMapping;
	// 	OldKeyMapping.ActionName = ActionName;
	// 	return RebindInputKey(OldKeyMapping, NewKeyMapping, bCheckExistingBinding, bUnbindExisting);
	// }

	// Reverse of RebindInputKey
	bool UnbindInputActionMapping(const FInputActionKeyMapping& OldKeyMapping);
	bool UnbindKeyboardInputActionMapping(const FInputActionKeyMapping& OldKeyMapping);
	bool UnbindAllKeyboardInputsForActionName(FName ActionName);

	// Unbinds all the ActionMappings
	void UnbindInputActionMappings(const TArray<FInputActionKeyMapping>& ActionMappings);

	// Gets the default FInputActionKeyMapping for the ModifiedKeyMapping,
	// NOTE: same as GetDefaultInputActionMappingsForModified only this is public facing and should return only one result
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	bool GetDefaultInputActionMapping(const FInputActionKeyMapping& ModifiedMapping, FInputActionKeyMapping& OutDefault) const;

	// Rebinds the ModifiedMapping to the default one if it can.
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	bool SetInputActionMappingToDefault(const FInputActionKeyMapping& ModifiedMapping, bool bUnbindExisting, bool bUsePreset = false);

	// Is the InputChord already used?
	// If bOnlyForTheActionCategory = true then we return the duplicates only for the category that corresponds for ForActionName
	bool IsInputChordAlreadyUsed(FName ForActionName, const FInputChord& InputChord, ESoInputActionCategoryType FilterCategory, TArray<FInputActionKeyMapping>& OutByActionMappings) const;

	// Same as IsInputChordAlreadyUsed only does it with a key
	bool IsInputKeyAlreadyUsed(FName ForActionName, FKey Key, ESoInputActionCategoryType FilterCategory, TArray<FInputActionKeyMapping>& OutByActionMappings) const
	{
		const FInputChord InputChord{ Key };
		return IsInputChordAlreadyUsed(ForActionName, InputChord, FilterCategory, OutByActionMappings);
	}

	// Check if FirstActionName and SecondActionName have the same assigned key for the specified input device (either keyboard or gamepad)
	bool DoActionNamesHaveTheSameKey(FName FirstActionName, FName SecondActionName, bool bIsKeyboard);

	// Gets the InActionName action key mappings array. This is O(1) compared to UInputSettings O(n)
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	const TArray<FInputActionKeyMapping>& GetInputActionMappingsForActionName(FName ActionName) const;

	// Helper method: Gets only the keyboard input action names
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	TArray<FInputActionKeyMapping> GetKeyboardInputActionMappingsForActionName(FName ActionName, bool bIncludeUnbound) const;

	// Helper method: Gets only the gamepad input action names
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	TArray<FInputActionKeyMapping> GetGamepadInputActionMappingsForActionName(FName ActionName, bool bIncludeUnbound) const;

	// Gets the InAxisName action mappings array. This is O(1) compared to UInputSettings O(n)
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	const TArray<FInputAxisKeyMapping>& GetInputAxisMappingForAxisName(FName AxisName) const;

	/** Are the default user input settings used? */
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	bool AreDefaultInputSettingsUsed() const;

	/** Is the provided KeyMapping a default one? From Config/DefaultInput.ini  */
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	bool IsInputActionMappingDefault(const FInputActionKeyMapping& CheckKeyMapping, bool bUsePreset = false) const;

	/** Is the provided AxisMapping a default one? From Config/DefaultInput.ini  */
	UFUNCTION(BlueprintPure, Category = ">Settings|Input")
	bool IsInputAxisMappingDefault(const FInputAxisKeyMapping& CheckAxisMapping) const;

	/** Rebuilds all the input key maps of all player inputs */
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void RebuildInputKeymapsForAllPlayers() const;

	/** Rebuilds only the input key map of the PlayerInput  */
	UFUNCTION(BlueprintCallable, Category = ">Settings|Input")
	void RebuildInputKeymapsForPlayerInput(UPlayerInput* PlayerInput) const;

	UPROPERTY(BlueprintAssignable, Category = ">Settings|Events")
	FSoSettingsNotify OnInputSettingsApplied;

	UPROPERTY(BlueprintAssignable, Category = ">Settings|Events")
	FSoSettingsNotify OnForceWeakHardwareOptimizationsChanged;

	UPROPERTY(BlueprintAssignable, Category = ">Settings|Events")
	FSoSettingsNotify OnDisplayEnemyHealthBarChanged;

	UPROPERTY(BlueprintAssignable, Category = ">Settings|Events")
	FSoSettingsNotify OnCharacterSkinChanged;

protected:
	// UGameUserSettings Interface
	bool IsVersionValid() override;
	void UpdateVersion() override;

	// Own methods

	// Getter so that we only have one place where we set the default audio settings
	static const FSoAudioSettings& GetDefaultAudioSettings()
	{
		static const FSoAudioSettings Default{};
		return Default;
	}

	////////////////////////////////////////////////////
	// Input settings
	////////////////////////////////////////////////////

	// Generates the map for InputActionMappings
	TMap<FName, TArray<FInputActionKeyMapping>> GenerateInputActionMappings() const;

	void EnsureActionNamesAreInSyncIfModified(const FSoInputActionKeyMapping& ActionMapping);
	void EnsureUnbindingDoesNotBreakGame(const FSoInputActionKeyMapping& ActionMapping);

	// Gets the modified key mappings for the default key mapping
	TArray<FSoInputActionKeyMapping> GetModifiedInputActionMappingsForDefault(const FInputActionKeyMapping& DefaultKeyMapping) const;

	// Gets the default key mappings for the modified key mapping
	TArray<FInputActionKeyMapping> GetDefaultInputActionMappingsForModified(const FInputActionKeyMapping& ModifiedKeyMapping) const;

	// Gets the modified axis mappings for the default axis mappings
	TArray<FSoInputAxisKeyMapping> GetModifiedInputAxisMappingsForDefault(const FInputAxisKeyMapping& DefaultAxisMapping) const;

	// Deduce the axis mapping we need to modify from the modified ActionMapping. False if we there is nothing to deduce
	bool GetModifiedInputAxisMappingForActionMapping(const FSoInputActionKeyMapping& ActionMapping, FName AxisNameToModify, FSoInputAxisKeyMapping& OutAxisMapping) const;

	/** Adds the ActionMapping into the ModifiedInputActionMappings.  */
	bool AddModifiedInputActionMapping(const FSoInputActionKeyMapping& ActionMapping);

	/** Adds the AxisMapping into the ModifiedInputAxisMappings. */
	bool AddModifiedInputAxisMapping(const FSoInputAxisKeyMapping& AxisMapping);

	/** Removes all the modified action mappings that have the default as DefaultKeyMapping */
	void RemoveModifiedInputActionMappingsForActionName(FName ActionName);
	void RemoveModifiedInputActionMappingsForDefault(const FInputActionKeyMapping& DefaultKeyMapping);

	/** Removes all the modified axis mappings that have the default as DefaultAxisMapping */
	void RemoveModifiedInputAxisMappingsForDefault(const FInputAxisKeyMapping& DefaultAxisMapping);

	// Converts InputActionMappings into a TArray
	void GetAllInputActionMappings(TArray<FInputActionKeyMapping>& OutMappings) const;

	// Converts InputAxisMappings into a TArray
	void GetAllInputAxisMappings(TArray<FInputAxisKeyMapping>& OutMappings) const;

	// Checks if the ActionMapping is valid
	ESoInputIntegrity ValidateModifiedInputActionMapping(const FSoInputActionKeyMapping& ActionMapping, bool bLog = true) const;

	// Checks if the ActionMapping is valid;
	ESoInputIntegrity ValidateModifiedInputAxisMapping(const FSoInputAxisKeyMapping& AxisMapping, bool bLog = true) const;

	// Checks if the modified input settings are valid if they are not it tries to "repair" them
	void CheckAndRepairModifiedInputSettings();

protected:
	//
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// NOTE: The defaults are set in the construct and that calls a general reset function
	// This is because we want to modify it in only one place instead of two
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	////////////////////////////////////////////////////
	// Video Demo settings
	////////////////////////////////////////////////////

	// If this is enabled we try to disables the main menu
	UPROPERTY(Config)
	bool bVideoDemoDisableMainMenu;

	// Should we start video loop
	UPROPERTY(Config)
	bool bVideoDemoStartVideoOnIdle;

	// If <= 0 then this is disabled
	UPROPERTY(Config)
	float VideoDemoIdleTimeSecondsStartVideo;

	////////////////////////////////////////////////////
	// Game settings
	////////////////////////////////////////////////////

	UPROPERTY(Config)
	bool bDisplayFPS;

	UPROPERTY(Config)
	bool bDisplayTime;

	UPROPERTY(Config)
	bool bDisplayDamageTexts;

	UPROPERTY(Config)
	bool bDisplayEnemyHealthBar;

	UPROPERTY(Config)
	bool bDisplayFloatingVOLines;

	UPROPERTY(Config)
	bool bDisplayStat;

	// Should we pause the game when the game is in the background
	// Other options is to idle: Set t.IdleWhenNotForeground=1
	UPROPERTY(Config)
	bool bPauseGameWhenUnfocused;

	// Should the game pause when we are idling over IdleTimeSecondsBeforeGamePause
	UPROPERTY(Config)
	bool bPauseGameOnIdle;

	// If the user reaches this idle time then the game pauses
	// If <= 0 then this is disabled
	UPROPERTY(Config)
	float IdleTimeSecondsBeforeGamePause;

	// Started game for the first time.
	// NOTE: Saved/Config/<Platform>/GameUserSettings does not work properly
	UPROPERTY(Config)
	bool bIsFirstTimeGameStarted;

	// Can we use the custom maps?
	UPROPERTY(Config)
	bool bCustomMapsEnabled;

	// Screen percentage screen var
	const TCHAR* ScreenPercentageVariable = TEXT("r.ScreenPercentage");

	//
	// Language selection - only first time
	//

	UPROPERTY(Config)
	bool bLanguageAlreadySelected;

	//
	// Game Speed
	//

	// Keep track of the game speed version
	UPROPERTY(config)
	uint32 GameSpeedVersion = 0;

	// The current game speed multiplier
	UPROPERTY(Config)
	float GameSpeed;

	//
	// Brightness
	//

	// The current screen brightness
	UPROPERTY(Config)
	float Brightness;

	//
	// Gamepad vibration
	//

	// The current gamepad vibration scale
	UPROPERTY(Config)
	float VibrationScale;

	//
	// Analytics
	//

	// Does the user allow us to collect anonymous game analytics?
	UPROPERTY(Config)
	bool bCollectGameAnalytics;

	// NOTE: the following settings are not not configured in the settings, so no functions in this class affects this, only
	// changing this in the settings file can actually change these value.

	// Unix time at which, bCollectGameAnalyticswas set to true
	// If 0 - then it was set in the default config
	// If INDEX_NONE - then it means the user did not accept anything
	UPROPERTY(Config)
	int64 UnixTimeModifiedCollectGameAnalytics;

		// Should the game wait at the end for all analytics to send?
	UPROPERTY(Config)
	bool bWaitForAnalyticsToSend;

	// For analytics wait thread. Default is 1.
	UPROPERTY(Config)
	double AnalyticsWaitSeconds;

	// For Analytics process events thread. Default is 8.
	UPROPERTY(Config)
	double AnalyticsProcessEventsSeconds;

	//
	// BounceMode
	//

	UPROPERTY(Config)
	ESoBounceModeType BounceModeType = ESoBounceModeType::Always;

	// Value used if for non custom events
	UPROPERTY(Config)
	ESoCharacterSkinType CharacterSkinType = ESoCharacterSkinType::Classic;

	// Used to save CharacterSkinType if we are overriding it for a custom event
	UPROPERTY(Config)
	ESoCharacterSkinType PreviousCharacterSkinType = ESoCharacterSkinType::Classic;

	// Keep track of set times
	UPROPERTY(Config)
	FDateTime ModifiedDateTimePreviousCharacterSkinType = FDateTime::MinValue();
	UPROPERTY(Config)
	FDateTime ModifiedDateTimeCharacterSkinType = FDateTime::MinValue();

	//
	// Saves
	//

	// Auto save and load in game.
	UPROPERTY(Config)
	bool bAutoSaveAndLoad;

	// Create backup files for deleted files (before they are deleted)
	UPROPERTY(Config)
	bool bAutoBackupDeletedSaves;

	// Create a backup file before saving each save file
	UPROPERTY(Config)
	bool bAutoBackupBeforeSave;


	//
	// Game speed
	//

	////////////////////////////////////////////////////
	// Display settings
	////////////////////////////////////////////////////
	UPROPERTY(Config)
	bool bVolumetricFog;

	UPROPERTY(Config)
	bool bForceWeakHardwareOptimizations;

	////////////////////////////////////////////////////
	// Audio settings
	// NOTE: Change defaults in C++ in GetDefaultAudioSettings()
	////////////////////////////////////////////////////
	UPROPERTY(Config)
	bool bMuteAudio;

	UPROPERTY(Config)
	float VolumeMaster;

	UPROPERTY(Config)
	float VolumeMusic;

	UPROPERTY(Config)
	float VolumeSFX;

	// Mutes the audio when the game is in background
	UPROPERTY(Config)
	bool bMuteAudioWhenUnfocused;

	// Should we mute the gibberish voices?
	UPROPERTY(Config)
	bool bMuteVoiceGibberish;

	UPROPERTY(Config)
	bool bMuteDialogueVoiceGibberish;

	////////////////////////////////////////////////////
	// Input settings
	////////////////////////////////////////////////////

	// We automatically detect the gamepad UI type instead of relying the manual value set by the user GamepadUIType.
	UPROPERTY(Config)
	bool bAutoDetectGamepadUIType;

	// Was the keyboard preset already selected?
	UPROPERTY(Config)
	bool bKeyboardPresetAlreadySelected;

	// Does the user prefer the arrow keys preset for keyboard?
	UPROPERTY(Config)
	bool bUseKeyboardArrowPresetAsDefault;

	// Default UI for the gamepad to display
	UPROPERTY(Config)
	ESoGamepadLayoutType GamepadUIType;

	// Modified action mappings by this user
	UPROPERTY(Config)
	TArray<FSoInputActionKeyMapping> ModifiedInputActionMappings;

	// Modified axis mappings by this user
	UPROPERTY(Config)
	TArray<FSoInputAxisKeyMapping> ModifiedInputAxisMappings;

	// All action mappings, combines UInputSettings.ActionMappings + ModifiedInputActionMappings so that we access it in O(1);
	// Key: ActionName
	// Value: All the key mappings
	TMap<FName, TArray<FInputActionKeyMapping>> InputActionMappings;

	// All axis mappings, combines UInputSettings.AxisMappings + ModifiedInputAxisMappings so that we access it in O(1);
	// Key: ActionName
	// Value: All the axis mappings
	TMap<FName, TArray<FInputAxisKeyMapping>> InputAxisMappings;

	/**
	 * Map that points from the gamepad key name to the action name.
	 * NOTE: only includes game action names
	 * Key: Gamepad Key Name
	 * Value: Array of ActionNames
	 */
	TMap<FName, TArray<FName>> GamepadKeyToGameActionNameMap;

	/**
	* Map that points from the gamepad key name to the action name.
	* NOTE: only includes UI action names
	* Key: Gamepad Key Name
	* Value: Array of action names ActionName
	*/
	TMap<FName, TArray<FName>> GamepadKeyToUIActionNameMap;

	////////////////////////////////////////////////////
	// Other
	////////////////////////////////////////////////////


	// See RunAndApplyHardwareBenchmarkIfRequested
	bool bShouldDoHardwareBenchmark = false;

	// Reference Actor used to get the UWorld.
	UPROPERTY(Transient)
	UObject* WorldContextObject = nullptr;

	bool bBlockConfigSave = false;

	bool bCacheSaveRequests = false;
	bool bAnySaveRequestCached = false;

public:
	// Empties
	static const TArray<FInputActionKeyMapping> EmptyInputActionKeysArray;
	static const TArray<FInputAxisKeyMapping> EmptyInputAxisKeysArray;

	// Maximum/Minimum single axis scale for render resolution
	static constexpr float MaxScreenPercentage = 200.0f;
	static constexpr float MinScreenPercentage = 75.f;

	// Brightness limits, Original limits are on UEngine::HandleGammaCommand
	static constexpr float MinBrightness = 1.2f;
	static constexpr float MaxBrightness = 3.2;
	static constexpr float DefaultBrightness = 2.2f;

	// Gamepad vibration
	static constexpr float MinVibrationScale = 0.f;
	static constexpr float MaxVibrationScale = 2.f;
	static constexpr float DefaultVibrationScale = 1.f;

	// Game Speed
	static constexpr float DefaultGameSpeed = 1.f;
};
