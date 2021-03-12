// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"

#include "SoCooldown.generated.h"

class UTexture2D;

/**
 *  Interface for anything with cooldown: spells/strikes
 */
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoCooldown : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class SORB_API ISoCooldown
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Cooldown)
	float GetCooldownDuration() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Cooldown)
	UTexture2D* GetCooldownIcon() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Cooldown)
	bool CanCountDownInAir() const;
};
