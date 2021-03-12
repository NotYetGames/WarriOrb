// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTemplate.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "SoItemTemplateShard.generated.h"

class USoEffectBase;

/**
*
*/
UCLASS()
class SORB_API USoItemTemplateShard : public USoItemTemplate
{
	GENERATED_BODY()

public:

	USoItemTemplateShard() { ItemType = ESoItemType::EIT_Shard; }

	bool IsEquipable() const override { return true; }

	bool IsSlotCompatible(ESoItemSlot Slot) const override;
	ESoItemSlot GetDefaultSlot() const override;

	const FText& GetSubTypeText() const override;

	void FillTooltipProperties(UObject* Tooltip) const override;

	void GetStaticEffects(TArray<TSubclassOf<USoEffectBase>>& OutEffects) const override { OutEffects = Effects; }

	ESoShardType GetClothType() const { return ShardType; }


protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemCloth, AssetRegistrySearchable)
	ESoShardType ShardType;

	// TODO: check effect type, force item based
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemCloth)
	TArray<TSubclassOf<USoEffectBase>> Effects;
};
