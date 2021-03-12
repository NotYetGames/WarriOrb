// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIUserWidgetDeviceVisibility.h"

#include "Settings/Input/SoInputHelper.h"
#include "Character/SoPlayerController.h"
#include "UI/SoUIHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetDeviceVisibility::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetDeviceVisibility::NativeConstruct()
{
	Super::NativeConstruct();

	SubscribeToDeviceChanged();
	SetLocalDeviceType(USoInputHelper::GetCurrentDeviceType(this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetDeviceVisibility::NativeDestruct()
{
	UnSubscribeFromDeviceChanged();
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetDeviceVisibility::SubscribeToDeviceChanged()
{
	if (bIsListeningToDevicesChanges)
		return;

	if (ASoPlayerController* Controller = USoUIHelper::GetSoPlayerControllerFromUWidget(this))
	{
		Controller->OnDeviceTypeChanged().AddDynamic(this, &ThisClass::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidgetDeviceVisibility::UnSubscribeFromDeviceChanged()
{
	if (!bIsListeningToDevicesChanges)
		return;

	if (ASoPlayerController* Controller = USoUIHelper::GetSoPlayerControllerFromUWidget(this))
	{
		Controller->OnDeviceTypeChanged().RemoveDynamic(this, &ThisClass::HandleDeviceTypeChanged);
		bIsListeningToDevicesChanges = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidgetDeviceVisibility::CanBeVisibleOnCurrentDeviceType() const
{
	// Tooltip visibility does not match with the device type, hide widget
	const bool bShouldBeKeyboard = WhenToShowVisibility == ESoUIUserWidgetDeviceVisibilityType::Keyboard && CachedDeviceType != ESoInputDeviceType::Keyboard;
	const bool bShouldBeGamepad = WhenToShowVisibility == ESoUIUserWidgetDeviceVisibilityType::Gamepad && !USoInputHelper::IsInputDeviceTypeGamepad(CachedDeviceType);
	if (bShouldBeKeyboard || bShouldBeGamepad)
	{
		return false;
	}

	// Default is all
	return true;
}
