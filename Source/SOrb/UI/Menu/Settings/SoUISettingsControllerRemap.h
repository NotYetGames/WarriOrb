// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SoUISettingsBase.h"
#include "UI/General/SoUITypes.h"
#include "Settings/Input/SoInputNames.h"
#include "GameFramework/PlayerInput.h"


#include "SoUISettingsControllerRemap.generated.h"

enum class ESoInputDeviceType : uint8;
class UPanelWidget;
class USoUIInputKeySelectorOverlay;
class USoUICommandTooltipArray;
class USoGameSettings;
class ASoPlayerController;
class USoUIButtonArray;
class UOverlay;
class USoUIButtonImage;


UENUM(BlueprintType)
enum class ESoUISettingsControllerRemapState : uint8
{
	Browse = 0			UMETA(DisplayName = "Browse"),
	SelectingKey		UMETA(DisplayName = "SelectingKey"),
	ResetAllToDefaults	UMETA(DisplayName = "Reset All to defaults"),
};

/**
 * Shows the controller input options
 */
UCLASS()
class SORB_API USoUISettingsControllerRemap : public USoUISettingsBase
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
	bool CanBeInterrupted_Implementation() const override;

	//
	// USoUISettingsBase Interface
	//
	void CreateChildrenAndSetTexts(bool bFromSynchronizeProperties = false) override;
	bool HandleOnUICommand(ESoUICommand Command) override;
	void NavigateBackCommand() override;
	void OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget) override;
	bool NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput = false) override;

	void RefreshButtonImagesTooltips(bool bForceShowAll = false) override;
	void OnLineChanged(int32 OldLineIndex, int32 NewLineIndex) override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

	//
	// Own methods
	//

	void OnCommandResetAllToDefaults();

	UFUNCTION()
	void HandleKeySelected(const FInputChord& InSelectedKey);

	UFUNCTION()
	void HandleIsSelectingKeyChanged(bool bIsSelectingKey);

	UFUNCTION()
	void HandleApplySelectedKey(const FInputChord& SelectedKey);

	UFUNCTION()
	void OnKeySelectorOverlayOpenChanged(bool bOpen);

	// Is the SelectedKey valid?
	bool IsSelectedKeyValid(const FInputChord& SelectedKey, FText& OutError, FText& OutWarning, bool bNoOutput = false);

	// Can we edit gamepad buttons
	bool UpdateCanEditGamepadButtons();

	// Creates the input button from the input user settings.
	void CreateInputButtonsFromUserSettings(int32 SelectLineIndex = INDEX_NONE);

	// Called at the end of CreateInputButtonsFromUserSettings
	UFUNCTION(BlueprintImplementableEvent, Category = UI, meta = (DisplayName = "OnCreateInputButtonsFromUserSettings"))
	void ReceiveOnCreateInputButtonsFromUserSettings();

	// Change state
	void SetCurrentState(ESoUISettingsControllerRemapState InCurrentState);

	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType);

	void SubscribeToDeviceChanged();
	void UnSubscribeFromDeviceChanged();

	// Update images to match the device type
	void UpdateGamepadButtonsImages();

	UFUNCTION(BlueprintPure, Category = UI)
	bool IsLineUnboundGamepad(int32 LineIndex) const { return UnboundLineIndices.Contains(LineIndex);  }

	UFUNCTION(BlueprintImplementableEvent, Category = UI, meta = (DisplayName = "OnUpdateGamepadButton"))
	void ReceiveOnUpdateGamepadButton(int32 ButtonIndex, USoUIButtonImage* Button);

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UOverlay* OnlyGamepadOverlay = nullptr;

	// Widget that selects the key from the user
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIInputKeySelectorOverlay* KeySelectorOverlay = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UPanelWidget* ContentContainer;

	// Tells us which lines have the gamepad key unbound
	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	TSet<int32> UnboundLineIndices;

	// Are we browsing, selecting a key, resetting to default
	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	ESoUISettingsControllerRemapState CurrentState = ESoUISettingsControllerRemapState::Browse;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	bool bIsListeningToDevicesChanges = false;

	// Remember our line we are modifying
	int32 SelectedKeyLineIndex = INDEX_NONE;

	// All the configurable input option names
	// Maps from selected line => FSoInputConfigurableActionName
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	TArray<FSoInputConfigurableActionName> InputOptions;

	// All the keyboard mapping options
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = ">Runtime")
	TArray<FInputActionKeyMapping> GamepadMappingOptions;

	static constexpr ESoUICommand UICommand_ResetAllToDefaults = ESoUICommand::EUC_Action0;
};
