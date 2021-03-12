// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "UObject/Interface.h"

#include "SoItemTooltipUI.generated.h"

/**
 *
 */
UINTERFACE(BlueprintType, Blueprintable)
class SORB_API USoItemTooltipUI : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


/**
 *
 */
class SORB_API ISoItemTooltipUI
{
	GENERATED_IINTERFACE_BODY()

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemTooltip)
	void AddDamageLine(ESoDmgType Type, int32 Value, bool bShowDamageIcon, bool bDualWield, bool bAsBonus);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemTooltip)
	void AddParamWithFloatLine(ESoItemParam Param, float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemTooltip)
	void AddParamWithIntLine(ESoItemParam Param, int32 Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemTooltip)
	void AddParamWithBoolLine(ESoItemParamBool Param, bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemTooltip)
	void AddSpecialLine(USoCharacterStrike* CharacterStrike);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemTooltip)
	void AddEffectLine(const USoEffectBase* Effect, bool bFirstLine);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ItemTooltip)
	void AddEmptyLine();
};
