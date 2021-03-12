// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "UObject/Object.h"
#include "Items/SoItemTypes.h"

#include "SoItemTemplate.generated.h"

class UTexture2D;
class USoEffectBase;

/**
 *
 */
UCLASS(BlueprintType, Abstract)
class SORB_API USoItemTemplate : public UObject
{
	GENERATED_BODY()

public:

	USoItemTemplate();

#if WITH_EDITOR
	void PreSave(const ITargetPlatform* TargetPlatform) override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	//~ Begin UObject Interface.
	/** @return a one line description of an object for viewing in the thumbnail view of the generic browser */
	FString GetDesc() override
	{
		return FString::Printf(TEXT("Item Name = %s\nType = %s"), *Name.ToString(), *GetSubTypeText().ToString());
	}


	static USoItemTemplate* GetTemplateFromPath(const FString& Path);

	FString GetObjectPath() const;


	UFUNCTION(BlueprintCallable, Category = Equipment)
	virtual bool IsSlotCompatible(ESoItemSlot Slot) const { return false; }

	UFUNCTION(BlueprintCallable, Category = Equipment)
	virtual bool IsStackable() const { return false; }

	UFUNCTION(BlueprintCallable, Category = Equipment)
	virtual int32 GetMaxStackNum() const { return 1; }

	UFUNCTION(BlueprintCallable, Category = Equipment)
	virtual bool CanBePutTogether(const USoItemTemplate* TheOther) const { return false; }

	UFUNCTION(BlueprintCallable, Category = Equipment)
	virtual bool IsEquipable() const { return false; }

	// weather or not to destroy the item if the amount is 0
	virtual bool DestroyOnRunOut() const { return true; }

	UFUNCTION(BlueprintCallable, Category = Equipment)
	virtual ESoItemSlot GetDefaultSlot() const { return ESoItemSlot::EIS_Max; }

	UFUNCTION(BlueprintPure, Category = Equipment)
	virtual const FText& GetSubTypeText() const;

	UFUNCTION(BlueprintPure, Category = Equipment)
	virtual bool HasPropertyBlock(ESoItemPropertyBlock Block) const { return false; }

	UTexture2D* GetIcon() const { return Icon; }

	UFUNCTION(BlueprintCallable, Category = UI)
	virtual void FillTooltipProperties(UObject* Tooltip) const {};

	UFUNCTION(BlueprintCallable, Category = Equipment)
	virtual bool CanEquip(UObject* ItemOwner) const { return true; }

	/** effects active while the item is active, some type does not have this */
	virtual void GetStaticEffects(TArray<TSubclassOf<USoEffectBase>>& OutEffects) const {};

	ESoItemType GetItemType() const { return ItemType; }
	FString GetItemTypeAsFriendlyString() const;

	FText GetItemName() const { return Name; }
	int32 GetValue() const { return Value; }

	virtual void UpdateLocalizedFields();

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Item, AssetRegistrySearchable)
	FText Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Item, AssetRegistrySearchable, Meta = (MultiLine = true))
	FText Description;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Item, AssetRegistrySearchable)
	int32 Value = 5;

	//	Level	Text						First Chapter
	//
	//		0	Better than nothing, right?		1
	//		1	Better than nothing				1
	//		2	Good Stuff						2
	//		3	Pretty Good Stuff				2
	//		4	Amazing							3
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Item, AssetRegistrySearchable)
	int32 RarityValue = 1;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = Item, AssetRegistrySearchable)
	ESoItemType ItemType;

	// in inventory / character sheet / etc.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Visual)
	UTexture2D* Icon;

	// Used for all FTexts
	static FString TextNamespace;
};
