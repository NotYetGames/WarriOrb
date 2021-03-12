// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/SoItemTypes.h"

#include "SoUIActiveSpellSlot.generated.h"

class USoItemTemplateRuneStone;
class UImage;
class UTextBlock;

UCLASS()
class SORB_API USoUIActiveSpellSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnSelectedSpellChanged();

	UFUNCTION()
	void OnCharacterItemSlotChanged(ESoItemSlot ChangedSlot);

	UFUNCTION()
	void OnCooldownChanged(int32 Index, float RemainingTime, const UObject* ObjectWithCooldown);

	UFUNCTION()
	void OnSpellUsabilityMightChanged();

protected:
	UFUNCTION(BlueprintCallable, Category = SpellSlot)
	void UpdateVisibility();

	void UpdateInactiveFadeVisibility();
	void Refresh();

protected:
	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UImage* InactiveFade = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* AmountText = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Options", meta = (BindWidget))
	bool bCanBeDisplayed = true;

	UPROPERTY()
	const USoItemTemplateRuneStone* CurrentlySelectedSpell = nullptr;

	bool bRunOut = false;
	bool bCooldownBlocked = false;
};
