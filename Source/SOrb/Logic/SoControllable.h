// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "SoControllable.generated.h"


UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoControllable : public UInterface
{
	GENERATED_UINTERFACE_BODY()

};


/** Used by some levers to move or rotate objects between to positions */
class SORB_API ISoControllable
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Triggerable)
	void OnControledValueChange(float fNewPercent);
};
