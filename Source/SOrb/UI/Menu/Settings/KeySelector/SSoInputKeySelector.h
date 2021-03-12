// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "InputCoreTypes.h"
#include "Layout/Margin.h"
#include "Fonts/SlateFontInfo.h"
#include "Input/Reply.h"
#include "Styling/SlateWidgetStyleAsset.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Framework/Commands/InputChord.h"
#include "Layout/Visibility.h"

#include "SoLocalization.h"


// NOTE: to debug the selector, set this to All
DECLARE_LOG_CATEGORY_EXTERN(LogSoInputSelector, Log, All);

class SButton;
class STextBlock;

// From what device should we accept input
UENUM(BlueprintType)
enum class ESoInputKeySelectorDeviceType : uint8
{
	All = 0,

	Keyboard,
	KeyboardAndMouse,

	Gamepad,
};

/**
 * A widget for selecting keys or input chords.
 * Modified version of the SInputKeySelector from unreal
 */
class SORB_API SSoInputKeySelector : public SCompoundWidget
{
	typedef SSoInputKeySelector Self;
	typedef SCompoundWidget Super;

public:
	// Events that something changed
	DECLARE_DELEGATE_OneParam(FSoKeySelectedEvent, const FInputChord&)
	DECLARE_DELEGATE(FSoIsSelectingKeyChangedEvent)

	SLATE_BEGIN_ARGS(Self)
		: _SelectedKey(FInputChord(EKeys::Invalid) )
		, _ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
		, _TextStyle(&FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText"))
		, _KeySelectionText(FROM_STRING_TABLE_UI("line_three_dots"))
		, _NoKeySpecifiedText(FROM_STRING_TABLE_UI("empty"))
		, _AllowModifierKeysInSelection(false)
		, _AllowedInputType(ESoInputKeySelectorDeviceType::Keyboard)
		, _EscapeCancelsSelection(true)
		, _IsButtonFocusable(true)
		{}

		/** The currently selected key */
		SLATE_ATTRIBUTE(FInputChord, SelectedKey)

		/** The font used to display the currently selected key. */
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)

		/** The margin around the selected key text. */
		SLATE_ATTRIBUTE(FMargin, Margin)

		/** The style of the button used to enable key selection. */
		SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)

		/** The text style of the button text */
		SLATE_STYLE_ARGUMENT(FTextBlockStyle, TextStyle)

		/** The text to display while selecting a new key. */
		SLATE_ARGUMENT(FText, KeySelectionText)

		/** The text to display while no key text is available or not selecting a key. */
		SLATE_ARGUMENT(FText, NoKeySpecifiedText)

		/** When true modifier keys are captured in the selected key chord, otherwise they are ignored. */
		SLATE_ARGUMENT(bool, AllowModifierKeysInSelection)

		/** What type of input are we accepting, if keyboard only all other inputs are ignored */
		SLATE_ARGUMENT(ESoInputKeySelectorDeviceType, AllowedInputType)

		/** When true, pressing escape will cancel the key selection, when false, pressing escape will select the escape key. */
		SLATE_ARGUMENT(bool, EscapeCancelsSelection)

		/** When EscapeCancelsSelection is true, escape on specific keys that are unbind able by the user. */
		SLATE_ARGUMENT(TArray<FKey>, EscapeKeys)

		/** The Widget to return the focus to after we are not selecting the key anymore */
		SLATE_ARGUMENT(TSharedPtr<SWidget>, PreviousFocusedWidget)

		/** Occurs whenever a new key is selected. */
		SLATE_EVENT(FSoKeySelectedEvent, OnKeySelectedEvent)

		/** Occurs whenever key selection mode starts and stops. */
		SLATE_EVENT(FSoIsSelectingKeyChangedEvent, OnIsSelectingKeyChangedEvent)

		/** Sometimes a button should only be mouse-clickable and never keyboard focusable. */
		SLATE_ARGUMENT(bool, IsButtonFocusable)
	SLATE_END_ARGS()

	//
	// Own methods
	//

	void Construct(const FArguments& InArgs);

	/** Gets the currently selected key chord. */
	FInputChord GetSelectedKey() const;

	/** Sets the currently selected key chord. */
	void SetSelectedKey(TAttribute<FInputChord> InSelectedKey);

	/** Sets the margin around the text used to display the currently selected key */
	void SetMargin(TAttribute<FMargin> InMargin);

	/** Sets the style of the button which is used enter key selection mode. */
	void SetButtonStyle(const FButtonStyle* ButtonStyle);

	/** Sets the style of the text on the button which is used enter key selection mode. */
	void SetTextStyle(const FTextBlockStyle* InTextStyle);

	/** Sets the text which is displayed when selecting a key. */
	void SetKeySelectionText(FText InKeySelectionText) { KeySelectionText = MoveTemp(InKeySelectionText); }

	/** Sets the text to display when no key text is available or not selecting a key. */
	void SetNoKeySpecifiedText(FText InNoKeySpecifiedText) { NoKeySpecifiedText = MoveTemp(InNoKeySpecifiedText); }

	// what it says
	void SetPreviousFocusedWidget(TSharedPtr<SWidget> InWidget) { PreviousFocusedWidget = InWidget; }

	/** When true modifier keys are captured in the selected key chord, otherwise they are ignored. */
	void SetAllowModifierKeysInSelection(bool bInAllowModifierKeys) { bAllowModifierKeysInSelection = bInAllowModifierKeys; }

	/** What type of input are we accepting, if keyboard only all other inputs are ignored */
	void SetAllowedInputType(ESoInputKeySelectorDeviceType NewType) { AllowedInputType = NewType; }

	/** Sets the escape keys to check against. */
	void SetEscapeKeys(const TArray<FKey>& InEscapeKeys) { EscapeKeys = InEscapeKeys; }

	/** Returns true whenever key selection mode is active, otherwise returns false. */
	bool IsSelectingKey() const { return bIsSelectingKey; }

	/** Sets the visibility of the text block. */
	void SetTextBlockVisibility(EVisibility InVisibility);

	/** Sets bIsSelectingKey and invokes the associated events. */
	void SetIsSelectingKey(bool bIsEnabled, bool bFromOnClicked = false);

	//
	// SWidget interface
	//
	void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;


	// Key input
	FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override;
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	// Mouse
	FReply OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Focus
	void OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override;
	FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	void OnFocusLost(const FFocusEvent& InFocusEvent) override;

	FNavigationReply OnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent) override;
	bool SupportsKeyboardFocus() const override { return true; }

private:
	//
	// Own methods
	//

	/** Gets the display text for the currently selected key. */
	FText GetSelectedKeyText() const;

	/**  Gets the margin around the text used to display the currently selected key. */
	FMargin GetMargin() const;

	// Handles the OnClicked event from the button which enables key selection mode.
	FReply OnClicked();

	/** Sets the currently selected key and invokes the associated events. */
	void SetSelectedKeyInternal(FKey Key, bool bShiftDown, bool bControlDown, bool bAltDown, bool bCommandDown);

	/** Returns true, if the key has been specified as an escape key, else false. */
	bool IsEscapeKey(const FKey& InKey) const
	{
		return InKey == EKeys::PS4_Special || InKey == EKeys::Escape || EscapeKeys.Contains(InKey);
	}

	bool AllowKeyboardAndMouseKeys() const
	{
		return AllowedInputType == ESoInputKeySelectorDeviceType::All ||
			   AllowedInputType == ESoInputKeySelectorDeviceType::KeyboardAndMouse;
	}
	bool AllowGamepadKeys() const
	{
		return AllowedInputType == ESoInputKeySelectorDeviceType::All ||
			   AllowedInputType == ESoInputKeySelectorDeviceType::Gamepad;
	}
	bool AllowKeyboardKeys() const
	{
		return AllowedInputType == ESoInputKeySelectorDeviceType::All ||
			   AllowedInputType == ESoInputKeySelectorDeviceType::Keyboard ||
			   AllowedInputType == ESoInputKeySelectorDeviceType::KeyboardAndMouse;
	}
	bool AllowKey(const FKey& Key) const;

	bool IsThisFocused() const;
	void FocusThisWidget();
	void FocusPreviousWidget();

private:
	/** True when key selection mode is active. */
	bool bIsSelectingKey = false;

	/** The currently selected key chord. */
	TAttribute<FInputChord> SelectedKey;

	/** The margin around the text used to display the currently selected key. */
	TAttribute<FMargin> Margin;

	/** The text to display when selecting keys. */
	FText KeySelectionText;

	/**  The text to display while no key text is available or not selecting a key. */
	FText NoKeySpecifiedText;

	/** When true modifier keys are recorded on the selected key chord, otherwise they are ignored. */
	bool bAllowModifierKeysInSelection = false;

	/** What type of input are we accepting, if keyboard only all other inputs are ignored */
	ESoInputKeySelectorDeviceType AllowedInputType = ESoInputKeySelectorDeviceType::Keyboard;

	/** When true, pressing escape will cancel the key selection, when false, pressing escape will select the escape key. */
	bool bEscapeCancelsSelection = true;

	/**
	 * When EscapeCancelsSelection is true, escape on specific keys that are unbind able by the user.
	 *
	 * This is besides  EKeys::PS4_Special and EKeys::Escape
	 */
	TArray<FKey> EscapeKeys;

	/** Delegate which is run any time a new key is selected. */
	FSoKeySelectedEvent OnKeySelectedEvent;

	/** Delegate which is run when key selection mode starts and stops. */
	FSoIsSelectingKeyChangedEvent OnIsSelectingKeyChangedEvent;

	/** The button which starts the key selection mode. */
	TSharedPtr<SButton> Button;

	/** The text which is rendered on the button. */
	TSharedPtr<STextBlock> TextBlock;

	/** Can this button be focused? */
	bool bIsButtonFocusable = false;

	// Should we ignore it?
	bool bIgnoreKeyUp = false;

	// TODO get rid of this, use OnFocusChanging
	// Keep track of the previous focused widget
	TSharedPtr<SWidget> PreviousFocusedWidget;


	// Track the current tick
	int64 CurrentTick = 0;
	int64 OnFocusedReceivedTick = 0;
};
