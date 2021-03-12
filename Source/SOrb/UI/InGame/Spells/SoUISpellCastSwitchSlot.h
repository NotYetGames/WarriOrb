// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"

#include "SoUISpellCastSwitchSlot.generated.h"

class UImage;
class UTexture2D;
class UTextBlock;
class USoItemTemplateRuneStone;

UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUISpellCastSwitchSlot : public UUserWidget
{
	GENERATED_BODY()
	typedef USoUISpellCastSwitchSlot Self;
public:
	// Begin UUserWidget Interface
	void NativeConstruct() override;
	void NativeDestruct() override;
	// End UUserWidget Interface

	void SetSelected(bool bSelected);
	void SetSpell(const USoItemTemplateRuneStone* InActiveItem);
	void Reset();
	void UpdateFromState();

	const USoItemTemplateRuneStone* GetSpell() const { return ActiveSpell; }

protected:
	void SubscribeToEvents();
	void UnsubscribeFromEvents();
	void UpdateInactiveFadeVisibility();

	void RefreshActiveSpellState();

	UFUNCTION()
	void OnCooldownChanged(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown);

	UFUNCTION()
	void OnSpellUsabilityMightChanged();

protected:
	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* Background = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* Border = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bIsSelected = false;

	// Used only if an item is set
	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* InactiveFade = nullptr;

	// Used only if and item is set
	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* AmountText = nullptr;

	// Used only if an item is set
	bool bRunOut = false;
	bool bCooldownBlocked = false;

	// Used only if selected and not empty
	UPROPERTY(BlueprintReadOnly, Category = ">Runtime")
	const USoItemTemplateRuneStone* ActiveSpell;

	UPROPERTY(EditAnywhere, Category = ">Slot")
	UTexture2D* EmptyBackground;

	UPROPERTY(EditAnywhere, Category = ">Slot")
	UTexture2D* SpellBackground;

	UPROPERTY(EditAnywhere, Category = ">Color")
	FLinearColor ActiveSelectedBorderColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = ">Color")
	FLinearColor ActiveUnSelectedBorderColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, Category = ">Color")
	FLinearColor InActiveSelectedBorderColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = ">Color")
	FLinearColor InActiveUnSelectedBorderColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);
};
