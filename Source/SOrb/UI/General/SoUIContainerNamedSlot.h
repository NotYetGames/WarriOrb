// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoUIUserWidgetDeviceVisibility.h"

#include "SoUIContainerNamedSlot.generated.h"

class UNamedSlot;


/**
 * Container that can contain anything but only displays that widget in case of some input
 * NOTE: this is the generalized version of USoUICommandTooltip
 */
UCLASS()
class SORB_API USoUIContainerNamedSlot : public USoUIUserWidgetDeviceVisibility
{
	GENERATED_BODY()
public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

protected:
	// The slot we want our widget to be in
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UNamedSlot* NamedSlot = nullptr;
};
