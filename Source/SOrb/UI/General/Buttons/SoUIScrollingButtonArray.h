// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SoUIButtonArray.h"

#include "SoUIScrollingButtonArray.generated.h"

class UImage;
class UTextBlock;

/**
 * Like the normal Button array only of this layout
 * |<| |Button text| |>|
 * The active element is the current selected one
 */
UCLASS()
class SORB_API USoUIScrollingButtonArray : public USoUIButtonArray
{
	GENERATED_BODY()
public:
	USoUIScrollingButtonArray(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// USoUIButtonArray Interface
	//
	void CreateButtons(const TArray<FText>& ElementTexts, bool bIgnoreEmptyTexts = false) override;
	bool Navigate(ESoUICommand Command, bool bPlaySound = true) override;
	void SetSelectedIndex(int32 ElementIndex, bool bPlaySound = false) override;

	bool SetIsHighlightedOnChildAt(int32 ChildIndex, bool bInHighlighted, bool bPlaySound = false) override
	{
		return SetIsHighlighted(bInHighlighted, bPlaySound);
	}
	bool SetIsHighlighted(bool bInHighlighted, bool bPlaySound = false) override;
	void SetIsHighlightedOnAllChildren(bool bInHighlighted) override
	{
		SetSelectedChildIsHighlighted(bInHighlighted);
	}
	void OnRequestChildHighlightChange(int32 ChildIndex, bool bHighlight) override;

	bool SelectFirstValidChild(bool bPlaySound = false) override { return false; }
	int32 SelectFirstActiveChild(int32 StartFrom = 0) override;

	/** Active/Inactive state is not used on buttons in ScrollingButtonArray, only Highlighted/Not highlighted */
	bool IsChildActive(int32 ChildIndex) const override;
	bool IsChildHighlighted(int32 ChildIndex) const override;
	bool IsChildEnabled(int32 ChildIndex) const override;

	bool SetIsActiveOnChildAt(int32 ChildIndex, bool bActive, bool bPlaySound = false) override { return true; }
	bool SetIsEnabledOnChildAt(int32 ChildIndex, bool bEnabled) override { return true; }
	void SetIsActiveOnAllChildren(bool bActive) override {}

protected:
	//
	// USoUIButtonArray Interface
	//
	void UpdateWidgetOverrides(UUserWidget* Widget, int32 ChildIndex) override;

	//
	// Own Methods
	//

	// Updates the SelectedButton with the state from the Children[SelectedIndex]
	void UpdateSelectedButton(bool bPlaySound);

	UFUNCTION()
	void OnPressedArrowLeft();
	UFUNCTION()
	void OnPressedArrowRight();

protected:
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* LeftArrowButton = nullptr;

	// The Selected button, kept in sync with the text from Children[SelectedIndex]
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* SelectedButton = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* RightArrowButton = nullptr;
};
