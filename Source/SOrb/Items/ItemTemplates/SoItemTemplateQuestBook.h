// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTemplate.h"

#include "SoItemTemplateQuestBook.generated.h"


/**
 *  Used for e.g. keys
 *  Item in the inventory, nothing special, but objects can check if the character has one or not
 */
UCLASS()
class SORB_API USoItemTemplateQuestBook : public USoItemTemplate
{
	GENERATED_BODY()

public:
	USoItemTemplateQuestBook() { ItemType = ESoItemType::EIT_QuestItem; }

	const FText& GetSubTypeText() const override;
	void UpdateLocalizedFields() override;

public:
	/** text inside books */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemQuest, Meta = (MultiLine = true))
	TArray<FText> Pages;
};
