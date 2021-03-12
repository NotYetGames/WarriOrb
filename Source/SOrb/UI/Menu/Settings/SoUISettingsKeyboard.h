// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SoUISettingsBase.h"

#include "GameFramework/PlayerInput.h"

#include "UI/General/SoUITypes.h"
#include "Settings/Input/SoInputNames.h"

#include "SoUISettingsKeyboard.generated.h"

enum class ESoInputDeviceType : uint8;
class USoUIButtonArray;
class UPanelWidget;
class USoUICommandTooltipArray;
class USoUIInputKeySelectorOverlay;
class USoUIConfirmPanel;
class ASoPlayerController;
class UOverlay;
class USoGameSettings;
class USoUIConfirmKeyboardSettings;
class UScrollBox;

UENUM(BlueprintType)
enum class ESoUISettingsKeyboardState : uint8
{
	Browse = 0				UMETA(DisplayName = "Browse"),
	SelectingKey			UMETA(DisplayName = "SelectingKey"),
	ResetAllToDefaults		UMETA(DisplayName = "Reset All to defaults"),
};

UCLASS()
class SORB_API USoUISettingsKeyboard : public USoUISettingsBase
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
	void OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget) override;
	void CreateChildrenAndSetTexts(bool bFromSynchronizeProperties = false) override;
	bool HandleOnUICommand(ESoUICommand Command) override;
	void RefreshButtonImagesTooltips(bool bForceShowAll = false) override;
	void OnLineChanged(int32 OldLineIndex, int32 NewLineIndex) override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

	//
	// Own methods
	//

	bool OnCommandResetAllToDefaults();
	bool OnCommandResetKeyToDefault();

	// Creates the input button from the input user settings.
	void CreateInputButtonsFromUserSettings(const int32 SelectLineIndex = INDEX_NONE);

	// Called at the end of CreateInputButtonsFromUserSettings
	UFUNCTION(BlueprintImplementableEvent, Category = UI, meta = (DisplayName = "OnCreateInputButtonsFromUserSettings"))
	void ReceiveOnCreateInputButtonsFromUserSettings();

	// Is the key on the selected line default?
	UFUNCTION(BlueprintPure, Category = UI)
	bool IsSelectedActionMappingDefault() const;

	// Get the ActionMapping for the selected line/column
	UFUNCTION(BlueprintPure, Category = UI)
	bool GetSelectedActionMapping(FInputActionKeyMapping& ActionMapping) const;

	UFUNCTION()
	void HandleKeySelected(const FInputChord& InSelectedKey);

	UFUNCTION()
	void HandleIsSelectingKeyChanged(bool bIsSelectingKey);

	UFUNCTION()
	void HandleApplySelectedKey(const FInputChord& SelectedKey);

	// Did the settings change? default is what is displayed currently
	bool DidSettingsChange() const;

	// Is the SelectedKey valid?
	bool IsSelectedKeyValid(const FInputChord& SelectedKey, FText& OutError, FText& OutWarning, bool bNoOutput = false);

	// Sets the current state, opens/closes the confirm panel if it has to
	void SetCurrentState(ESoUISettingsKeyboardState InCurrentState);

	bool IsConfirmMode() const;
	bool OpenConfirmPanel();

	// Close the confirm panel
	UFUNCTION(BlueprintCallable)
	void ConfirmPanelClose();

	UFUNCTION()
	void OnKeySelectorOverlayOpenChanged(bool bOpen);

	UFUNCTION()
	void OnConfirmPanelOpenChanged(bool bPanelOpened);

	// Can we edit keyboard buttons
	bool UpdateCanEditKeyboardKeys();

	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType);

	void SubscribeToDeviceChanged();
	void UnSubscribeFromDeviceChanged();
protected:
	// Widget that selects the key from the user
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIInputKeySelectorOverlay* KeySelectorOverlay = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UPanelWidget* ContentContainer;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UOverlay* OnlyKeyboardOverlay;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	USoUIConfirmKeyboardSettings* ConfirmKeyboardSettings;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UScrollBox* ScrollBox;
	
	// All the configurable input option names
	// Maps from selected line => FSoInputConfigurableActionName
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FSoInputConfigurableActionName> InputOptions;

	// All the keyboard mapping options
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FInputActionKeyMapping> KeyboardMappingOptions;

	// Tells us which lines have the keyboard key unbound
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TSet<int32> UnboundKeyboardKeys;

	// Are we browsing, selecting a key, resetting to default
	UPROPERTY(BlueprintReadOnly)
	ESoUISettingsKeyboardState CurrentState = ESoUISettingsKeyboardState::Browse;

	UPROPERTY(BlueprintReadOnly)
	bool bIsListeningToDevicesChanges = false;

	// TODO do we need this? only if we implement Apply shortcuts
	// Tells us if any keys changed. Value is the line index of the key in it's respective column
	TArray<int32> ChangedKeyboardKeys;

	static constexpr ESoUICommand UICommand_ResetKeyToArrows = ESoUICommand::EUC_Action1;
	static constexpr ESoUICommand UICommand_ResetAllToWASD = ESoUICommand::EUC_Action2;
};
