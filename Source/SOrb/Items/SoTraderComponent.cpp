// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoTraderComponent.h"

#include "SaveFiles/SoWorldState.h"
#include "Items/ItemTemplates/SoItemTemplate.h"
#include "Basic/SoGameMode.h"
#include "Interactables/SoNPC.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoStaticHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUITraderComponent, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoTraderComponent::USoTraderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTraderComponent::InitializeComponent()
{
	check(GetOwner());

	const FString OwnerName = GetOwner()->GetName();

	PurchasedListName = FName(*(OwnerName + "_Purchased"));
	SoldListName = FName(*(OwnerName + "_Sold"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTraderComponent::BeginPlay()
{
	Super::BeginPlay();

	ASoGameMode::Get(this).OnPostLoad.AddDynamic(this, &USoTraderComponent::OnReload);
	OnReload();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTraderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ASoGameMode::Get(this).OnPostLoad.RemoveDynamic(this, &USoTraderComponent::OnReload);
	Super::EndPlay(EndPlayReason);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTraderComponent::OnReload()
{
	// construct ActualList from StartingList + AlreadySoldItems
	TArray<FSoItem> AlreadySoldItems;

	FSoWorldState::Get().ReadItemList(SoldListName, AlreadySoldItems);
	ActualList.Empty();
	// ActualList still follows the initial order, only the AlreadySoldItems does not, but that one isn't displayed anyway

	ASoNPC* NPC = Cast<ASoNPC>(GetOwner());
	if (NPC == nullptr)
	{
		UE_LOG(LogSoUITraderComponent, Error, TEXT("USoTraderComponent has a non-npc owner %s"), *GetOwner()->GetName());
		return;
	}

	for (const FSoItem& Item : NPC->GetStartingItemList())
	{
		if (Item.Template != nullptr)
		{
			if (!Item.Template->IsStackable())
			{
				if (AlreadySoldItems.RemoveSingleSwap(Item) == 0)
					ActualList.Add(Item);
			}
			else
			{
				FSoItem StillInShopItem = Item;
				for (int32 i = AlreadySoldItems.Num() - 1; i >= 0 && StillInShopItem.Amount > 0; --i)
					if (AlreadySoldItems[i].Template == Item.Template)
					{
						const int32 AmountToSubtract = FMath::Min(AlreadySoldItems[i].Amount, StillInShopItem.Amount);
						StillInShopItem.Amount -= AmountToSubtract;
						AlreadySoldItems[i].Amount -= AmountToSubtract;
					}

				if (StillInShopItem.Amount > 0)
					ActualList.Add(StillInShopItem);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoItem USoTraderComponent::PurchaseItem(int32 ItemIndex, int32 Amount)
{
	if (ItemIndex < 0 || ItemIndex > ActualList.Num())
	{
		UE_LOG(LogSoUITraderComponent, Warning, TEXT("Invalid item was bought?! (OnItemPurchased)"));
		return{};
	}

	FSoItem Item = ActualList[ItemIndex];
	if (Item.Template == nullptr)
		return{};

	if (Item.Template->IsStackable() && Item.Amount != Amount)
	{
		ensure(Item.Amount > Amount);
		ActualList[ItemIndex].Amount -= Amount;
		Item.Amount = Amount;
		FSoWorldState::Get().GetItemList(SoldListName).Add(Item);
	}
	else
	{
		FSoWorldState::Get().GetItemList(SoldListName).Add(Item);
		ActualList.RemoveAt(ItemIndex);
	}
	return Item;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoItem USoTraderComponent::BuyBackItem(int32 ItemIndex, int32 Amount)
{
	TArray<FSoItem>& ItemList =	FSoWorldState::Get().GetItemList(PurchasedListName);
	if (ItemIndex < 0 || ItemIndex > ItemList.Num())
	{
		UE_LOG(LogSoUITraderComponent, Warning, TEXT("Invalid item index?! (BuyBackItem)"));
		return{};
	}

	FSoItem Item = ItemList[ItemIndex];
	if (Item.Template == nullptr)
		return{};

	if (Item.Template->IsStackable() && Item.Amount != Amount)
	{
		ensure(Item.Amount > Amount);
		ItemList[ItemIndex].Amount -= Amount;
		Item.Amount = Amount;
		return Item;
	}

	ItemList.RemoveAt(ItemIndex);
	return Item;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoTraderComponent::OnItemSold(const FSoItem& Item, int32 SoldAmount)
{
	if (Item.Template == nullptr)
		return;

	TArray<FSoItem>& ItemList = FSoWorldState::Get().GetItemList(PurchasedListName);
	if (Item.Template->IsStackable())
	{
		for (FSoItem& ItemInList : ItemList)
			if (ItemInList.Template == Item.Template)
			{
				const int32 CanBeAdded = FMath::Min(Item.Template->GetMaxStackNum() - ItemInList.Amount, SoldAmount);
				ItemInList.Amount += CanBeAdded;
				SoldAmount -= CanBeAdded;
				if (SoldAmount == 0)
					return;
			}

		ItemList.Add(Item);
		ItemList.Last(0).Amount = SoldAmount;
	}
	else
		ItemList.Add(Item);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TArray<FSoItem>& USoTraderComponent::GetPurchasedList()
{
	return FSoWorldState::Get().GetItemList(PurchasedListName);
}
