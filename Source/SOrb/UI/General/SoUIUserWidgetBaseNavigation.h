// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SoUITypes.h"

#include "SoUIUserWidgetBaseNavigation.generated.h"

class UImage;
class UFMODEvent;


DECLARE_MULTICAST_DELEGATE_OneParam(FSoUIUserWidgetRequestHighlightChangeEvent, bool);


/**
 *  Common UserWidget functionality to be inherited by other widgets used for navigation with keyboard/controller/mouse
 */
UCLASS(Abstract, HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIUserWidgetBaseNavigation : public UUserWidget
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
 	FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	//  Sets the current enabled status of the widget
	void SetIsEnabled(bool bInIsEnabled) override;

	//
	// Own methods
	//

	UFUNCTION(BlueprintCallable, Category = ">Navigation")
	virtual bool Navigate(ESoUICommand Command, bool bPlaySound = true);

	UFUNCTION(BlueprintCallable, Category = ">Navigation")
	virtual bool NavigateOnPressed(bool bPlaySound = true);

	// Shortcut to set visibility so that we show/hide it correctly;
	UFUNCTION(BlueprintCallable, Category = ">Visibility")
	virtual void Show() { SetVisibility(ESlateVisibility::Visible); }

	UFUNCTION(BlueprintCallable, Category = ">Visibility")
	virtual void Hide() { SetVisibility(ESlateVisibility::Collapsed); }

	// Is active and visible
	UFUNCTION(BlueprintPure, Category = ">Widget")
	virtual bool IsValidWidget() const
	{
		return GetIsEnabled() && IsVisible();
	}

	UFUNCTION(BlueprintCallable, Category = ">State")
	bool Highlight(bool bPlaySound = false) { return SetIsHighlighted(true, bPlaySound); }

	UFUNCTION(BlueprintCallable, Category = ">State")
	bool Unhighlight(bool bPlaySound = false) { return SetIsHighlighted(false, bPlaySound); }

	UFUNCTION(BlueprintPure, Category = ">State", meta=(ScriptName="GetIsHighlighted"))
	bool IsHighlighted() const { return bIsHighlighted; }

	UFUNCTION(BlueprintPure, Category = ">State", meta=(ScriptName="GetIsPressed"))
	bool IsPressed() const { return bIsPressed; }

	// Return true if NativeConstruct() was called
	UFUNCTION(BlueprintPure, Category = ">State", meta=(ScriptName="GetIsConstructed"))
	bool IsConstructed() const { return bIsConstructed; }

	UFUNCTION(BlueprintCallable, Category = ">State")
	virtual bool SetIsHighlighted(bool bInHighlighted, bool bPlaySound = false);

	bool IsHandlingOwnMouseEvents() const { return bHandleOwnMouseEvents; }
	void SetHandleOwnMouseEvents(bool bValue) { bHandleOwnMouseEvents = bValue; }

	ThisClass* SetSFXHighlightChanged(UFMODEvent* SFX)
	{
		SFXHighlightChanged = SFX;
		return this;
	}
	ThisClass* SetSFXOnPressed(UFMODEvent* SFX)
	{
		SFXOnPressed = SFX;
		return this;
	}

	// Events
	FSoUIUserWidgetRequestHighlightChangeEvent& OnHighlightRequestChangeEvent() { return HighlightRequestChangeEvent; }
	FSoPreNavigateEvent& OnPreNavigateEvent() { return PreNavigateEvent; }
	FSoPostNavigateEvent& OnPostNavigateEvent() { return PostNavigateEvent; }

	FSoUIUserWidgetEventCPP& OnStateChangedEvent() { return ProxyStateChangedEventCPP; }
	FSoUIUserWidgetEventCPP& OnPressedEvent() { return ProxyPressedEventCPP; }
	FSoUIUserWidgetChangedEventCPP& OnHighlightChangedEvent() { return ProxyHighlightChangedCPP; }
	FSoUIUserWidgetChangedEventCPP& OnEnabledChangedEvent() { return ProxyEnabledChangedCPP; }

protected:
	//
	// Handle CPP proxies
	//
	UFUNCTION()
	void OnProxyStateChangedEvent() { ProxyStateChangedEventCPP.Broadcast(); }
	UFUNCTION()
	void OnProxyPressedEvent() { ProxyPressedEventCPP.Broadcast(); }
	UFUNCTION()
	void OnProxyHighlightChangedEvent(bool bNewValue) { ProxyHighlightChangedCPP.Broadcast(bNewValue); }
	UFUNCTION()
	void OnProxyEnabledChangedEvent(bool bNewValue) { ProxyEnabledChangedCPP.Broadcast(bNewValue); }

	virtual void SetIsPressed(bool bValue)
	{
		bIsPressed = bValue;
		if (bIsPressed)
			OnPressed();
	}
	virtual FReply OnUIEventType(ESoUIEventType EventType);

public:
	//
	// Handle BP events and events firing
	//

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", DisplayName = "OnStateChanged")
	void ReceiveOnStateChanged();
	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnStateChanged()
	{
		ReceiveOnStateChanged();
		StateChangedEvent.Broadcast();
	}

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", DisplayName = "OnPressed")
	void ReceiveOnPressed();
	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnPressed()
	{
		ReceiveOnPressed();
		PressedEvent.Broadcast();
	}

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", DisplayName = "OnHighlightChanged")
	void ReceiveOnHighlightChanged(bool bHighlight);
	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnHighlightChanged(bool bHighlight)
	{
		ReceiveOnHighlightChanged(bHighlight);
		HighlightChangedEvent.Broadcast(bHighlight);
		OnStateChanged();
	}

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", DisplayName = "OnEnabledChanged")
	void ReceiveOnEnabledChanged(bool bEnabled);
	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnEnabledChanged(bool bEnabled)
	{
		ReceiveOnEnabledChanged(bEnabled);
		EnabledChangedEvent.Broadcast(bEnabled);
		OnStateChanged();
	}

protected:
	//
	// Events
	//

	// Special event used for arrays to know if we got highlighter
	FSoUIUserWidgetRequestHighlightChangeEvent HighlightRequestChangeEvent;

	// Broadcasts before doing anything in Navigate()
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnPreNavigateEvent")
	FSoPreNavigateEvent PreNavigateEvent;

	// Broadcasts after executing Navigate()
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnPostNavigateEvent")
	FSoPostNavigateEvent PostNavigateEvent;

	// Fired when the state of the widget changes
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnStateChangedEvent")
	FSoUIUserWidgetEvent StateChangedEvent;

	// Fired when widget is pressed
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnPressedEvent")
	FSoUIUserWidgetEvent PressedEvent;

	// Fired when the Highlight value of this widget changed
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnHighlightChangedEvent")
	FSoUIUserWidgetChangedEvent HighlightChangedEvent;

	// Fired when the Enabled value of this widget changed
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnEnabledChangedEvent")
	FSoUIUserWidgetChangedEvent EnabledChangedEvent;

	// Proxy events that pass from Dynamic event to our event handler in CPP
	FSoUIUserWidgetEventCPP ProxyStateChangedEventCPP;
	FSoUIUserWidgetEventCPP ProxyPressedEventCPP;
	FSoUIUserWidgetChangedEventCPP ProxyHighlightChangedCPP;
	FSoUIUserWidgetChangedEventCPP ProxyEnabledChangedCPP;

	//
	// Variables
	//

	// Is highlighted, usually used for selected stuff.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">State")
	bool bIsHighlighted = false;

	// This widget is handling the mouse events
	// Instead of a parent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Mouse")
	bool bHandleOwnMouseEvents = false;

	// Is this widget pressed
	// Set on  ESoUIEventType::MouseButtonDown
	// Unset on ESoUIEventType::MouseButtonUp
	UPROPERTY(BlueprintReadOnly, Category = ">Mouse")
	bool bIsPressed = false;

	// Was constructed at runtime?
	// Set in NativeConstruct()
	// Unset in NativeDestruct()
	UPROPERTY(BlueprintReadOnly, Category = ">State")
	bool bIsConstructed = false;

	//
	// SFX
	//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXHighlightChanged = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOnPressed = nullptr;
};
