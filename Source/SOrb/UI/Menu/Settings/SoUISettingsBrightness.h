// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoUISettingsBase.h"

#include "SoUISettingsBrightness.generated.h"


class USoGameSettings;
class USoUISlider;

class USoUICommandTooltipArray;

// All the line options for the USoUISettingsBrightness
enum class ESoUISettingsBrightnessLine : uint8
{
	None = 0,
	Brightness
};

/**
 * Controls the brightness of the game
 */
UCLASS()
class SORB_API USoUISettingsBrightness : public USoUISettingsBase
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
	bool NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput = false) override;
	bool IsValidLineIndex(int32 LineIndex) const override
	{
		return Super::IsValidLineIndex(LineIndex) && LineOptions.IsValidIndex(LineIndex);
	}
	void RefreshButtonImagesTooltips(bool bForceShowAll = false) override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

	//
	// Own Methods
	//
	void ResetSelectedOptionsToCurrentSettings();
	bool DidInitialValuesChange() const;
	bool CanRevertToDefault() const;

	void OnCommandApply();
	void OnCommandResetToDefault();

	UFUNCTION()
	void OnBrightnessValueChanged(float NewValueNormalized);

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISlider* Brightness = nullptr;

	// Map Selected Line Index => ESoUISettingsBrightnessLine (line type)
	const TArray<ESoUISettingsBrightnessLine> LineOptions = {
		ESoUISettingsBrightnessLine::Brightness
	};

	UPROPERTY()
	float InitialBrightnessNormalized = INDEX_NONE;

	static constexpr ESoUICommand UICommand_Apply = ESoUICommand::EUC_Action0;
	static constexpr ESoUICommand UICommand_ResetToDefault = ESoUICommand::EUC_Action1;
};
