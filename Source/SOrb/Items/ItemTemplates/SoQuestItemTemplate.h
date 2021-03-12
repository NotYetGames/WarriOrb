// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTemplate.h"

#include "SoQuestItemTemplate.generated.h"


/**
 *  Used for e.g. keys
 *  Item in the inventory, nothing special, but objects can check if the character has one or not
 */
UCLASS()
class SORB_API USoQuestItemTemplate : public USoItemTemplate
{
	GENERATED_BODY()

public:

	USoQuestItemTemplate() { ItemType = ESoItemType::EIT_QuestItem; Value = 0; }

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	const FText& GetSubTypeText() const override;

	virtual bool IsStackable() const override { return MaxStackNum > 0; }
	virtual int32 GetMaxStackNum() const override { return MaxStackNum; }
	virtual bool CanBePutTogether(const USoItemTemplate* TheOther) const override { return TheOther == this; }

private:

	void UpdateItemType();

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemQuest, AssetRegistrySearchable)
	ESoQuestItemType QuestItemType;

	/** name of the bool dialogue variable which is set to true if the item is picked up (must be maxstacknum amount) */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemQuest)
	FName BoolToAdd;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemQuest)
	int32 MaxStackNum = 1;
};

