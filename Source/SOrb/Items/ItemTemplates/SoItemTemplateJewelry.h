// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTemplate.h"

#include "SoItemTemplateJewelry.generated.h"


UCLASS()
class SORB_API USoItemTemplateJewelry : public USoItemTemplate
{
	GENERATED_BODY()

public:

	USoItemTemplateJewelry() { ItemType = ESoItemType::EIT_Jewelry; }

	virtual bool IsStackable() const override { return MaxStackNum > 1; }
	virtual int32 GetMaxStackNum() const override { return MaxStackNum; }

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxStackNum = 1;
};
