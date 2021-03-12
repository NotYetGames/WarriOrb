// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Framework/Commands/InputChord.h"
#include "Layout/Margin.h"
#include "Widgets/SWidget.h"
#include "Components/Widget.h"
#include "Styling/SlateTypes.h"
#include "SSoInputKeySelector.h"


#include "SoUIInputKeySelector.generated.h"

class SSoInputKeySelector;
struct FButtonStyle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoKeySelectedEvent, const FInputChord&, SelectedKey);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoIsSelectingKeyChangedEvent, bool, bIsSelectingKey);


/** Own version of the UInputKeySelector so that we can use the keyboard/gamepad only */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIInputKeySelector : public UWidget
{
	GENERATED_BODY()
	typedef USoUIInputKeySelector Self;

public:
	USoUIInputKeySelector(const FObjectInitializer& ObjectInitializer);

	// Events Hierarchy:
	// 1. SSoInputKeySelector
	// 2. Self

	//
	// Own methods
	//

	/** Sets the currently selected key. */
	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetSelectedKey(const FInputChord& InSelectedKey);

	// Gets the current selected key
	const FInputChord& GetSelectedKey() const { return SelectedKey; }

	/** Sets the text which is displayed while selecting keys. */
	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetKeySelectionText(FText InKeySelectionText);

	/** Sets the text to display when no key text is available or not selecting a key. */
	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetNoKeySpecifiedText(FText InNoKeySpecifiedText);

	void SetPreviousFocusedWidget(TSharedPtr<SWidget> InWidget);

	/** Sets whether or not modifier keys are allowed in the selected key. */
	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetAllowModifierKeysInSelection(bool bInAllowModifierKeys);

	/** What type of input are we accepting, if keyboard only all other inputs are ignored */
	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetAllowedInputType(ESoInputKeySelectorDeviceType NewType);

	/** Returns true if the widget is currently selecting a key, otherwise returns false. */
	UFUNCTION(BlueprintPure, Category = ">Widget")
	bool IsSelectingKey() const;

	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetIsSelectingKey(bool bInIsSelectingKey);

	/** Sets the visibility of the text block. */
	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetTextBlockVisibility(ESlateVisibility InVisibility);

	/** Sets the escape keys to check against. */
	UFUNCTION(BlueprintCallable, Category = ">Widget")
	void SetEscapeKeys(const TArray<FKey>& InEscapeKeys);

	/** Sets the style of the button used to start key selection mode. */
	void SetButtonStyle(const FButtonStyle* ButtonStyle);

	FSoKeySelectedEvent& OnKeySelectedEvent() { return KeySelectedEvent; }
	FSoIsSelectingKeyChangedEvent& OnIsSelectingKeyChangedEvent() { return IsSelectingKeyChangedEvent; }

	//
	// UWidget Interface
	//
	void SynchronizeProperties() override;

protected:
	//
	// UObject Interface
	//
	void PostLoad() override;
	void Serialize(FArchive& Ar) override;

	//
	// UWidget Interface
	//
	TSharedRef<SWidget> RebuildWidget() override;

	//
	// Begin UVisual Interface
	//
	void ReleaseSlateResources(bool bReleaseChildren) override;

	//
	// Own methods
	//
	void HandleKeySelected(const FInputChord& InSelectedKey);
	void HandleIsSelectingKeyChanged();

protected:
		/** Called whenever a new key is selected by the user. */
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoKeySelectedEvent KeySelectedEvent;

	/** Called whenever the key selection mode starts or stops. */
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoIsSelectingKeyChangedEvent IsSelectingKeyChangedEvent;

	/** The button style used at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Appearance", meta = (DisplayName = "Style"))
	FButtonStyle WidgetStyle;

	/** The button style used at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Appearance", meta = (DisplayName = "Text Style"))
	FTextBlockStyle TextStyle;

	/** The amount of blank space around the text used to display the currently selected key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Appearance")
	FMargin Margin;

	/** Sets the text which is displayed while selecting keys. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Appearance")
	FText KeySelectionText;

	/** Sets the text to display when no key text is available or not selecting a key. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Appearance")
	FText NoKeySpecifiedText;

	/** The currently selected key chord. */
	UPROPERTY(BlueprintReadOnly, Category = ">Key Selection")
	FInputChord SelectedKey;

	/**
	 * When true modifier keys such as control and alt are allowed in the
	 * input chord representing the selected key, if false modifier keys are ignored.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Key Selection")
	bool bAllowModifierKeysInSelection = false;

	/** What type of input are we accepting, if keyboard only all other inputs are ignored */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Key Selection")
	ESoInputKeySelectorDeviceType AllowedInputType = ESoInputKeySelectorDeviceType::Keyboard;

	/** When true gamepad keys are allowed in the input chord representing the selected key, otherwise they are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Key Selection")
	TArray<FKey> EscapeKeys;

private:
	/** The input key selector widget managed by this object. */
	TSharedPtr<SSoInputKeySelector> MyInputKeySelector;
};
