// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Misc/SecureHash.h"

#include "CharacterBase/SoIMortalTypes.h"

#include "SoItem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSoItem, All, All);

class UTexture2D;
class USoItemTemplateRuneStone;
class USoEffectBase;
class USoItemTemplate;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// an item or a stack of items in an inventory or in any other storage
USTRUCT(BlueprintType)
struct FSoItem
{
	GENERATED_USTRUCT_BODY()

public:
	FSoItem() {};

	/** effects active while the item is active, some type does not have this */
	void GetStaticEffects(TArray<TSubclassOf<USoEffectBase>>& OutEffects) const;

	/** Template->GetIcon() or nullptr */
	UTexture2D* GetIcon();

	int32 GetValue(bool bBuyPrice) const;

	// weapon only functions:

	bool operator==(const FSoItem& Other) const;
	void UpdateSHA1(FSHA1& HashState) const;

	// only call on valid RuneStone!!!
	USoItemTemplateRuneStone* GetTemplateAsRuneStone();

	// only call on valid RuneStone!!!
	const USoItemTemplateRuneStone* GetTemplateAsRuneStone() const;

public:
	static FSoItem Invalid;

public:
	/** reference to the data asset which defines this item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoItemTemplate* Template = nullptr;

	/**
	 *  can mean actual amount, or the amount the item can be used in given type - depending on the Template
	 *  item is may or may not destroyed on 0 amount. Max value is also defined by the template (max stack size).
	 *  for most items it's just const 1
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Amount = 1;
};
