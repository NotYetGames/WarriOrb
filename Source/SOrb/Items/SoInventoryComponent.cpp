// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInventoryComponent.h"
#include "SoItem.h"
#include "ItemTemplates/SoQuestItemTemplate.h"
#include "SaveFiles/SoWorldState.h"
#include "Basic/SoGameInstance.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoInventoryComponent::USoInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts
void USoInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInventoryComponent::Save()
{
	FSoWorldState::Get().WriteInventoryItemList(ItemList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInventoryComponent::Reload()
{
	ItemList.Empty();
	FSoWorldState::Get().ReadInventoryItemList(ItemList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoInventoryComponent::AddItem(const FSoItem& Item)
{
	// handle stack
	if (Item.Template != nullptr && Item.Template->IsStackable())
	{
		int32 LastAddedIndex = -1;
		int32 AmountToAdd = Item.Amount;
		for (int32 i = 0; i < ItemList.Num() && AmountToAdd > 0; ++i)
			if (ItemList[i].Template == Item.Template)
			{
				const int32 CanBeAdded = FMath::Max(ItemList[i].Template->GetMaxStackNum() - ItemList[i].Amount, 0);
				ItemList[i].Amount += FMath::Min(AmountToAdd, CanBeAdded);
				AmountToAdd -= CanBeAdded;
				LastAddedIndex = i;
			}

		if (AmountToAdd <= 0)
			return LastAddedIndex;

		ItemList.Add(Item);
		ItemList.Last(0).Amount = AmountToAdd;
		return ItemList.Num() - 1;
	}

	ItemList.Add(Item);
	return ItemList.Num() - 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoInventoryComponent::RemoveAndGetItem(int32 ItemIndex, int32 Amount, FSoItem& OutRemovedItem)
{
	if (!ItemList.IsValidIndex(ItemIndex))
		return false;

	OutRemovedItem = ItemList[ItemIndex];
	if (OutRemovedItem.Template == nullptr || !OutRemovedItem.Template->IsStackable() || OutRemovedItem.Amount == Amount)
		ItemList.RemoveAt(ItemIndex);
	else
	{
		ensure(OutRemovedItem.Amount > Amount);
		ItemList[ItemIndex].Amount -= Amount;
		OutRemovedItem.Amount = Amount;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInventoryComponent::ModifyItemCount(int32 ItemIndex, bool bDelta, int32 Amount)
{
	if (!ItemList.IsValidIndex(ItemIndex) || ItemList[ItemIndex].Template == nullptr)
		return;

	if (ItemList[ItemIndex].Template->DestroyOnRunOut() &&
		((bDelta && ItemList[ItemIndex].Amount <= Amount) || (!bDelta && Amount == 0)))
		RemoveItem(ItemIndex, ItemList[ItemIndex].Amount);
	else
	{
		if (bDelta)
			ItemList[ItemIndex].Amount -= Amount;
		else
			ItemList[ItemIndex].Amount = Amount;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoItem& USoInventoryComponent::GetItem(int32 SlotIndex)
{
	check(SlotIndex >= 0 && SlotIndex < ItemList.Num());
	return ItemList[SlotIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInventoryComponent::OrderRuneStones()
{
	// 1. move runestones to another array
	TArray<FSoItem> RuneStones;
	for (int32 Index = ItemList.Num() - 1; Index >= 0; --Index)
	{
		if (USoItemTemplateRuneStone* RuneStone = Cast<USoItemTemplateRuneStone>(ItemList[Index].Template))
		{
			RuneStones.Add(ItemList[Index]);
			ItemList.RemoveAtSwap(Index);
		}
	}

	// 2. order runtestones
	RuneStones.Sort([](const FSoItem& First, const FSoItem& Second)
	{
		if (First.Template == nullptr || Second.Template == nullptr)
			return false;

		return First.Template->GetName() > Second.Template->GetName();
	});

	// 3. add ordered runestones to the back of ItemList
	ItemList.Append(RuneStones);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoInventoryComponent::HasQuestItem(USoQuestItemTemplate* QuestItem) const
{
	for (const FSoItem& Item : ItemList)
		if (Item.Template == QuestItem)
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoInventoryComponent::HasItem(USoItemTemplate* Template) const
{
	for (const FSoItem& Item : ItemList)
		if (Item.Template == Template)
			return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoItemTemplate* USoInventoryComponent::FindQuestItem(FName QuestItemBoolValueName)
{
	for (int32 i = 0; i < ItemList.Num(); ++i)
	{
		USoQuestItemTemplate* QuestTemplate = Cast<USoQuestItemTemplate>(ItemList[i].Template);
		if (QuestTemplate != nullptr &&QuestTemplate->BoolToAdd == QuestItemBoolValueName)
			return ItemList[i].Template;
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoInventoryComponent::GetIndexFromTemplate(const USoItemTemplate* Template) const
{
	for (int32 i = 0; i < ItemList.Num(); ++i)
		if (ItemList[i].Template == Template)
			return i;

	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoInventoryComponent::CalcItemCount(ESoItemType Type) const
{
	int32 Count = 0;

	for (const FSoItem& Item : ItemList)
		if (Item.Template != nullptr && Item.Template->GetItemType() == Type)
			Count += 1;

	return Count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoInventoryComponent::CalcItemCountFromTemplate(USoItemTemplate* Template) const
{
	int32 Count = 0;

	for (const FSoItem& Item : ItemList)
		if (Item.Template == Template)
			Count += Item.Amount;

	return Count;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoInventoryComponent::CalcItemCountForSlot(ESoItemSlot Slot) const
{
	int32 Count = 0;

	for (const FSoItem& Item : ItemList)
		if (Item.Template != nullptr && Item.Template->IsSlotCompatible(Slot))
			Count += 1;

	return Count;
}
