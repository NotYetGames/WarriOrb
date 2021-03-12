// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"

#include "SoSimulated.generated.h"



UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoSimulated : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class SORB_API ISoSimulated
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Triggerable)
	void OnPlayerBounce(const FVector& PlayerVelocity, const FHitResult& HitData);
};
