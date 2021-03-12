// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoUISettingsBase.h"
#include "Settings/SoDisplaySettingsTypes.h"

#include "SoUISettingsDisplay.generated.h"

class USoUIButtonArray;
class USoUIScrollingButtonArray;
class USoUICheckbox;
class USoUISlider;
class UTextBlock;
class USoUICommandTooltipArray;
class USoUIConfirmDisplaySettings;

// All the line options for the USoUISettingsDisplay
UENUM(BlueprintType)
enum class ESoUISettingsDisplayLine : uint8
{
	None = 0,

	DisplayMode,
	AspectRatio,
	Resolution,
	ResolutionScale,
	VSync,
	FrameLimit,
	Shadows,
	Textures,
	Effects,
	PostEffects,
	AntiAliasing,
	VolumetricFog,
	ForceWeakHardwareOptimization,
};



// Enum representing the epic settings
// 0:low, 1:med, 2:high, 3:epic, 4:cinematic
enum class ESoUISettingsEpicQualityOption : uint8
{
	Low = 0,
	Medium = 1,
	High = 2,
	Epic = 3,
	// Cinematic = 4,

	Num
};


/**
 * Control video/display related settings
 */
UCLASS()
class SORB_API USoUISettingsDisplay : public USoUISettingsBase
{
	GENERATED_BODY()

protected:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// ISoUIEventHandler Interface
	//
	void Open_Implementation(bool bOpen) override;

	//
	// USoUISettingsBase Interface
	//
	void CreateChildrenAndSetTexts(bool bFromSynchronizeProperties = false) override;
	bool HandleOnUICommand(ESoUICommand Command) override;
	void OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget) override;
	bool NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput = false) override;
	bool IsValidLineIndex(int32 LineIndex) const override
	{
		return Super::IsValidLineIndex(LineIndex) && LineOptions.IsValidIndex(LineIndex);
	}
	void OnPostSetSelectedIndex(int32 OldIndex, int32 NewIndex) override;
	void RefreshButtonImagesTooltips(bool bForceShowAll = false) override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

	//
	// Own methods
	//

	// Gets the array of texts depending on the quality options array providedation
	static TArray<FText> GetTextsForQualityOptions(const TArray<ESoUISettingsEpicQualityOption>& InQualityOptions);

	// Navigate left or right on the selected line

	// Apply/Save the user settings from the selected line options.
	void ApplyUserSettingsFromSelectedOptions();

	// Reset settings to the system settings and updates selected options to match the user settings one.
	void ResetSelectedOptionsToUserSettings();

	// Enable/Disable some lines depending on the options set.
	void UpdateLinesVisibility();

	// Updates the lines (of the selected options) with the status from the UserSettings
	void UpdateLinesFromUserSettings();

	// Update the resolution lines options from the selected options.
	// NOTE that these values may or may not be user settings.
	void UpdateResolutionLineOptionsFromSelectedOptions();

	// Updates the Resolution line  options depending on the CurrentAspectRatio and CurrentDisplayMode
	// This creates new children for the Resolution line + selects a sane default.
	void UpdateResolutionLineOptions(const FSoDisplayResolution& CurrentResolution,
		ESoDisplayAspectRatio CurrentAspectRatio,
		EWindowMode::Type CurrentDisplayMode, const FIntPoint& MaxResolution);

	// Updates the frame limit options and the frame limit from the use settings
	void UpdateFrameLimitFromUserSettings();

	// Updates the resolution scale options and the selected resolution scale from the user settings.
	void UpdateResolutionScaleFromUserSettings();

	// Gets a valid index in the GeneralQualityOptions Array
	int32 GetIndexForGeneralQuality(int32 Quality) const
	{
		return GeneralQualityOptions.IsValidIndex(Quality) ? Quality : 0;
	}

	// Gets a valid index in the LimitedHighQualityOptions for the supplied Quality.
	// Modulo works here because it remaps Quality 2 -> 0 (index) and 3 -> 1
	int32 GetIndexForLimitedHighQuality(int32 Quality) const
	{
		return Quality % LimitedHighQualityOptions.Num();
	}

	// Gets a valid index in the LimitedMediumQualityOptions for the supplied Quality
	int32 GetIndexForLimitedMediumQuality(int32 Quality) const
	{
		// Maps:
		// 3 -> 2
		// 2 -> 1
		// 1 -> 0
		return Quality > 0 ? (Quality - 1) % LimitedMediumQualityOptions.Num() : 0;
	}

	// Gets the current value of the Effects options selected
	// Returns ESoUISettingsEpicQualityOption::Num is the option is invalid
	ESoUISettingsEpicQualityOption GetEffectsQualityOption() const;
	ESoUISettingsEpicQualityOption GetShadowQualityOption() const;

	// Apply/Reset settings buttons pressed
	void OnCommandApplySettings();
	void OnCommandResetSettings();

	// Resolution/Window Mode changed
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", meta = (DisplayName = "OnDisplayModeOrResolutionChanged"))
	void ReceiveOnDisplayModeOrResolutionChanged();
	void OnDisplayModeOrResolutionChanged();

	UFUNCTION()
	void OnResolutionScaleValueChanged(float NewValueNormalized);

	UFUNCTION()
	void OnScrollingArrayChildChanged(int32 PreviousChild, int32 NewChild);

	bool IsConfirmMode() const;
	bool OpenConfirmPanel();

	UFUNCTION()
	void OnConfirmPanelOpenChanged(bool bPanelOpened);

	// Reload the default settings to the current lines options
	void ReloadDefaultSettings();

	// Did the default settings change? default is what is displayed currently
	bool DidDefaultSettingsChange() const;

	// Did defaults settings change for this line?
	bool DidDefaultSettingChangeForLine(int32 LineIndex) const;

	// Gets the value of the default setting for this line
	int32 GetDefaultSettingForLine(int32 LineIndex) const;
	int32 GetDefaultSettingForLine(ESoUISettingsDisplayLine LineType) const
	{
		for (int32 LineIndex = 0; LineIndex < LineOptions.Num(); LineIndex++)
			if (LineOptions[LineIndex] == LineType)
				return GetDefaultSettingForLine(LineIndex);

		return INDEX_NONE;
	}

protected:
	// Lines:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* DisplayMode = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* AspectRatio = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* Resolution = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISlider* ResolutionScale = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* VSync = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* FrameLimit = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* Shadows = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* Textures = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* Effects = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* PostEffects = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* Antialiasing = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* VolumetricFog = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* ForceWeakHardwareOptimization = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* CurrentTooltipText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	USoUIConfirmDisplaySettings* ConfirmDisplaySettings;

	// Set in UMG
	UPROPERTY(EditAnywhere, Category = ">Lines")
	TMap<ESoUISettingsDisplayLine, FText> LinesTooltips;

	// ResolutionScale slider moved/changed
	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	bool bResolutionScaleChanged = false;

	// Default Settings, initialized on Menu Open. Used to keep track if any settings changed.
	// Map from Line Index => int (can be selected element index, true false, etc depending on the context)
	TArray<int32> DefaultSettings;

	// Options for the different settings.
	// We use arrays for ordering because the order might differ from it's enum. Also it makes reordering the options
	// very easily, just by changing one of the lines below.
	// For Example we might want to change DisplayMode to be: Fullscreen, Windowed, WindowedFullscreen
	// Each of the array sizes here should match the number of buttons in the button array.

	// Map Selected Button Index => EWindowMode::Type. Usually they are one and the same.
	const TArray<EWindowMode::Type> DisplayModeOptions = {
		EWindowMode::Fullscreen,
		EWindowMode::WindowedFullscreen,
		EWindowMode::Windowed
	};

	// Map Selected Button Index => ESoDisplayAspectRatio
	const TArray<ESoDisplayAspectRatio> AspectRatioOptions = {
		ESoDisplayAspectRatio::Ratio_4_3,
		ESoDisplayAspectRatio::Ratio_16_9,
		ESoDisplayAspectRatio::Ratio_16_10
	};

	// All the FrameLimit Preset options
	const TArray<int32> FrameLimit_PresetOptions = {
		30, 60, 120, 144, 160, 165, 180, 200, 240,
		INDEX_NONE // unlimited
	};

	// The used frame limit options
	// Maps Selected Button Index => frame rate limit
	TArray<int32> FrameLimitOptions;

	// All the 4:3 preset resolution options
	const TArray<FSoDisplayResolution> Resolution_4_3_PresetOptions = {
		FSoDisplayResolution(1280, 960, ESoDisplayAspectRatio::Ratio_4_3),
		FSoDisplayResolution(1400, 1050, ESoDisplayAspectRatio::Ratio_4_3),
		FSoDisplayResolution(1440, 1080, ESoDisplayAspectRatio::Ratio_4_3),
		FSoDisplayResolution(1600, 1200, ESoDisplayAspectRatio::Ratio_4_3),
		FSoDisplayResolution(1856, 1392, ESoDisplayAspectRatio::Ratio_4_3)
	};

	// All the 16:9 preset resolution options
	const TArray<FSoDisplayResolution> Resolution_16_9_PresetOptions = {
		FSoDisplayResolution(1280, 720, ESoDisplayAspectRatio::Ratio_16_9),
		FSoDisplayResolution(1366, 768, ESoDisplayAspectRatio::Ratio_16_9),
		FSoDisplayResolution(1600, 900, ESoDisplayAspectRatio::Ratio_16_9),
		FSoDisplayResolution(1920, 1080, ESoDisplayAspectRatio::Ratio_16_9),
		FSoDisplayResolution(2560, 1440, ESoDisplayAspectRatio::Ratio_16_9),
		FSoDisplayResolution(3840, 2160, ESoDisplayAspectRatio::Ratio_16_9)
	};

	// All the 16:10 preset resolution options
	const TArray<FSoDisplayResolution> Resolution_16_10_PresetOptions = {
		FSoDisplayResolution(1280, 800, ESoDisplayAspectRatio::Ratio_16_10),
		FSoDisplayResolution(1440, 900, ESoDisplayAspectRatio::Ratio_16_10),
		FSoDisplayResolution(1680, 1050, ESoDisplayAspectRatio::Ratio_16_10),
		FSoDisplayResolution(1920, 1200, ESoDisplayAspectRatio::Ratio_16_10),
		FSoDisplayResolution(2560, 1600, ESoDisplayAspectRatio::Ratio_16_10)
	};

	// The used Resolution Options
	// Map Selected Button Index => FSoDisplayResolution
	TArray<FSoDisplayResolution> ResolutionOptions;

	// Map Selected Button Index => ESoUISettingsEpicQualityOptions
	const TArray<ESoUISettingsEpicQualityOption> GeneralQualityOptions = {
		ESoUISettingsEpicQualityOption::Low,
		ESoUISettingsEpicQualityOption::Medium,
		ESoUISettingsEpicQualityOption::High,
		ESoUISettingsEpicQualityOption::Epic
	};
	const TArray<ESoUISettingsEpicQualityOption> LimitedMediumQualityOptions = {
		ESoUISettingsEpicQualityOption::Medium,
		ESoUISettingsEpicQualityOption::High,
		ESoUISettingsEpicQualityOption::Epic
	};
	const TArray<ESoUISettingsEpicQualityOption> LimitedHighQualityOptions = {
		ESoUISettingsEpicQualityOption::High,
		ESoUISettingsEpicQualityOption::Epic
	};

	// Map Selected Line Index => ESoUISettingsDisplayLine (line type)
	// NOTE for this you also need to change the order in the UMG Blueprint Widget of the USoUIButtonArray.
	const TArray<ESoUISettingsDisplayLine> LineOptions = {
		ESoUISettingsDisplayLine::DisplayMode,
		ESoUISettingsDisplayLine::AspectRatio,
		ESoUISettingsDisplayLine::Resolution,
		ESoUISettingsDisplayLine::ResolutionScale,
		ESoUISettingsDisplayLine::VSync,
		ESoUISettingsDisplayLine::FrameLimit,
		ESoUISettingsDisplayLine::Shadows,
		ESoUISettingsDisplayLine::Textures,
		ESoUISettingsDisplayLine::Effects,
		ESoUISettingsDisplayLine::VolumetricFog,
		ESoUISettingsDisplayLine::ForceWeakHardwareOptimization,
		ESoUISettingsDisplayLine::PostEffects,
		ESoUISettingsDisplayLine::AntiAliasing
	};

	// UI shortcuts for pressing apply/reset OR confirm/cancel
	static constexpr ESoUICommand UICommand_ApplySettings = ESoUICommand::EUC_Action0;
	static constexpr ESoUICommand UICommand_ResetSettings = ESoUICommand::EUC_Action1;
};
