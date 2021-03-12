// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "UI/General/SoUITypes.h"
#include "SoUIInputKeySelector.h"
#include "SSoInputKeySelector.h"
#include "Settings/Input/SoInputSettingsTypes.h"

#include "SoUIInputKeySelectorOverlay.generated.h"

class UTextBlock;
class USoUIButtonImageArray;
class UImage;

UENUM(BlueprintType)
enum class ESoInputOverlayKeyImageVisibility : uint8
{
	// Do not display any image
	None = 0,

	Keyboard,
	Gamepad,
	All
};

/**
 * Overlay that contains the InputKey selector
 */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIInputKeySelectorOverlay : public UUserWidget, public ISoUIEventHandler
{
	GENERATED_BODY()

public:
	// Events Hierarchy:
	// 1. SSoInputKeySelector
	// 2. USoUIInputKeySelector
	// 3. Self

	//
	// Own methods
	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoApplySelectedKeyEvent, const FInputChord&, SelectedKey);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoCancelSelectedKeyEvent);
	DECLARE_DELEGATE_RetVal_FourParams(bool, FSoIsSelectedKeyValidEvent, const FInputChord&, FText&, FText&, bool);

	// Is the KeySelector selecting keys?
	UFUNCTION(BlueprintPure, Category = ">Key Selection")
	bool IsSelectingKey() const;

	FKey GetSelectedKey() const;

	UFUNCTION(BlueprintCallable, Category = ">Key Selection")
	void SetIsSelectingKey(bool bInIsSelectingKey);

	// Key action name
	void SetActionName(const FText& ActionNameText);

	/** Fills the command tooltips from the current context. */
	void RefreshButtonImagesTooltips();

	// Is the selected key valid?
	bool IsSelectedKeyValid(FText& OutError, FText& OutWarning, bool bNoOutput = false) const;

	// Variant without error
	UFUNCTION(BlueprintPure, Category = ">Key Selection")
	bool IsSelectedKeyValid() const
	{
		FText Error, Warning;
		static constexpr bool bNoOutput = true;
		return IsSelectedKeyValid(Error, Warning, bNoOutput);
	}

	bool CanHandleUICommand() const { return !IsSelectingKey() && ISoUIEventHandler::Execute_IsOpened(this); }
	void Open(bool bSelectKey = false)
	{
		ISoUIEventHandler::Execute_Open(this, true);
		if (bSelectKey)
			OnCommandSelectAnotherKey();
	}
	void Close() { ISoUIEventHandler::Execute_Open(this, false); }

	bool OnPressedButtonTooltipsCommand(ESoUICommand Command);
	bool OnCommandSelectAnotherKey();
	bool OnCommandApply();
	bool OnCommandCancel();

	//
	// Events
	//
	FSoUIOpenChangedEvent& OnOpenChangedEvent() { return OpenChangedEvent; }
	FSoKeySelectedEvent& OnKeySelectedEvent() { return KeySelectedEvent; }
	FSoIsSelectingKeyChangedEvent& OnIsSelectingKeyChangedEvent() { return IsSelectingKeyChangedEvent; }
	FSoIsSelectedKeyValidEvent& OnIsSelectedKeyValidEvent() { return IsSelectedKeyValidEvent; }
	FSoApplySelectedKeyEvent& OnApplySelectedKeyEvent() { return ApplySelectedKeyEvent; }
	FSoCancelSelectedKeyEvent& OnCancelSelectedKeyEvent() { return CancelSelectedKeyEvent; }


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
	bool OnUICommand_Implementation(ESoUICommand Command) override;
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool CanBeInterrupted_Implementation() const override { return true; }

	//
	// Own methods
	//

	UFUNCTION()
	void HandleIsSelectingKeyChanged(bool bIsSelectingKey);

	UFUNCTION()
	void HandleKeySelected(const FInputChord& InSelectedKey);

	// KeySelectorImage
	void UpdateKeySelectorImage();

public:


protected:
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoUIOpenChangedEvent OpenChangedEvent;

	/** Called whenever a new key is selected by the user. */
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoKeySelectedEvent KeySelectedEvent;

	/** Called whenever the key selection state is changed. */
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoIsSelectingKeyChangedEvent IsSelectingKeyChangedEvent;

	/** Called whenever the selected key is applied. Before this fires this widget is closed. */
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoApplySelectedKeyEvent ApplySelectedKeyEvent;

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoCancelSelectedKeyEvent CancelSelectedKeyEvent;

	// Used to figure out if a key is valid
	FSoIsSelectedKeyValidEvent IsSelectedKeyValidEvent;

	// Widget that selects the key from the user
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIInputKeySelector* KeySelector = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* ActionName = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* CurrentlySelectedButtonText = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* ErrorMessage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* WarningMessage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* KeySelectorImageRepresentation = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonImageArray* ButtonImagesTooltipsArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Key Selection")
	ESoInputOverlayKeyImageVisibility KeySelectorImageVisibility = ESoInputOverlayKeyImageVisibility::None;

	// Passed to the KeySelector
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Key Selection")
	bool bAllowModifierKeysInSelection = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Key Selection")
	ESoInputKeySelectorDeviceType AllowedInputType = ESoInputKeySelectorDeviceType::Keyboard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Key Selection")
	TArray<FKey> EscapeKeys;

	// Is this widget opened?
	UPROPERTY()
	bool bOpened = false;

	// Keep track if we are listening or not
	UPROPERTY(BlueprintReadOnly, Category = ">Key Selection")
	ESoInputDeviceType GamepadDeviceType = ESoInputDeviceType::Gamepad_Generic;


	// Apply selected key/cancel
	static constexpr ESoUICommand UICommand_Apply = ESoUICommand::EUC_Action0;
	static constexpr ESoUICommand UICommand_Cancel = ESoUICommand::EUC_Action1;
	static constexpr ESoUICommand UICommand_SelectAnother = ESoUICommand::EUC_Action2;
};
