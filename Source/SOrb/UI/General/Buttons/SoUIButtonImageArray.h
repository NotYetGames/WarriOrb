// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "SoUIButtonArray.h"

#include "SoUIButtonImageArray.generated.h"

class UImage;
class UTextBlock;
class USoUIButtonImage;




UCLASS()
class SORB_API USoUIButtonImageArray : public USoUIButtonArray
{
	GENERATED_BODY()
public:
	USoUIButtonImageArray(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;


	//
	// USoUIUserWidgetBaseNavigation interface
	//
	bool IsValidWidget() const override
	{
		return Super::IsValidWidget() && CreatedCommandsMap.Num() > 0;
	}
	
	//
	// USoUIButtonArray interface
	//
	void EmptyContainerChildren(bool bContainer = false) override
	{
		Super::EmptyContainerChildren(bContainer);
		CreatedCommandsMap.Empty();
	}

	//
	// Own methods
	//

	UFUNCTION(BlueprintCallable, Category = ">UI")
	USoUIButtonImage* GetOrCreateButtonImage(bool bTryCache = true);

	// This updates the tooltip array without making unnecessary modifications, it ensures the commands in the input map always exists
	// If there was none to display, we simply hide this widget
	UFUNCTION(BlueprintCallable, Category = ">UI")
	void UpdateEnsureOnlyTheseCommandsExist(const TArray<FSoUITextCommandPair> CommandsToBeDisplayed, bool bAllowEmpty = false);

	// Button must already exist
	UFUNCTION(BlueprintCallable, Category = ">UI")
	bool SetButtonTextForUICommand(FText InText, ESoUICommand Command);

	UFUNCTION(BlueprintCallable, Category = ">UI")
	void AddButtonForUICommand(FText InText, ESoUICommand Command);

	UFUNCTION(BlueprintCallable, Category = ">UI")
	void RemoveButtonForUICommand(ESoUICommand Command);

	UFUNCTION(BlueprintCallable, Category = ">UI")
	void EmptyAllUIButtons();

	UFUNCTION(BlueprintCallable, Category = ">UI")
	void HideIfEmpty()
	{
		if (IsEmpty())
			SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION(BlueprintCallable, Category = ">UI")
	void ClearEditorPreviewButtons()
	{
		if (CreatedCommandsMap.Num() > 0)
			return;

		EmptyAllUIButtons();
	}

	UFUNCTION(BlueprintPure, Category = ">UI")
	ESoUICommand GetSelectedButtonCommand() const
	{
		return GetButtonIndexCommand(GetSelectedIndex());
	}

	UFUNCTION(BlueprintPure, Category = ">UI")
	ESoUICommand GetButtonIndexCommand(int32 ButtonIndex) const;
	
protected:
	// keep track of button children created from what, so that we don't refresh this every time
	// Maps UICommand -> Index in ButtonChildren array
	UPROPERTY()
	TMap<ESoUICommand, int32> CreatedCommandsMap;
};
