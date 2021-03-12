// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "SoUISettingsBase.h"
#include "Settings/Input/SoInputNames.h"
#include "Settings/Input/SoInputSettingsTypes.h"
#include "Basic/Helpers/SoPlatformHelper.h"

#include "SoUISettingsController.generated.h"

class USoUIButtonArray;
class UPanelWidget;
class USoUICommandTooltipArray;
class USoUIScrollingButtonArray;
class USoUICommandTooltip;
class USoGameSettings;
class USoUISettingsControllerRemap;
class UBackgroundBlur;
class USoUICheckbox;
class UWidgetSwitcher;

// All the line options for the USoUISettingsController
enum class ESoUISettingsControllerLine : uint8
{
	Layout = 0,
	AutoDetectControllerType,
	Vibration
};

/**
 * Shows the controller input options
 */
UCLASS()
class SORB_API USoUISettingsController : public USoUISettingsBase
{
	GENERATED_BODY()
	typedef USoUISettingsController Self;

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
	bool CanBeInterrupted_Implementation() const override;

	//
	// USoUISettingsBase Interface
	//
	void CreateChildrenAndSetTexts(bool bFromSynchronizeProperties = false) override;
	bool HandleOnUICommand(ESoUICommand Command) override;
	virtual bool ShouldChildHandleCommands() const;
	void OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget) override;
	bool NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput = false) override;
	bool IsValidLineIndex(int32 LineIndex) const override
	{
		return Super::IsValidLineIndex(LineIndex) && LineOptions.IsValidIndex(LineIndex);
	}
	void RefreshButtonImagesTooltips(bool bForceShowAll = false) override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

	void UpdateWidgetsVisibilities() override;

	//
	// Own methods
	//

	void RefreshLineOptions();
	void OnCommandRemapController();

	// Reset settings to the system settings and updates selected options to match the user settings one.
	void ResetSelectedOptionsToUserSettings();
	void UpdateUI();

	ESoGamepadLayoutType GetSelectedGamepadLayoutType() const;

	void InitializeAllGamepadButtons();
	void InitializeGamepadButton(USoUICommandTooltip* Button, FName GamepadKeyName, bool bAlsoCheckUICommand = false);
	void InitializeEmptyGamepadButton(USoUICommandTooltip* Button, FName GamepadKeyName);

	void OverrideGamepadButtonTextWith(USoUICommandTooltip* Button, FText Text);
	void OverrideGamepadButtonImageWithTexturesOf(USoUICommandTooltip* Button, FName GamepadKeyName);

	void UpdateAllImagesFromSelectedDeviceType();
	void UpdateImageFromSelectedDeviceType(USoUICommandTooltip* Button, ESoInputDeviceType DeviceType);

	// Enable/Disable some lines depending on the options set.
	void UpdateLinesVisibility();

	UFUNCTION()
	void OnControllerPlatformChildChanged(int32 PreviousChild, int32 NewChild);

	UFUNCTION()
	void OnVibrationValueChanged(float NewValueNormalized);

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UBackgroundBlur* BackgroundBlur = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UWidgetSwitcher* ControllerSwitcher = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UWidgetSwitcher* ControllerSwitcherLines = nullptr;

	// Lines:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICheckbox* AutoDetectControllerType = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISlider* Vibration = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIScrollingButtonArray* ControllerPlatform = nullptr;

	// Map Selected Line Index => ESoUISettingsControllerLine (line type)
	// NOTE for this you also need to change the order in the UMG Blueprint Widget of the USoUIButtonArray.
	TArray<ESoUISettingsControllerLine> LineOptions = {
		//ESoUISettingsControllerLine::AutoDetectControllerType,
		ESoUISettingsControllerLine::Vibration,
		ESoUISettingsControllerLine::Layout,
	};

	// Map Selected Button Index => ESoGamepadLayoutType. Usually they are one and the same.
	const TArray<ESoGamepadLayoutType> ControllerPlatformOptions = {
		ESoGamepadLayoutType::Xbox,
		ESoGamepadLayoutType::PlayStation,
		ESoGamepadLayoutType::Switch
	};

	// The Force feedback to play if the Vibration slider changed
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Vibration")
	UForceFeedbackEffect* ForceFeedbackToPlayOnChange;

	// Gamepad buttons
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_RightShoulder = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_RightTrigger = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_LeftTrigger = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_LeftShoulder = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_FaceButton_Top = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_FaceButton_Left = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_FaceButton_Right = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_FaceButton_Bottom = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_Special_Left = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_Special_Right = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_LeftThumbstick = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_RightThumbstick = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_DPad_Up = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_DPad_Down = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_DPad_Left = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* Gamepad_DPad_Right = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettingsControllerRemap* RemapController = nullptr;

	static constexpr ESoUICommand UICommand_RemapController = ESoUICommand::EUC_Action0;
};
