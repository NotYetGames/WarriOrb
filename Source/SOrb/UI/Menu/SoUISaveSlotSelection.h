// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "UI/General/SoUIUserWidgetArray.h"
#include "SaveFiles/SoWorldStateTable.h"

#include "SoUISaveSlotSelection.generated.h"

class USoUIDifficultySelection;
class USoUICommandTooltipArray;
class USoUIConfirmQuestion;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveSlotLoaded, int32, SelectedSaveSlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBlackFadeRequest);

/**
 *
 */
UCLASS()
class SORB_API USoUISaveSlotSelection : public USoUIUserWidgetArray, public ISoUIEventHandler
{
	GENERATED_BODY()

public:

	//
	// Begin UUserWidget Interface
	//
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

	void RefreshSlots();
	void ProceedWithSlot(int32 SlotIndex);

	void LoadSaveSlot(const int32 SaveSlotIndex);
	void RemoveSelectedSaveSlot();

	// Save slot was loaded
	FOnSaveSlotLoaded& OnSaveSlotLoaded() { return SaveSlotLoaded; }
	FOnBlackFadeRequest& OnBlackFadeRequest() { return BlackFadeRequest; }
	

	UFUNCTION()
	void OnDifficultySelected(ESoDifficulty SelectedDifficulty);


	UFUNCTION()
	void IntroFinished();


	UFUNCTION()
	void OnSelectedChildChanged(int32 OldIndex, int32 NewIndex);

	UFUNCTION()
	void OnClearSaveSlotConfirmQuestionAnswered();

	UFUNCTION()
	void OnHandleButtonPress(int32 SelectedChild, USoUIUserWidget* SoUserWidget);


	void RefreshCommandTooltips();

	bool CanRemoveSelectedSlot() const;
	bool CanLoadSelectedSlot() const;

protected:

	// The event that fires after a save slot is loaded
	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FOnSaveSlotLoaded SaveSlotLoaded;

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FOnBlackFadeRequest BlackFadeRequest;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIDifficultySelection* DifficultySelection;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltipArray* CommandTooltips;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIConfirmQuestion* ClearSlotConfirmation;


	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXClearSlot = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXLoadSlot = nullptr;

	UPROPERTY(EditAnywhere, Category = SFX)
	UFMODEvent* SFXStartNewSlot = nullptr;

	// All the cached save slots
	TMap<int32, FSoStateTable> SaveSlotsData;

	ESoDifficulty CachedDifficulty;
	bool bIntroPlaying = false;

	float CutsceneStartCounter = -1.0f;
};
