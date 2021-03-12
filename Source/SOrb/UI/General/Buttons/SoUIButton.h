// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/TextLayout.h"

#include "UI/General/SoUIUserWidget.h"

#include "SoUIButton.generated.h"

class UTexture2D;
class UImage;
class UTextBlock;

/**
 *  Primitive UI element, usually all it has is a text and a background image/color
 *  Can have 2x2 different states, mostly used in SoUIButtonArray
 */
UCLASS()
class SORB_API USoUIButton : public USoUIUserWidget
{
	GENERATED_BODY()

public:
	USoUIButton(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;

	//
	// Own methods
	//

	UFUNCTION(BlueprintCallable, Category = Data)
	void SetButtonText(FText Text);

	UFUNCTION(BlueprintCallable, Category = Data)
	FText GetButtonText() const;

	UFUNCTION(BlueprintCallable, Category = Data)
	virtual void SetOverrideBackgroundImage(UTexture2D* NewImage, bool bInMatchSize = false)
	{
		OverrideBackgroundImage = NewImage;
		bOverrideBackgroundImageMatchSize = bInMatchSize;
		UpdateBackgroundImage();
	}

	//
	// Setters
	//

	ThisClass* SetShowText(bool bValue)
	{
		bShowText = bValue;
		return this;
	}
	ThisClass* SetShowBackgroundImage(bool bValue)
	{
		bShowBackgroundImage = bValue;
		return this;
	}

	void SetTextAlignment(TEnumAsByte<ETextJustify::Type> TextAlignment);

	virtual void UpdateAll()
	{
		UpdateBackgroundImage();
		UpdateText();
		InvalidateLayoutAndVolatility();
	}
	virtual void UpdateBackgroundImage();
	virtual void UpdateText();

protected:
	// If this is set it will use this instead of the default one used for the background image in editor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Override")
	UTexture2D* OverrideBackgroundImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Override")
	bool bOverrideBackgroundImageMatchSize = false;

	// Override the ElementText
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Override")
	FText OverrideText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Visiblity")
	bool bShowBackgroundImage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Visiblity")
	bool bShowText = true;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* ElementText = nullptr;

	// If any
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* BackgroundImage = nullptr;
};
