// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoUIUserWidgetBaseNavigation.h"
#include "SoUITypes.h"

#include "SoUIUserWidget.generated.h"

class UImage;
class UFMODEvent;



/**
 *  Common UserWidget functionality to be inherited by other widgets
 *  A USoUIUserWidgetBaseNavigation + can be activated/deactivate
 */
UCLASS(Abstract)
class SORB_API USoUIUserWidget : public USoUIUserWidgetBaseNavigation
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// USoUIUserWidgetBaseNavigation Interface
	//

	bool NavigateOnPressed(bool bPlaySound = true) override;

	// Is active and visible
	bool IsValidWidget() const override
	{
		return IsActive() && GetIsEnabled() && IsVisible();
	}

	//
	// Own methods
	//

	// used as Enabled/Disabled for normal buttons, Checked/Unchecked for checkboxes
	UFUNCTION(BlueprintCallable, Category = ">State")
	void Activate(bool bPlaySound = false) { SetIsActive(true, bPlaySound); }

	UFUNCTION(BlueprintCallable, Category = ">State")
	void Deactivate(bool bPlaySound = false) { SetIsActive(false, bPlaySound); }

	UFUNCTION(BlueprintPure, Category = ">State", meta=(ScriptName="GetIsActive"))
	bool IsActive() const { return bIsActive; }

	// NOTE: Not BP on purpose
	UFUNCTION(BlueprintCallable, Category = ">State")
	virtual void SetIsActive(bool bActive, bool bPlaySound = false);

	// Events
	FSoUIUserWidgetChangedEventCPP& OnActiveChangedEvent() { return ProxyActiveChangedCPP; }

protected:
	//
	// Handle CPP proxies
	//
	UFUNCTION()
	void OnProxyActiveChangedEvent(bool bNewValue) { ProxyActiveChangedCPP.Broadcast(bNewValue); }

public:
	//
	// Handle BP events and events firing
	//

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", DisplayName = "OnActiveChanged")
	void ReceiveOnActiveChanged(bool bActive);
	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnActiveChanged(bool bActive)
	{
		ReceiveOnActiveChanged(bActive);
		ActiveChangedEvent.Broadcast(bActive);
		OnStateChanged();
	}

protected:

	/**
	 * Can be used for Toggleable buttons, or for disabled ones if they need special status
	 * Can be combined with bHighlighted to generate 4 different styles
	 * not all button need to use this
	 * Can be used for check boxes to verify if it is checked
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">State")
	bool bIsActive = false;

	//
	// Events
	//


	// Fired when the Active value of this widget changed
	UPROPERTY(BlueprintAssignable, Category = ">Events", DisplayName = "OnActiveChangedEvent")
	FSoUIUserWidgetChangedEvent ActiveChangedEvent;

	// Proxy events that pass from Dynamic event to our event handler in CPP
	FSoUIUserWidgetChangedEventCPP ProxyActiveChangedCPP;


	//
	// SFX
	//

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXActiveChanged = nullptr;
};
