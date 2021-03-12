// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUITradePanel.h"

#include "DlgDialogueParticipant.h"

#include "Items/SoTraderComponent.h"
#include "UI/SoUIHelper.h"
#include "Items/SoItem.h"
#include "Items/SoInventoryComponent.h"
#include "Items/ItemTemplates/SoItemTemplate.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameInstance.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Basic/SoAudioManager.h"

#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/SoUITypes.h"
#include "UI/General/SoUIConfirmPanel.h"
#include "Online/Analytics/SoAnalytics.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUITradePanel::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	Categories = { ESoItemType::EIT_Weapon,
				   ESoItemType::EIT_Shard,
				   ESoItemType::EIT_Jewelry,
				   ESoItemType::EIT_Key,
				   ESoItemType::EIT_RuneStone };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUITradePanel::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	if (bOpened && bEnable)
		return true;

	if (bEnable)
	{
		TraderComponent = Cast<USoTraderComponent>(Source);
		if (TraderComponent == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to start trading - TradePanel expected a tradecomponent!"));
			return false;
		}

		SubMenus->SetSelectedIndex(0);
		OnActiveSubPanelChanged(0, false);
		OnGoldChanged();

		USoAudioManager::PlaySound2D(this, SFXOpen);
	}
	else
	{
		// restore music
		ASoCharacter* PlayerCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (PlayerCharacter != nullptr)
			PlayerCharacter->UpdateMusic(true);

		USoAudioManager::PlaySound2D(this, SFXClose);
	}

	bOpened = bEnable;
	SetVisibility(bEnable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	return bEnable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUITradePanel::HandleCommand_Implementation(ESoUICommand Command)
{
	Command = USoUIHelper::TryTranslateMenuCommandDirectionToGame(Command);
	if (ISoUIEventHandler::Execute_IsOpened(ConfirmPanel))
	{
		ISoUIEventHandler::Execute_OnUICommand(ConfirmPanel, Command);
		return true;
	}

	const int32 OldPanelIndex = SubMenus->GetSelectedIndex();
	switch (Command)
	{
		case ESoUICommand::EUC_TopLeft:
			SubMenus->Navigate(ESoUICommand::EUC_Left);
			OnActiveSubPanelChanged(SubCategories->GetSelectedIndex(), false);
			OnSelectedItemChanged(static_cast<ESoTraderSubPanel>(SubMenus->GetSelectedIndex()));
			if (OldPanelIndex != SubMenus->GetSelectedIndex())
				USoAudioManager::PlaySound2D(this, SFXPanelSwitch);
			break;

		case ESoUICommand::EUC_TopRight:
			SubMenus->Navigate(ESoUICommand::EUC_Right);
			OnActiveSubPanelChanged(SubCategories->GetSelectedIndex(), false);
			OnSelectedItemChanged(static_cast<ESoTraderSubPanel>(SubMenus->GetSelectedIndex()));
			if (OldPanelIndex != SubMenus->GetSelectedIndex())
				USoAudioManager::PlaySound2D(this, SFXPanelSwitch);
			break;

		case ESoUICommand::EUC_Left:
		case ESoUICommand::EUC_Right:
			SubCategories->Navigate(Command);
			ChangeCategoryBP(GetItemTypeFromIndex(SubCategories->GetSelectedIndex()));
			OnSelectedItemChanged(static_cast<ESoTraderSubPanel>(SubMenus->GetSelectedIndex()));
			break;

		case ESoUICommand::EUC_Up:
		case ESoUICommand::EUC_Down:
			ISoUIEventHandler::Execute_OnUICommand(ItemList, Command);
			OnSelectedItemChanged(static_cast<ESoTraderSubPanel>(SubMenus->GetSelectedIndex()));
			return true;

		case ESoUICommand::EUC_Action0:
			if (CanExecuteTransaction())
			{
				const int32 ActivePanelIndex = SubMenus->GetSelectedIndex();
				UFMODEvent* Events[] = { SFXBuy, SFXSell, SFXBuyBack };
				USoAudioManager::PlaySound2D(this, Events[ActivePanelIndex]);
				ISoUIEventHandler::Execute_Open(ConfirmPanel, true);
			}
			break;

		case ESoUICommand::EUC_MainMenuBack:
			bOpened = false;
			break;

		default:
			break;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUITradePanel::OnActiveSubPanelChanged(int32 PreferedCategory, bool bTryToKeepItemIndex)
{
	const int32 SubPanelNum = static_cast<int32>(ESoTraderSubPanel::ETSP_NumOf);

	check(TraderComponent);
	USoInventoryComponent* PlayerInventory = USoUIHelper::GetPlayerInventory(this);
	check(PlayerInventory);

	const TArray<FSoItem>* Lists[] = { &TraderComponent->GetActualList(), &PlayerInventory->GetItemList(), &TraderComponent->GetPurchasedList() };
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bActivate = Lists[i]->Num() > 0;
		bActivate ? SubMenus->ActivateChildAt(i) : SubMenus->DeactivateChildAt(i);
	}

	SubMenus->SelectFirstActiveChild(SubMenus->GetSelectedIndex());
	const int32 SubMenuIndex = FMath::Max(SubMenus->GetSelectedIndex(), 0);

	SubCategories->DeactivateAllChildren();
	for (const FSoItem& Item : *Lists[SubMenuIndex])
		if (Item.Template != nullptr)
			SubCategories->ActivateChildAt(GetCategoryIndexFromItemType(Item.Template->GetItemType()));

	const int32 CategoryIndex = SubCategories->SelectFirstActiveChild(PreferedCategory);

	ReinitializeItemListBP(*Lists[SubMenuIndex],
						   GetItemTypeFromIndex(CategoryIndex),
						   bTryToKeepItemIndex,
						   SubMenuIndex == static_cast<int32>(ESoTraderSubPanel::ETSP_Buy));
	OnSelectedItemChanged(static_cast<ESoTraderSubPanel>(SubMenuIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoItemType USoUITradePanel::GetItemTypeFromIndex(int32 Index) const
{
	if (Categories.IsValidIndex(Index))
		return Categories[Index];

	return ESoItemType::EIT_Weapon;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoUITradePanel::GetCategoryIndexFromItemType(ESoItemType ItemType) const
{
	return FMath::Max(Categories.Find(ItemType), 0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUITradePanel::Update_Implementation(float DeltaSeconds)
{
	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUITradePanel::IsInConfirmationPanel() const
{
	return ConfirmPanel->IsVisible();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUITradePanel::CanExecuteTransaction()
{
	const int32 ActivePanelIndex = SubMenus->GetSelectedIndex();
	if (ActivePanelIndex < 0 || ActivePanelIndex >= static_cast<int32>(ESoTraderSubPanel::ETSP_NumOf))
		return false;

	AActor* PlayerCharacter = USoStaticHelper::GetPlayerCharacterAsActor(this);

	const FSoItem Item = GetSelectedItem();
	if (Item.Template == nullptr || PlayerCharacter == nullptr)
		return false;

	switch (static_cast<ESoTraderSubPanel>(ActivePanelIndex))
	{
		case ESoTraderSubPanel::ETSP_BuyBack:
			return Item.GetValue(false) <= IDlgDialogueParticipant::Execute_GetIntValue(PlayerCharacter, USoStaticHelper::NameGold);

		case ESoTraderSubPanel::ETSP_Buy:
			return Item.GetValue(true) <= IDlgDialogueParticipant::Execute_GetIntValue(PlayerCharacter, USoStaticHelper::NameGold);

		case ESoTraderSubPanel::ETSP_Sell:
			return true;

		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoTraderSubPanel USoUITradePanel::GetActivePanel() const
{
	return static_cast<ESoTraderSubPanel>(SubMenus->GetSelectedIndex());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUITradePanel::ExecuteTransaction(int32 Amount)
{
	const int32 SubPanelIndex = SubMenus->GetSelectedIndex();
	if (SubPanelIndex < 0 || SubPanelIndex >= static_cast<int32>(ESoTraderSubPanel::ETSP_NumOf))
		return;

	USoInventoryComponent* PlayerInventory = USoUIHelper::GetPlayerInventory(this);
	ASoCharacter* PlayerCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);

	switch (static_cast<ESoTraderSubPanel>(SubPanelIndex))
	{
		case ESoTraderSubPanel::ETSP_Buy:
		{
			const FSoItem Item = TraderComponent->PurchaseItem(GetSelectedItemIndex(), Amount);
			if (Item.Template != nullptr)
			{
				PlayerCharacter->AddItem(Item, false);
				const int32 GoldAmount = -Item.GetValue(true) * FMath::Max(Amount, 1);
				IDlgDialogueParticipant::Execute_ModifyIntValue(PlayerCharacter, USoStaticHelper::NameGold, true, GoldAmount);

				// Analytics
				USoGameInstance::Get(this).GetAnalytics()->SubtractGold(GoldAmount, ESoResourceItemType::ERI_BuyTrader, Item.Template->GetItemTypeAsFriendlyString());

				OnItemBought();
			}
		}
		break;

		case ESoTraderSubPanel::ETSP_BuyBack:
		{
			const FSoItem Item = TraderComponent->BuyBackItem(GetSelectedItemIndex(), Amount);
			if (Item.Template != nullptr)
			{
				PlayerCharacter->AddItem(Item, false);
				const int32 GoldAmount = -Item.GetValue(false) * FMath::Max(Amount, 1);
				IDlgDialogueParticipant::Execute_ModifyIntValue(PlayerCharacter, USoStaticHelper::NameGold, true, GoldAmount);

				// Analytics
				USoGameInstance::Get(this).GetAnalytics()->SubtractGold(GoldAmount, ESoResourceItemType::ERI_BuyTrader, Item.Template->GetItemTypeAsFriendlyString());
			}
		}
		break;

		case ESoTraderSubPanel::ETSP_Sell:
		{
			const int32 ItemIndex = GetSelectedItemIndex();
			if (ItemIndex >= 0 && ItemIndex < PlayerInventory->GetItemList().Num())
			{
				FSoItem Item;
				if (!PlayerInventory->RemoveAndGetItem(GetSelectedItemIndex(), Amount, Item))
					break;

				PlayerCharacter->GetPlayerCharacterSheet()->OnItemRemoved(Item);

				const int32 GoldAmount = Item.GetValue(false) * FMath::Max(Amount, 1);
				IDlgDialogueParticipant::Execute_ModifyIntValue(PlayerCharacter, USoStaticHelper::NameGold, true, GoldAmount);
				TraderComponent->OnItemSold(Item, Amount);

				// Analytics
				USoGameInstance::Get(this).GetAnalytics()->AddGold(GoldAmount, ESoResourceItemType::ERI_SellTrader, Item.Template->GetItemTypeAsFriendlyString());
			}
		}
		break;

		default:
			break;
	}

	OnActiveSubPanelChanged(SubCategories->GetSelectedIndex(), true);
	OnGoldChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoUITradePanel::GetGoldName()
{
	return USoStaticHelper::NameGold;
}
