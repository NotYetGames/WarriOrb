// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Items/SoItem.h"
#include "SoTraderComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SORB_API USoTraderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoTraderComponent();

	virtual void InitializeComponent() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void OnReload();

	/** Player bought something */
	UFUNCTION(BlueprintCallable, Category = "Trade")
	FSoItem PurchaseItem(int32 ItemIndex, int32 Amount);

	/** Player bought something back*/
	UFUNCTION(BlueprintCallable, Category = "Trade")
	FSoItem BuyBackItem(int32 ItemIndex, int32 Amount);

	/** Player sold something */
	UFUNCTION(BlueprintCallable, Category = "Trade")
	void OnItemSold(const FSoItem& Item, int32 SoldAmount);

	UFUNCTION(BlueprintCallable, Category = "Trade")
	const TArray<FSoItem>& GetActualList() { return ActualList;	}

	UFUNCTION(BlueprintCallable, Category = "Trade")
	const TArray<FSoItem>& GetPurchasedList();

protected:

	/** the items the merchant can sell right now */
	UPROPERTY(BlueprintReadOnly)
	TArray<FSoItem> ActualList;

	/** bought from player, he can buy back */
	FName PurchasedListName;

	/** already sold items, these are serialized to get the ActualList */
	FName SoldListName;
};
