// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SSoInputKeySelector.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Settings/Input/SoInputNames.h"

// #include "Widgets/Input/SInputKeySelector.h"

DEFINE_LOG_CATEGORY(LogSoInputSelector);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::Construct(const FArguments& InArgs)
{
	PreviousFocusedWidget = nullptr;

	SelectedKey = InArgs._SelectedKey;
	KeySelectionText = InArgs._KeySelectionText;
	NoKeySpecifiedText = InArgs._NoKeySpecifiedText;
	OnKeySelectedEvent = InArgs._OnKeySelectedEvent;
	OnIsSelectingKeyChangedEvent = InArgs._OnIsSelectingKeyChangedEvent;
	bAllowModifierKeysInSelection = InArgs._AllowModifierKeysInSelection;
	AllowedInputType = InArgs._AllowedInputType;
	bEscapeCancelsSelection = InArgs._EscapeCancelsSelection;
	EscapeKeys = InArgs._EscapeKeys;
	bIsButtonFocusable = InArgs._IsButtonFocusable;
	PreviousFocusedWidget = InArgs._PreviousFocusedWidget;

	bIsSelectingKey = false;

	ChildSlot
	[
		SAssignNew(Button, SButton)
		.ButtonStyle(InArgs._ButtonStyle)
		.IsFocusable(bIsButtonFocusable)
		.OnClicked(this, &Self::OnClicked)
		[
			SAssignNew(TextBlock, STextBlock)
			.Text(this, &Self::GetSelectedKeyText)
			.TextStyle(InArgs._TextStyle)
			.Margin(Margin)
			.Justification(ETextJustify::Center)
		]
	];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText SSoInputKeySelector::GetSelectedKeyText() const
{
	if (bIsSelectingKey)
		return KeySelectionText;

	if (SelectedKey.IsSet())
	{
		const FKey Key = SelectedKey.Get().Key;
		if (Key.IsValid())
		{
			// If the key in the chord is a modifier key, print it's display name directly since the FInputChord
			// displays these as empty text.
			return Key.IsModifierKey() ? FSoInputKey::GetKeyDisplayName(Key) : FSoInputKey::GetInputChordDisplayName(SelectedKey.Get());
		}
	}

	return NoKeySpecifiedText;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FInputChord SSoInputKeySelector::GetSelectedKey() const
{
	return SelectedKey.IsSet() ? SelectedKey.Get() : EKeys::Invalid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::SetSelectedKey(TAttribute<FInputChord> InSelectedKey)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::SetSelectedKey = %s"), bIsSelectingKey, *SelectedKey.Get().Key.ToString());
	if (SelectedKey.IdenticalTo(InSelectedKey) == false)
	{
		SelectedKey = InSelectedKey;
		OnKeySelectedEvent.ExecuteIfBound(SelectedKey.IsSet() ? SelectedKey.Get() : FInputChord(EKeys::Invalid));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FMargin SSoInputKeySelector::GetMargin() const
{
	return Margin.Get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnClicked()
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnClicked"), bIsSelectingKey);
	if (bIsSelectingKey == false)
	{
		UE_LOG(LogSoInputSelector, Verbose, TEXT("SSoInputKeySelector::OnClicked - calling SetIsSelectingKey(true)"));
		SetIsSelectingKey(true, true);
		return FReply::Handled()
			.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
	}
	return FReply::Handled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::SetMargin(TAttribute<FMargin> InMargin)
{
	Margin = InMargin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::SetButtonStyle(const FButtonStyle* ButtonStyle)
{
	if (Button.IsValid())
	{
		Button->SetButtonStyle(ButtonStyle);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::SetTextStyle(const FTextBlockStyle* InTextStyle)
{
	if (TextBlock.IsValid())
	{
		TextBlock->SetTextStyle(InTextStyle);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::SetSelectedKeyInternal(FKey Key, bool bShiftDown, bool bControlDown, bool bAltDown, bool bCommandDown)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::SetSelectedKeyInternal - Key = %s"), bIsSelectingKey, *Key.ToString());

	const FInputChord NewSelectedKey = bAllowModifierKeysInSelection
		? FInputChord(Key, bShiftDown, bControlDown, bAltDown, bCommandDown)
		: FInputChord(Key);

	// Not bound to anything
	if (SelectedKey.IsBound() == false)
	{
		SelectedKey.Set(NewSelectedKey);
	}
	OnKeySelectedEvent.ExecuteIfBound(NewSelectedKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::SetIsSelectingKey(bool bIsEnabled, bool bFromOnClicked)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::SetIsSelectingKey = %d"), bIsSelectingKey, bIsEnabled);
	if (bIsSelectingKey == bIsEnabled)
		return;

	// if (bInIsSelectingKey && !bFromOnClicked)
	// {
	// 	// FSlateApplication::Get().SetAllUserFocus(Button, EFocusCause::SetDirectly);
	// 	FGeometry Geometry;
	// 	FPointerEvent MouseEvent;
	// 	Button->OnMouseButtonDown(Geometry, MouseEvent);
	// 	return;
	// }

	// Focus first
	if (bIsEnabled)
	{
		UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::SetIsSelectingKey Focus THIS Selector"), bIsSelectingKey);

		// Ignore first key if from keyboard
		if (!bFromOnClicked)
			bIgnoreKeyUp = true;

		SelectedKey = FInputChord();
		FocusThisWidget();
	}
	else
	{
		UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::SetIsSelectingKey Returning focus to game"), bIsSelectingKey);
		FocusPreviousWidget();
	}

	bIsSelectingKey = bIsEnabled;
	// bWasFirstKeyIgnored = false;

	// Prevents certain inputs from being consumed by the button
	if (Button.IsValid())
	{
		Button->SetEnabled(!bIsSelectingKey);
	}
	OnIsSelectingKeyChangedEvent.ExecuteIfBound();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	Super::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	CurrentTick++;

	// Mouse fix hack I guess
	if (bIsSelectingKey)
	{
		if (!IsThisFocused())
		{
			UE_LOG(
				LogSoInputSelector,
				Verbose,
				TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::Tick - We are not focused calling FocusThisWidget() - MOUSE PROBLEM?"),
				bIsSelectingKey
			);
			bIgnoreKeyUp = false;
			FocusThisWidget();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// If a widget handles this event, OnKeyDown will *not* be passed to the focused widget.
	UE_LOG(
		LogSoInputSelector,
		Verbose,
		TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnPreviewKeyDown Key = %s, bIsSelectingKey = %d"),
		bIsSelectingKey, *InKeyEvent.GetKey().GetFName().ToString()
	);

	// NOTE: if this won't get handled here then we will get the input not consumed bug, that is resolved
	// by loosing focus and getting focus back
	if (bIsSelectingKey && AllowKey(InKeyEvent.GetKey()))
	{
		// While selecting keys handle all key downs to prevent contained controls from interfering with key selection.
		UE_LOG(LogSoInputSelector, Verbose, TEXT("SSoInputKeySelector::OnPreviewKeyDown Not Handled (allowing key)"));
		return FReply::Handled();
	}

	UE_LOG(LogSoInputSelector, Verbose, TEXT("SSoInputKeySelector::OnPreviewKeyDown Handled (not allowing key)"));
	return Super::OnPreviewKeyDown(MyGeometry, InKeyEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnKeyChar - %s"), bIsSelectingKey, *InCharacterEvent.ToText().ToString());
	return Super::OnKeyChar(MyGeometry, InCharacterEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// NOTE: This method should NOT be called. As the Widget passes it to the parent which is us.
	// It is handled in OnPreviewKeyDown
	// This should only work if bIsButtonFocusable = true
	// TODO: does this ever work?
	UE_LOG(
		LogSoInputSelector,
		Verbose,
		TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnKeyDown - Key = %s"),
		bIsSelectingKey, *InKeyEvent.GetKey().GetFName().ToString(), bIsSelectingKey
	);

	// Ignore all keys if we are not selecting anything
	if (!bIsSelectingKey)
	{
		// TODO we probably need to change this
		if (SelectedKey.IsSet() && SelectedKey.Get().Key.IsValid() && (AllowGamepadKeys() && InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Left))
		{
			SelectedKey = FInputChord();
			return FReply::Handled();
		}
		if (Button.IsValid())
		{
			return Button->OnKeyDown(MyGeometry, InKeyEvent);
		}
	}

	return Super::OnKeyDown(MyGeometry, InKeyEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FString ContextString = FString::Printf(
		TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnKeyUp - Key = %s"),
		bIsSelectingKey, *InKeyEvent.GetKey().GetFName().ToString()
	);
	UE_LOG(LogSoInputSelector, Verbose, TEXT("%s"), *ContextString);

	if (bIgnoreKeyUp)
	{
		UE_LOG(LogSoInputSelector, Verbose, TEXT("%s - Ignoring because bIgnoreKeyUp = %d"), *ContextString, bIgnoreKeyUp);
		bIgnoreKeyUp = false;
		return FReply::Handled();
	}

	if (bIsSelectingKey)
	{
		const FKey KeyUp = InKeyEvent.GetKey();
		const EModifierKey::Type ModifierKey = EModifierKey::FromBools(
			InKeyEvent.IsControlDown() && KeyUp != EKeys::LeftControl && KeyUp != EKeys::RightControl,
			InKeyEvent.IsAltDown() && KeyUp != EKeys::LeftAlt && KeyUp != EKeys::RightAlt,
			InKeyEvent.IsShiftDown() && KeyUp != EKeys::LeftShift && KeyUp != EKeys::RightShift,
			InKeyEvent.IsCommandDown() && KeyUp != EKeys::LeftCommand && KeyUp != EKeys::RightCommand);

		// Don't allow chords consisting of just modifier keys.
		const bool bInvalidModifierKey = KeyUp.IsModifierKey() == false || ModifierKey == EModifierKey::None;
		if (AllowKey(KeyUp) && bInvalidModifierKey)
		{
			UE_LOG(LogSoInputSelector, Verbose, TEXT("%s - Allowing Key = %s"), *ContextString, *KeyUp.GetFName().ToString());

			// Cancel selection
			if (bEscapeCancelsSelection && IsEscapeKey(KeyUp))
			{
				UE_LOG(LogSoInputSelector, Verbose, TEXT("%s - Cancel Selection, caling SetIsSelectingKey(false)"), *ContextString);
				SetIsSelectingKey(false);
				return FReply::Handled();
			}

			SetSelectedKeyInternal(
				KeyUp,
				ModifierKey == EModifierKey::Shift,
				ModifierKey == EModifierKey::Control,
				ModifierKey == EModifierKey::Alt,
				ModifierKey == EModifierKey::Command);

			// Selected key return
			UE_LOG(LogSoInputSelector, Verbose, TEXT("%s - Selected key return, caling SetIsSelectingKey(false)"), *ContextString);
			SetIsSelectingKey(false);
			return FReply::Handled();
		}
		// Do this until we allow the key
	}
	if (!bIsSelectingKey && Button.IsValid())
	{
		return Button->OnKeyUp(MyGeometry, InKeyEvent);
	}

	return Super::OnKeyUp(MyGeometry, InKeyEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(
		LogSoInputSelector,
		Verbose,
		TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnPreviewMouseButtonDown - Button = %s"),
		bIsSelectingKey, *MouseEvent.ToText().ToString()
	);

	// Disabled mouse
	if (bIsSelectingKey && AllowKeyboardAndMouseKeys())
	{
		UE_LOG(
			LogSoInputSelector,
			Verbose,
			TEXT("SSoInputKeySelector::OnPreviewMouseButtonDown - Button = %s - Calling SetIsSelectingKey(false)"),
			*MouseEvent.ToText().ToString()
		);
		SetIsSelectingKey(false);
		SetSelectedKeyInternal(MouseEvent.GetEffectingButton(), false, false, false, false);
		return FReply::Handled();
	}

	return Super::OnPreviewMouseButtonDown(MyGeometry, MouseEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UE_LOG(
		LogSoInputSelector,
		Verbose,
		TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnMouseButtonDown Button = %s"),
		bIsSelectingKey, *MouseEvent.ToText().ToString()
	);
	if (!bIsSelectingKey && SelectedKey.IsSet() && SelectedKey.Get().Key.IsValid())
	{
		return FReply::Handled();
	}
	return Super::OnMouseButtonDown(MyGeometry, MouseEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	FString PreviousWidgetString = TEXT("NULL");
	FString NewWidgetString = TEXT("NULL");
	if (PreviousFocusPath.IsValid())
	{
		const FWidgetPath PreviousWidgetPath = PreviousFocusPath.ToWidgetPath();
		if (PreviousFocusPath.IsValid())
		{
			const TSharedRef<SWidget> PreviousWidget = PreviousWidgetPath.GetLastWidget();
			PreviousWidgetString = PreviousWidget->ToString();
		}
	}
	if (NewWidgetPath.IsValid())
	{
		const TSharedRef<SWidget> NewWidget = NewWidgetPath.GetLastWidget();
		NewWidgetString = NewWidget->ToString();
	}

	UE_LOG(
		LogSoInputSelector,
		Verbose,
		TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnFocusChanging - PreviousFocusPath = %s, NewWidgetPath = %s"),
		bIsSelectingKey, *PreviousWidgetString, *NewWidgetString
	);

	Super::OnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FReply SSoInputKeySelector::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnFocusReceived"), bIsSelectingKey);
	OnFocusedReceivedTick = CurrentTick;
	return Super::OnFocusReceived(MyGeometry, InFocusEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnFocusLost"), bIsSelectingKey);
	if (bIsSelectingKey)
	{
		// NOTE: See Tick() for a fix for mouse
		if (CurrentTick == OnFocusedReceivedTick)
		{
			UE_LOG(
				LogSoInputSelector,
				Verbose,
				TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnFocusLost - CurrentTick(%d) == OnFocusedReceivedTick(%d) - MOUSE PROBLEM FOCUS????"),
				bIsSelectingKey, CurrentTick, OnFocusedReceivedTick
			);
		}
		else
		{
			UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnFocusLost - Calling SetIsSelectingKey(false)"), bIsSelectingKey);
			SetIsSelectingKey(false);
		}
	}

	// if (!bIsSelectingKey)
	// 	return;
	//
	// UE_LOG(LogSoInputSelector, Verbose, TEXT("SSoInputKeySelector::OnFocusLost Cause = %d"), static_cast<int32>(InFocusEvent.GetCause()))
	// SetIsSelectingKey(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FNavigationReply SSoInputKeySelector::OnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent)
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::OnNavigation"), bIsSelectingKey);

	if (bIsSelectingKey)
		return FNavigationReply::Stop();

	if (Button.IsValid())
	{
		return Button->OnNavigation(MyGeometry, InNavigationEvent);
	}

	return Super::OnNavigation(MyGeometry, InNavigationEvent);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::SetTextBlockVisibility(EVisibility InVisibility)
{
	if (TextBlock.IsValid())
	{
		TextBlock->SetVisibility(InVisibility);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SSoInputKeySelector::AllowKey(const FKey& Key) const
{
	if (!Key.IsValid())
		return false;

	// Gamepad
	if (AllowGamepadKeys() && Key.IsGamepadKey())
		return true;

	// Mouse without keyboard
	const bool bIsKeyboardKey = !Key.IsGamepadKey();
	const bool bIsMouseButton = Key.IsMouseButton();
	if (AllowKeyboardKeys() && (bIsKeyboardKey && !bIsMouseButton))
		return true;

	// Mouse with keyboard
	if (AllowKeyboardAndMouseKeys() && (bIsKeyboardKey || bIsMouseButton))
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SSoInputKeySelector::IsThisFocused() const
{
	const TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
	return SharedThis(this) == FocusedWidget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::FocusThisWidget()
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::FocusThisWidget"), bIsSelectingKey);
	FSlateApplication::Get().SetAllUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SSoInputKeySelector::FocusPreviousWidget()
{
	UE_LOG(LogSoInputSelector, Verbose, TEXT("[bIsSelectingKey = %d] SSoInputKeySelector::FocusPreviousWidget"), bIsSelectingKey);
	if (PreviousFocusedWidget.IsValid())
		FSlateApplication::Get().SetAllUserFocus(PreviousFocusedWidget.ToSharedRef(), EFocusCause::SetDirectly);
	else
		// Reset to viewport
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
