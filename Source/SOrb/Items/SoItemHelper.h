// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "UObject/Object.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "SoItem.h"

#include "SoItemHelper.generated.h"

class USoWeaponTemplate;

/** because struct can't have functions, that's why */
UCLASS()
class SORB_API USoItemHelper : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = Item)
	static int32 GetValue(const FSoItem& Item, bool bBuyPrice) { return Item.GetValue(bBuyPrice); }

	/**
	 * Loads all Items from the filesystem into memory
	 */
	static void LoadAllItemsIntoMemory();
};
