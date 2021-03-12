// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "../SoUIUserWidgetArray.h"

#include "SoUIButtonArray.generated.h"

class USoUIButton;
class UFMODEvent;
class USoUIUserWidget;
class ASoCharacter;

/**
 *  Container class containing an array of SoUIButton
 *  the layout can be ordered horizontally or vertically
 *  it supports selecting/highlighting the buttons, navigation for the active button, etc.
 */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUIButtonArray : public USoUIUserWidgetArray
{
	GENERATED_BODY()

public:
	USoUIButtonArray(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
	void NativeConstruct() override;

	//
	// Own methods
	//

	/**
	 * ButtonClass has to be set before the function is executed
	 * All old children are removed from the container.
	 * Creates new children with the provided texts.
	 */
	UFUNCTION(BlueprintCallable, Category = Initialization)
	virtual void CreateButtons(const TArray<FText>& ElementTexts, bool bIgnoreEmptyTexts = false);

	/** Alternative way to initialize the children via setting the array directly, can be used e.g. from child BP */
	UFUNCTION(BlueprintCallable, Category = Initialization)
	void SetButtons(const TArray<USoUIButton*>& InButtons);

	/**
	 *  (Re)initializes the children texts. Does not modify the state of the Buttons
	 *  ElementTexts.Num does not have to match with Children.Num
	 *  (Out of index texts are ignored, buttons does not necessary have to have text)
	 */
	UFUNCTION(BlueprintCallable, Category = Initialization)
	void SetButtonsTexts(const TArray<FText>& ButtonTexts);

	/** Sets the Text on the Button located at ButtonIndex  */
	bool SetButtonText(int32 ButtonIndex, const FText& Text);
	FText GetButtonText(int32 ButtonIndex);

	// Get the button from ButtonChildren
	UFUNCTION(BlueprintPure, Category = Data)
	virtual USoUIButton* GetContainerButtonAt(int32 ButtonIndex) const;

	virtual void AddContainerButton(USoUIButton* Button);
	virtual void AddContainerButtonWithText(USoUIButton* Button, FText Text);
	virtual void RemoveContainerButton(USoUIButton* Button);

protected:
	void UpdateWidgetOverrides(UUserWidget* Widget, int32 ChildIndex) override;

protected:

	//
	// Override
	//

	// If these textures are set it will use them in a cyclic fashion when adding new buttons.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Override Images")
	TArray<UTexture2D*> OverrideButtonImages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Override Visibility")
	bool bOverrideButtonsVisibility = true;

	// Used only if bOverrideButtonsVisibility is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Override Visibility")
	bool bShowButtonsBackgroundImage = true;

	// Used only if bOverrideButtonsVisibility is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Override Visibility")
	bool bShowButtonsText = true;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Override Visibility")
	bool bOverrideTextAlignment = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Override Visibility")
	TEnumAsByte<ETextJustify::Type> TextAlignmentOverride;
};
