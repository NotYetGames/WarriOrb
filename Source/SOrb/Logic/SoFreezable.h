// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"

#include "SoFreezable.generated.h"


/**
 *  Interface to provide a stupid way of communication between different objects
 *  (e.g. a lever can send message to the three-story elevator with the 12 doors to open/close a few)
 *  both C++ and Blueprint classes can implement and use the interface
 */
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoFreezable : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 *  Interface to provide a stupid way of communication between different objects
 *  (e.g. a lever can send message to the three-story elevator with the 12 doors to open/close a few)
 *  both C++ and Blueprint classes can implement and use the interface
 */
class SORB_API ISoFreezable
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Triggerable)
	void OnIceStoneHit(const FHitResult& Hit);
};
