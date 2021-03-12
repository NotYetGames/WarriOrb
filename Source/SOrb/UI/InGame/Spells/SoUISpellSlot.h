// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUISpellSlot.generated.h"

class UImage;
class UTextBlock;
class USoItemTemplateRuneStone;
class UTexture2D;

UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUISpellSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSelected(bool bSelected);
	void SetEquipped(bool bEquipped);
	void SetCanBeEquipped(bool bCanBe);
	void SetItem(const USoItemTemplateRuneStone* InActiveItem, int32 InAmount);

	bool IsEquipped() const { return bIsEquipped; }
	int32 GetAmount() const { return Amount; }

	bool ModifyAmount(int32 Delta);

	void ShowBorderIfItemMatches(const USoItemTemplateRuneStone* Item);

	const USoItemTemplateRuneStone* GetItem() const { return ActiveItem; }

protected:

	void OnStateChanged();

protected:

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* Background = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* Border = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* EquippedIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* AmountText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	bool bIsEquipped = false;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	const USoItemTemplateRuneStone* ActiveItem;

	UPROPERTY(EditAnywhere, Category = ">Spell")
	UTexture2D* EmptyBackground;

	UPROPERTY(EditAnywhere, Category = ">Spell")
	UTexture2D* SpellBackground;

	UPROPERTY(EditAnywhere, Category = ">Spell")
	UTexture2D* SpellBackgroundCantEquip;

	UPROPERTY(EditAnywhere, Category = ">Spell")
	FLinearColor CantEquipBorderColor;

	UPROPERTY(EditAnywhere, Category = ">Spell")
	FLinearColor CanEquipBorderColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, Category = ">Spell")
	bool bShowEquippedIcon = true;

	int32 Amount = 0;

	bool bCanBeEquipped;
};
