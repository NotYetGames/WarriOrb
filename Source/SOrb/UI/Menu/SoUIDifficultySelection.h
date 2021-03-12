// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/General/Buttons/SoUIButtonArray.h"

#include "Basic/SoDifficulty.h"

#include "SoUIDifficultySelection.generated.h"

class USoUIConfirmQuestion;
class URichTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDifficultySelected, ESoDifficulty, SelectedDifficulty);

/**
 *
 */
UCLASS()
class SORB_API USoUIDifficultySelection : public USoUIButtonArray, public ISoUIEventHandler
{
	GENERATED_BODY()

public:

	//
	// Begin UUserWidget Interface
	//
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// ISoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override;
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return IsVisible(); }
	bool CanBeInterrupted_Implementation() const override { return true; }


	//
	// Own methods
	//

	FOnDifficultySelected& OnDifficultySelected() { return DifficultySelected; }

	void CreateChildrenAndSetTexts();
	void UpdateDescription();

	void SetButtonVisibility(bool bVisible);

	// Pressed on a button
	UFUNCTION()
	void OnHandleButtonPress(int32 SelectedChild, USoUIUserWidget* SoUserWidget);

	UFUNCTION()
	void OnConfirmQuestionAnswered();

protected:

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FOnDifficultySelected DifficultySelected;


	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* DifficultySane;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* DifficultyIntended;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* DifficultyInsane;


	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* DifficultyIcon;


	UPROPERTY(EditAnywhere, Category = ">Widgets")
	UTexture2D* IconSane;

	UPROPERTY(EditAnywhere, Category = ">Widgets")
	UTexture2D* IconIntended;

	UPROPERTY(EditAnywhere, Category = ">Widgets")
	UTexture2D* IconInsane;


	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIConfirmQuestion* Confirmation;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	URichTextBlock* DifficultyDescription;

	// Runtime
	ESoDifficulty SelectedDifficulty = ESoDifficulty::Intended;
};
