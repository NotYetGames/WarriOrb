// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "SoUICommandTooltip.h"

#include "SoUICommandTooltipArray.generated.h"

class UTextBlock;
class USpacer;
class UHorizontalBoxSlot;

/**
 * An array of command tooltips with space between them? possibly
 * NOTE: these are not navigable
 * NOTE: this assume horizontal layout for anything else pls modify this class
 */
UCLASS(HideCategories = ("Navigation"))
class SORB_API USoUICommandTooltipArray : public UUserWidget
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;

	//
	// Own methods
	//

	// Clears all the widgets from the WidgetsContainer
	UFUNCTION(BlueprintCallable)
	void Clear();

	// Is this widget empty?
	UFUNCTION(BlueprintPure)
	bool IsEmpty() const;

	// Get or create spacer widget
	// UFUNCTION(BlueprintCallable)
	// USpacer* GetSpacerWidget();

	// Get or create command tooltip widget
	UFUNCTION(BlueprintCallable)
	USoUICommandTooltip* GetOrCreateCommandTooltipWidget(bool bTryCache = true);

	// Adds a command tooltip from the UI command provided
	UFUNCTION(BlueprintCallable)
	void AddTooltipFromUICommand(const FText InText, const ESoUICommand Command);

	// Called after the tooltip is initialized
	UFUNCTION(BlueprintImplementableEvent)
	void OnPostInitializeTooltip(UHorizontalBoxSlot* TooltipSlot, USoUICommandTooltip* TooltipAdded);

	// Called after the tooltip is added to the widget container
	UFUNCTION(BlueprintImplementableEvent)
	void OnPostAddTooltip(UHorizontalBoxSlot* TooltipSlot, USoUICommandTooltip* TooltipAdded);

	UFUNCTION(BlueprintCallable)
	void SetupFromUICommands(const TArray<FSoCommandTooltipData>& Data);

protected:
	void SetHorizontalBoxSlotToDefaults(UHorizontalBoxSlot* HorizontalSlot);

protected:
	// Container for all the tooltip commands
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UHorizontalBox* WidgetsContainer = nullptr;

	/** Class used for creating command tooltips. Used in GetCommandTooltipWidget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Tooltips")
	TSubclassOf<USoUICommandTooltip> CommandTooltipClass;

	// Default Padding for each tooltip
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tooltips)
	FMargin TooltipsPadding;

	// Default size for each tooltip
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Tooltips)
	FSlateChildSize TooltipsSize = FSlateChildSize(ESlateSizeRule::Type::Automatic);

	// Default HorizontalAlignment for each tooltip
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Tooltips)
	TEnumAsByte<EHorizontalAlignment> TooltipsHorizontalAlignment = EHorizontalAlignment::HAlign_Center;

	// Padding for VerticalAlignment each tooltip
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Tooltips)
	TEnumAsByte<EVerticalAlignment> TooltipsVerticalAlignment = EVerticalAlignment::VAlign_Center;

	// Cached command tooltip widgets so that we do not recreate them all the time
	UPROPERTY(BlueprintReadOnly)
	TArray<USoUICommandTooltip*> CachedCommandTooltipWidgets;
};
