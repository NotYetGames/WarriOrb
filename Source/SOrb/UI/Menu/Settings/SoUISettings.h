// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/General/SoUITypes.h"

#include "SoUISettings.generated.h"

class UImage;
class USoUIButtonArray;
class USoUISettingsDisplay;
class USoUISettingsGame;
class USoUISettingsAudio;
class USoUISettingsKeyboard;
class USoUISettingsController;
class USoUICommandImage;
class USoUISettingsBrightness;
class UFMODEvent;
class ASoCharacter;
class USoUIButtonImage;

// All the panel options for the settings
enum class ESoUISettingsSubPanelOption : uint8
{
	Display = 0,
	Brightness,

	Audio,
	Game,

	Keyboard,
	Controller
};

/**
 * Show all the available settings of the game, can switch between different settings panels
 */
UCLASS()
class SORB_API USoUISettings : public UUserWidget, public ISoUIEventHandler
{
	GENERATED_BODY()

public:
	USoUISettings(const FObjectInitializer& ObjectInitializer);

	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);

	//
	// ISoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override;
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool CanBeInterrupted_Implementation() const override { return true; }

	//
	// Own methods
	//

	// No animation
	void InstantOpen(bool bOpen);

protected:
	//
	// Own methods
	//

	bool NavigateTopMenu(ESoUICommand Command);
	bool CloseOldPanelAndOpenNew(int32 OldPanelIndex, int32 NewPanelIndex);

	// Sets/Creates the texts and children for all sub panels
	// NOTE: does not depend on any runtime value
	void CreateChildrenAndSetTexts();

	// Open panel at PanelIndex
	void OpenPanelAtIndex(int32 PanelIndex, bool bSelectSubmenuButton = false);

	// Close panel at PanelIndex
	void ClosePanelAtIndex(int32 PanelIndex);
	void CloseAllPanels();

	// Can we interrupt the panel
	bool CanPanelAtIndexBeInterrupted(int32 PanelIndex) const;

	UUserWidget* GetPanelAtIndex(int32 PanelIndex) const
	{
		return SubPanels.IsValidIndex(PanelIndex) ? SubPanels[PanelIndex] : nullptr;
	}

	// Get the UI commands for top menu navigation
	UFUNCTION(BlueprintPure, Category = UI)
	ESoUICommand GetTopLeftUICommand() const { return UICommand_TopLeft; }

	UFUNCTION(BlueprintPure, Category = UI)
	ESoUICommand GetTopRightUICommand() const { return UICommand_TopRight; }

	void RefreshSubPanelOptions();
	void UpdateWidgetsVisibilities();

	bool IsValid() const { return IsVisible() && GetIsEnabled(); }

protected:
	// Top buttons array that is kept in the same order as SubPanels
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* SubMenus = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettingsDisplay* Display = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettingsBrightness* Brightness = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettingsGame* Game = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettingsAudio* Audio = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettingsKeyboard* Keyboard = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettingsController* Controller = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonImage* TopShortcutLeft = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonImage* TopShortcutRight = nullptr;

	// Keep all the panels into an array
	UPROPERTY()
	TArray<UUserWidget*> SubPanels;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">Input")
	FSoUITrackedInputKeyDown TrackedInput;

	// All the subpanel options
	// Map selected panel index => ESoUISettingsSubPanelOption (sub panel type)
	TArray<ESoUISettingsSubPanelOption> SubPanelOptions = {
		ESoUISettingsSubPanelOption::Display,
		ESoUISettingsSubPanelOption::Brightness,
		ESoUISettingsSubPanelOption::Audio,
		ESoUISettingsSubPanelOption::Game,
		ESoUISettingsSubPanelOption::Keyboard,
		ESoUISettingsSubPanelOption::Controller
	};

	// Are the settings opened?
	bool bOpened = false;

	// UI shortcuts for navigating the top menus
	static constexpr ESoUICommand UICommand_TopLeft = ESoUICommand::EUC_TopLeft;
	static constexpr ESoUICommand UICommand_TopRight = ESoUICommand::EUC_TopRight;

	//
	// Cached
	//

	UPROPERTY()
	ASoCharacter* SoCharacter = nullptr;
};
