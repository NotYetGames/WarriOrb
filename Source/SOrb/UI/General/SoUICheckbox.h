// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoUIUserWidget.h"

#include "SoUICheckbox.generated.h"

class UImage;

/**
 * Checkbox primitive
 * NOTE: we use the bIsActive from the base widget as our bIsChecked
 */
UCLASS(HideCategories = ("Navigation"))
class SORB_API USoUICheckbox : public USoUIUserWidget
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;

	//
	// USoUIUserWidget Interface
	//
	void OnStateChanged() override
	{
		Super::OnStateChanged();
		UpdateFromCurrentState();
	}
	bool IsValidWidget() const override
	{
		return GetIsEnabled() && IsVisible();
	}

	//
	// Own Methods
	//

	/**  Checked/Unchecked for checkboxes */
	UFUNCTION(BlueprintCallable, Category = State)
	void SetIsChecked(bool bActive, bool bPlaySound = false) { SetIsActive(bActive, bPlaySound); }

	UFUNCTION(BlueprintCallable, Category = State)
	bool IsChecked() const { return IsActive(); }

	UFUNCTION(BlueprintCallable, Category = State)
	void UpdateFromCurrentState();

protected:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = State)
	void ScaleSize(float ScaleAmount);
	void ScaleSize_Implementation(float ScaleAmount) {}

protected:
	// The X Image. Used when IsChecked() = true
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* XImage = nullptr;

	// Used when bIsHighlighted = true;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* SelectionImage = nullptr;
};
