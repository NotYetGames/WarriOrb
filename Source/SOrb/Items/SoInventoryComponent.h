// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Items/SoItem.h"
#include "SoInventoryComponent.generated.h"

enum class ESoItemType : uint8;
enum class ESoItemSlot : uint8;
class USoQuestItemTemplate;
class USoItemTemplate;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoItemChange, int32, ItemIndex);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SORB_API USoInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoInventoryComponent();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Loads the current items in ListName from the WorldState
	void Reload();

	// Saves the current items to the WorldState ListName
	void Save();

	UFUNCTION(BlueprintPure, Category = Item)
	int32 CalcItemCount(ESoItemType Type) const;

	UFUNCTION(BlueprintPure, Category = Item)
	int32 CalcItemCountFromTemplate(USoItemTemplate* Template) const;

	UFUNCTION(BlueprintPure, Category = Item)
	int32 CalcItemCountForSlot(ESoItemSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = Item)
	int32 AddItem(const FSoItem& ItemDesc);

	UFUNCTION()
	bool RemoveAndGetItem(int32 SlotIndex, int32 Amount, FSoItem& OutRemovedItem);

	UFUNCTION(BlueprintCallable, Category = Item)
	bool RemoveItem(int32 SlotIndex, int32 Amount)
	{
		FSoItem Ignored;
		return RemoveAndGetItem(SlotIndex, Amount, Ignored);
	}

	void ModifyItemCount(int32 ItemIndex, bool bDelta, int32 Amount = 1);

	UFUNCTION(BlueprintPure, Category = Item)
	bool HasQuestItem(USoQuestItemTemplate* QuestItem) const;

	UFUNCTION(BlueprintPure, Category = Item)
	bool HasItem(USoItemTemplate* Template) const;

	USoItemTemplate* FindQuestItem(FName QuestItemBoolValueName);

	/** returns the first item index matching the input template, or -1 if there isn't any */
	UFUNCTION(BlueprintPure, Category = Item)
	int32 GetIndexFromTemplate(const USoItemTemplate* Template) const;

	bool IsValidIndex(int32 Index) { return Index >= 0 && Index < ItemList.Num(); }
	const FSoItem& GetItem(int32 SlotIndex);

	void OrderRuneStones();


	UFUNCTION(BlueprintCallable, Category = Item)
	const TArray<FSoItem>& GetItemList() const { return ItemList; }

	TArray<FSoItem>& GetItemList() { return ItemList; }

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FSoItem> ItemList;
};
