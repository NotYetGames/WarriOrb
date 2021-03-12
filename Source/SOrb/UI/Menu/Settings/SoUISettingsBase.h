// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/General/SoUITypes.h"
#include "Components/PanelWidget.h"

#include "SoUISettingsBase.generated.h"

class USoUIButtonArray;
class USoUIScrollingButtonArray;
class USoUIButton;
class USoUISlider;
class USoUICheckbox;
class ASoCharacter;
class USoUIButtonImageArray;
class USoGameSettings;
class ASoPlayerController;
class USoUIUserWidgetArray;

DECLARE_LOG_CATEGORY_EXTERN(LogSoUISettings, All, All);

/**
 * Base class that tries to handle all the logic for using only keyboard/controller
 * Switching lines, activating lines, etc
 */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoUISettingsBase : public UUserWidget, public ISoUIEventHandler
{
	GENERATED_BODY()

protected:
	USoUISettingsBase(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// ISoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override { return HandleOnUICommand(Command); }
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool CanBeInterrupted_Implementation() const override { return true; }

	//
	// Own methods
	//

	virtual void OnPressedChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget);
	void OnPressedButtonTooltipsChild(int32 SelectedChild, USoUIUserWidget* SoUserWidget);

	// Sets/Creates the texts and children for all lines
	virtual void CreateChildrenAndSetTexts(bool bFromSynchronizeProperties = false);

	// Overwrite this in child classes for custom OnUICommand_Implementation
	virtual bool HandleOnUICommand(ESoUICommand Command);
	virtual void NavigateBackCommand();

	// Should any child widget handle the commands?
	virtual bool ShouldChildHandleCommands() const { return false; }

	// Updates the children widget visibilities from the current state
	virtual void UpdateWidgetsVisibilities() {}

	// Switch the the line depending on direction
	virtual bool NavigateSwitchLine(bool bUp);
	bool NavigateSwitchLineDown() { return NavigateSwitchLine(false); }
	bool NavigateSwitchLineUp() { return NavigateSwitchLine(true); }

	// Activate/Enter on the selected line
	bool NavigateOnPressed();

	// Navigate left or right on the selected line
	virtual bool NavigateLeftRightOnSelectedLine(ESoUICommand Command, bool bFromPressedInput = false);

	// Is the LineIndex a valid index?
	virtual bool IsValidLineIndex(int32 LineIndex) const;

	// Is the line at LineIndex enabled?
	bool IsLineEnabled(int32 LineIndex) const;

	bool SetIsLineEnabled(int32 LineIndex, bool bEnabled);

	void AddLine(UUserWidget* Widget);
	void AddLineWithText(UUserWidget* Widget, FText Text)
	{
		AddLine(Widget);
		AddTitleText(Text);
	}
	void AddTitleText(FText Text) { TitlesArrayTexts.Add(Text); }
	void EmptyLines();
	void EmptyTitles();
	void EmptyLinesAndTitles()
	{
		EmptyLines();
		EmptyTitles();
	}

	//
	// ButtonImagesTooltipsArray
	//

	// Refreshes ButtonImagesTooltipsArray
	void EmptyAndRefreshButtonImagesTooltips(bool bResetSelectedLine = true);

	UFUNCTION(BlueprintCallable, Category = UI)
	virtual void RefreshButtonImagesTooltips(bool bForceShowAll = false) { checkNoEntry(); }

	// Can we use the ButtonImagesTooltipsArray
	UFUNCTION(BlueprintPure, Category = UI)
	bool HasValidButtonImagesArray() const;

	UFUNCTION(BlueprintPure, Category = UI)
	bool IsLineIndexForButtonImagesArray(int32 LineIndex) const;

	UFUNCTION(BlueprintPure, Category = UI)
	bool IsOnButtonTooltips() const { return IsLineIndexForButtonImagesArray(GetSelectedLineIndex()); }

	// Select a new line
	UFUNCTION()
	virtual void OnPostSetSelectedIndex(int32 OldIndex, int32 NewIndex);
	void SetSelectedLineIndex(int32 NewIndex, bool bPlaySound = false);
	void SelectFirstValidLineIndex(bool bPlaySound = false);
	void SelectButtonTooltips(bool bPlaySound = false);

	int32 GetSelectedLineIndex() const;

	// Get the total number of lines
	int32 NumLines() const;

	// Gets the user widget at LineIndex (if any).
	UUserWidget* GetUserWidgetAtLineIndex(int32 LineIndex) const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", meta = (DisplayName = "OnLineChanged"))
	void ReceiveOnLineChanged(int32 OldLineIndex, int32 NewLineIndex);

	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual void OnLineChanged(int32 OldLineIndex, int32 NewLineIndex)
	{
		ReceiveOnLineChanged(OldLineIndex, NewLineIndex);
	}

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = ">Events", meta = (DisplayName = "OnPressedButtonTooltipsCommand"))
	bool ReceiveOnPressedButtonTooltipsCommand(ESoUICommand Command);

	UFUNCTION(BlueprintCallable, Category = ">Events")
	virtual bool OnPressedButtonTooltipsCommand(ESoUICommand Command)
	{
		return ReceiveOnPressedButtonTooltipsCommand(Command);
	}
	// Helper methods for GetUserWidgetAtLineIndex

	// Gets the button array at LineIndex if it is an array
	USoUIButtonArray* GetButtonArrayAtLineIndex(int32 LineIndex) const;

	// Gets the scrolling button array at LineIndex if it is a scrolling array
	USoUIScrollingButtonArray* GetScrollingButtonArrayAtLineIndex(int32 LineIndex) const;

	// Gets the button at LineIndex (if it is a button)
	USoUIButton* GetButtonAtLineIndex(int32 LineIndex) const;

	// Gets the checkbox at LineIndex (if it is a checkbox)
	USoUICheckbox* GetCheckboxAtLineIndex(int32 LineIndex) const;

	// Gets the slider at LineIndex (if it is a slider)
	USoUISlider* GetSliderAtLineIndex(int32 LineIndex) const;

	bool IsValidWidget() const { return IsVisible() && GetIsEnabled(); }

	USoUIButtonArray* GetWidgetsArrayAsButtons() const;

protected:
	//
	// TitlesArray | WidgetsArray
	//         ....|....
	//         ....|....
	//         ....|....
	//         ....|....
	//         ....|....
	//   ButtonImagesTooltipsArray
	//

	// If true the Widgets Array will be fake, we won't use its container and it will be hidden
	// Only the logic will be used
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">WidgetsArray")
	bool bFakeWidgetsArray = false;

	// Note this can be fake
	UPROPERTY(BlueprintReadOnly, Category = ">WidgetsArray", meta = (BindWidget))
	USoUIUserWidgetArray* WidgetsArray;

	// Contains the backgrounds images
	UPROPERTY(BlueprintReadOnly, Category = ">WidgetsArray", meta = (BindWidget))
	UPanelWidget* BackgroundsContainer;

	// Texts used for TitlesArray in PreConstruct
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Lines")
	TArray<FText> TitlesArrayTexts;

	// The bottom user interaction buttons, if any
	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonImageArray* ButtonImagesTooltipsArray;

	// Usually the left side titles for each settings panel
	UPROPERTY(BlueprintReadOnly, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* TitlesArray;

	// Is this UI opened?
	UPROPERTY(BlueprintReadOnly)
	bool bOpened = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Input")
	FSoUITrackedInputKeyDown TrackedInput;

	//
	// Cached
	//

	UPROPERTY()
	ASoCharacter* SoCharacter = nullptr;

	UPROPERTY()
	USoGameSettings* UserSettings = nullptr;

	UPROPERTY()
	ASoPlayerController* SoController = nullptr;

	// Maybe move this to array base class?
	bool bAllowChangingLines = true;
};
