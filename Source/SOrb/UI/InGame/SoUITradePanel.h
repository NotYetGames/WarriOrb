// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGame/SoUIGameActivity.h"
#include "Items/SoItemTypes.h"
#include "Items/SoItem.h"

#include "SoUITradePanel.generated.h"

class USoUIButtonArray;
class USoUIConfirmPanel;
class USoTraderComponent;
class UFMODEvent;


UENUM(BlueprintType)
enum class ESoTraderSubPanel : uint8
{
	ETSP_Buy				UMETA(DisplayName = "Buy"),
	ETSP_Sell				UMETA(DisplayName = "Sell"),
	ETSP_BuyBack			UMETA(DisplayName = "BuyBack"),
	ETSP_NumOf				UMETA(Hidden)
};

/**
 *
 */
UCLASS()
class SORB_API USoUITradePanel : public USoInGameUIActivity
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;

	bool HandleCommand_Implementation(ESoUICommand Command) override;

	bool Update_Implementation(float DeltaSeconds) override;

	bool ShouldHideUIElements_Implementation() const override { return true; }
	bool ShouldKeepMusicFromPreviousUIActivity_Implementation() const override { return true; }

protected:
	ESoItemType GetItemTypeFromIndex(int32 Index) const;
	int32 GetCategoryIndexFromItemType(ESoItemType ItemType) const;

	void OnActiveSubPanelChanged(int32 PreferedCategory, bool bTryToKeepItemIndex);

	bool IsInConfirmationPanel() const;

	UFUNCTION(BlueprintCallable, Category = Items)
	void ExecuteTransaction(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = Items)
	bool CanExecuteTransaction();

	UFUNCTION(BlueprintPure, Category = Items)
	ESoTraderSubPanel GetActivePanel() const;


	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Items)
	void ReinitializeItemListBP(const TArray<FSoItem>& Items, ESoItemType ItemType, bool bReserveItemIndexIfCategoryIsSame, bool bShowBuyPrice);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Items)
	void ChangeCategoryBP(ESoItemType ItemType);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Items)
	void OnSelectedItemChanged(ESoTraderSubPanel ActivePanel);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Items)
	void OnGoldChanged();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Items)
	int32 GetSelectedItemIndex();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, BlueprintPure, Category = Items)
	FSoItem GetSelectedItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = Items)
	void OnItemBought();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Items)
	FName GetGoldName();

protected:

	// TODO see if we have duplicate sounds

	/** <Buy> <Sell> <Redeem> */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* SubMenus = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* SubCategories = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Widgets", meta = (BindWidget))
	UUserWidget* ItemList = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Widgets", meta = (BindWidget))
	USoUIConfirmPanel* ConfirmPanel = nullptr;

	UPROPERTY(BlueprintReadOnly)
	USoTraderComponent* TraderComponent = nullptr;

	bool bOpened = false;

	UPROPERTY(EditAnywhere, Category = ">Trade")
	TArray<ESoItemType> Categories;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXPanelSwitch = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXBuy = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXSell = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXBuyBack = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOpen = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXClose = nullptr;
};
