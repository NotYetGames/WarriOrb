// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "UI/General/SoUITypes.h"
#include "Localization/SoLocalizationHelper.h"

#include "SoUILanguageSelection.generated.h"


class USoUIButtonArray;
class USoUIButton;
class USoUICommandTooltipArray;
class USoUIConfirmPanel;
class UImage;
class UTextBlock;
class UFMODEvent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoLanguageSelected);


UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUILanguageSelection : public UUserWidget, public ISoUIEventHandler
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// ISoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override;
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool CanBeInterrupted_Implementation() const override { return true; }

	void CreateButtonTexts();
	void FireOnLanguageSelected();

public:
	FSoLanguageSelected& OnLanguageSelectedEvent() { return LanguageSelectedEvent; }
	bool CanHandleInput() const;
	
protected:
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoLanguageSelected LanguageSelectedEvent;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* LanguageButtons = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Language")
	bool bOpened = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ">Language")
	bool bLanguageSelected = false;

	// Used to know  the status of the parent widget animations
	UPROPERTY(BlueprintReadWrite, Category = ">Runtime")
	UUserWidget* ParentUserWidget;
	
	// Cached values
	TArray<ESoSupportedCulture> SupportedCultures;
};
