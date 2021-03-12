// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUITypes.h"
#include "Settings/Input/SoInputSettingsTypes.h"

#include "SoUIUserWidgetDeviceVisibility.generated.h"

class UImage;


UENUM(BlueprintType)
enum class ESoUIUserWidgetDeviceVisibilityType : uint8
{
	All = 0			UMETA(DisplayName = "All"),
	Keyboard		UMETA(DisplayName = "Keyboard only"),
	Gamepad			UMETA(DisplayName = "Gamepad only"),
};

/**
 * Common UserWidget functionality for widgets to have different visibility depending on the device used.
 *
 * This will be visible only if the user allows us to be visible
 * AND our current device type allows us to be visible
 */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIUserWidgetDeviceVisibility : public UUserWidget
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
	// Own Methods
	//

	UFUNCTION(BlueprintCallable, Category = ">Visibility")
	void SetIsListeningToDeviceChanges(bool bIsListening)
	{
		if (bIsListeningToDevicesChanges == bIsListening)
			return;

		if (bIsListeningToDevicesChanges)
			UnSubscribeFromDeviceChanged();
		else
			SubscribeToDeviceChanged();
	}

	virtual void SetDeviceType(ESoInputDeviceType NewDevice)
	{
		SetLocalDeviceType(NewDevice);
	}

	UFUNCTION(BlueprintCallable, Category = ">Visibility")
	void SetWhenToShowVisibility(ESoUIUserWidgetDeviceVisibilityType InVisiblity)
	{
		WhenToShowVisibility = InVisiblity;
		UpdateVisibility();
	}

	// Method that take care of WhenToShowVisibility
	// If it is hidden because the wrong device is handling this method will do nothing if we are trying to hide something
	UFUNCTION(BlueprintCallable, Category = ">Visibility")
	void SetShouldBeVisible(bool bVisible)
	{
		bShouldBeVisible = bVisible;
		UpdateVisibility();
	}

protected:
	// Only update local so that we do not update the image, that is done already
	UFUNCTION(BlueprintCallable, Category = ">Visibility")
	void SetLocalDeviceType(ESoInputDeviceType NewDevice)
	{
		CachedDeviceType = NewDevice;
		UpdateVisibility();
	}

	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType)
	{
		if (!bIsListeningToDevicesChanges)
			return;

		SetLocalDeviceType(InDeviceType);
	}

	void SubscribeToDeviceChanged();
	void UnSubscribeFromDeviceChanged();
	void UpdateVisibility()
	{
		// User allows us to set it visible and container allows us
		const bool bVisible = bShouldBeVisible && CanBeVisibleOnCurrentDeviceType();
		SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	bool CanBeVisibleOnCurrentDeviceType() const;

protected:
	// Tells us where this container is visible
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Visibility")
	ESoUIUserWidgetDeviceVisibilityType WhenToShowVisibility = ESoUIUserWidgetDeviceVisibilityType::All;

	// Set by user in SetShouldBeVisible
	UPROPERTY(BlueprintReadOnly, Category = ">Visibility")
	bool bShouldBeVisible = true;

	// Keep track if we are listening or not
	UPROPERTY(BlueprintReadOnly, Category = ">Device")
	bool bIsListeningToDevicesChanges = false;

	// Cached Current device Type
	ESoInputDeviceType CachedDeviceType = ESoInputDeviceType::Keyboard;
};
