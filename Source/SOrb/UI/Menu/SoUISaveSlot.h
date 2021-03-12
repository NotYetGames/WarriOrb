// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/General/SoUIUserWidget.h"

#include "SoUISaveSlot.generated.h"

class UTextBlock;
class UImage;

/**
 *
 */
UCLASS()
class SORB_API USoUISaveSlot : public USoUIUserWidget
{
	GENERATED_BODY()

public:

	//
	// Begin UUserWidget Interface
	//
	// void NativeConstruct() override;
	// void NativeDestruct() override;


	//
	// Own methods
	//
	void InitializeAsNew();
	void InitializeFromState(const struct FSoStateTable& State, bool bCurrentlyLoaded);


protected:

	UFUNCTION(BlueprintCallable)
	void SetRunningSlotElementsVisibility(bool bVisible);

	UFUNCTION(BlueprintCallable)
	void SetNotStartedSlotElementsVisibility(bool bVisible);

	UFUNCTION(BlueprintCallable)
	void SetFinishedSlotElementsVisibility(bool bVisible);

	UFUNCTION(BlueprintCallable)
	void HideAll()
	{
		SetRunningSlotElementsVisibility(false);
		SetNotStartedSlotElementsVisibility(false);
		SetFinishedSlotElementsVisibility(false);
	}

protected:

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* BG;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* BGNew;

	//
	// FinishedSlotElements
	//

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* FinishedProgressText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* FinishedDifficulty;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* FinishedTime;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* FinishedDeathCountText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* FinishedDeathCountIcon;

	//
	// RunningSlotElements
	//

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* ChapterText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* LoadedText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* ProgressText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* DifficultyText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* PlayTimeText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* DeathCountText;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* DeathCountIcon;


	UPROPERTY(EditAnywhere, Category = ">Style")
	FSlateColor AreaTextColorActiveSlot;

	UPROPERTY(EditAnywhere, Category = ">Style")
	FSlateColor AreaTextColorDefault;


	UPROPERTY(EditAnywhere, Category = ">Style")
	FSlateColor DifficultyColorSane;

	UPROPERTY(EditAnywhere, Category = ">Style")
	FSlateColor DifficultyColorIntended;

	UPROPERTY(EditAnywhere, Category = ">Style")
	FSlateColor DifficultyColorInsane;

	//
	// Empty Slot elements
	//
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* NewGameText;

	UPROPERTY(EditAnywhere, Category = ">Widgets")
	UTexture2D* EmptyBackground;
};
