// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoUISettingsBase.h"

#include "SoUISettingsAudio.generated.h"

class USoUICheckbox;
class USoUIButton;
class USoUISlider;
class USoUICommandTooltipArray;
class UTextBlock;

// All the line options for the USoUISettingsAudio
UENUM(BlueprintType)
enum class ESoUISettingsAudioLine : uint8
{
	None = 0,

	MuteAudio,
	MasterAudio,
	SoundEffectsAudio,
	MusicAudio,
	MuteAudioWhenUnfocused,
	MuteVoiceGibberish,
	MuteDialogueVoiceGibberish,
};

/**
 * Control audio/music related settings
 */
UCLASS()
class SORB_API USoUISettingsAudio : public USoUISettingsBase
{
	GENERATED_BODY()
	typedef USoUISettingsAudio Self;

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

	bool OnCommandRevertToDefaults();

	// Reset settings to the system settings and updates selected options to match the user settings one.
	void ResetSelectedOptionsToUserSettings();

	// Enable/Disable some lines depending on the options set.
	void UpdateLinesVisibility();

	// Did the default settings change?
	bool AreDefaultAudioSettingsUsed() const;

	UFUNCTION()
	void OnMasterValueChanged(float NewValueNormalized);

	UFUNCTION()
	void OnSoundEffectsValueChanged(float NewValueNormalized);

	UFUNCTION()
	void OnMusicValueChanged(float NewValueNormalized);

protected:
	// Mute the sound
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* MuteAudio = nullptr;

	// Controls the overall sound
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISlider* MasterAudio = nullptr;

	// Controls Sound volume
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISlider* SoundEffectsAudio = nullptr;

	// Controls Music volume
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISlider* MusicAudio = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* MuteAudioWhenUnfocused = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* MuteVoiceGibberish = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* MuteDialogueVoiceGibberish = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* CurrentTooltipText = nullptr;

	// Set in UMG
	UPROPERTY(EditAnywhere, Category = ">Lines")
	TMap<ESoUISettingsAudioLine, FText> LinesTooltips;

	// Map Selected Line Index => ESoUISettingsAudioLine (line type)
	// NOTE for this you also need to change the order in the UMG Blueprint Widget of the USoUIButtonArray.
	const TArray<ESoUISettingsAudioLine> LineOptions = {
		ESoUISettingsAudioLine::MuteAudio,
		ESoUISettingsAudioLine::MasterAudio,
		ESoUISettingsAudioLine::MusicAudio,
		ESoUISettingsAudioLine::SoundEffectsAudio,
		ESoUISettingsAudioLine::MuteAudioWhenUnfocused,
		ESoUISettingsAudioLine::MuteVoiceGibberish,
		ESoUISettingsAudioLine::MuteDialogueVoiceGibberish
	};

	// UI shortcut to revert to defaults
	static constexpr ESoUICommand UICommand_RevertToDefaults = ESoUICommand::EUC_Action1;
};
