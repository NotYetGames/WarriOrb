// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/SoUITypes.h"

#include "SoUIMenuMain.generated.h"

class USoUISettings;
class USoUISaveSlotSelection;
class USoInGameUIActivity;
class USoUIChallenges;
class USoUICommandTooltip;
class USoUIContainerNamedSlot;
class UFMODEvent;
class USoGameInstance;
class USoUIExternalLink;
class UImage;
class USpacer;
class UTextBlock;
class URetainerBox;
class USoUIConfirmPanel;
class USoUIConfirmQuestion;
class USoUIButtonImage;
class USoUICredits;

UENUM(BlueprintType)
enum class ESoUIMainMenuButton : uint8
{
	Continue = 0 		UMETA(DisplayName = "Continue"),
	RestartFromSK		UMETA(DisplayName = "Restart From SoulKeeper"),
	RestartFromCP		UMETA(DisplayName = "Restart From Checkpoint"),
	Load				UMETA(DisplayName = "Load"),
	Challenges			UMETA(DisplayName = "Challenges"),
	Settings			UMETA(DisplayName = "Settings"),

	Credits				UMETA(DisplayName = "Credits"),
	ExitGameAlways		UMETA(DisplayName = "ExitGameAlways"),
	Exit				UMETA(DisplayName = "Exit"),

	ChangeUser			UMETA(DisplayName = "ChangeUser"),

	NumOf				UMETA(meta = Hidden)
};

UENUM(BlueprintType)
enum class ESoUIMainMenuExternalLinkButton : uint8
{
	Discord = 0		UMETA(DisplayName = "Discord"),
	Twitter			UMETA(DisplayName = "Twitter"),
	SteamMainGame   UMETA(DisplayName = "Steam Main Game"),
	MailingList		UMETA(DisplayName = "MailingList"),

	NumOf			UMETA(meta = Hidden)
};


UENUM(BlueprintType)
enum class ESoUIMainMenuState : uint8
{
	// only the buttons are visible
	Root				UMETA(DisplayName = "Root"),

	Settings			UMETA(DisplayName = "Settings"),
	SaveSlots			UMETA(DisplayName = "SaveSlots"),
	Challenges			UMETA(DisplayName = "Challenges"),

	FadeIn				UMETA(DisplayName = "FadeIn"),
	FadeOut				UMETA(DisplayName = "FadeOut"),

	ConfirmQuit			UMETA(DisplayName = "ConfirmQuit"),
	ConfirmRestart		UMETA(DisplayName = "ConfirmRestart"),

	DisplayCredits		UMETA(DisplayName = "DisplayCredits"),

	NumOf				UMETA(meta = Hidden)
};


UCLASS()
class SORB_API USoUIMenuMain : public USoUIButtonArray, public ISoUIEventHandler
{
	GENERATED_BODY()

public:
	//
	// Begin UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//
	// ISoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override;
	void Open_Implementation(bool bOpen) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool CanBeInterrupted_Implementation() const override { return true; }

	// No animation
	void InstantOpen(bool bOpen);

protected:
	//
	// USoUIButtonArray
	//
	bool IsValidWidget() const override { return Super::IsValidWidget() && ActiveState == ESoUIMainMenuState::Root; }
	void AddContainerButton(USoUIButton* Button) override;

	//
	// Own methods
	//
	USoUIMenuMain(const FObjectInitializer& ObjectInitializer);

	void SetActiveState(ESoUIMainMenuState State);

	// Sets/Creates the texts and children for all lines
	// NOTE: does not depend on any runtime value
	void CreateChildrenAndSetTexts();

	// Updates text dynamically at runtime
	void UpdateButtonTexts();

	//
	// Visibility
	//
	bool AreExternalLinksVisible() const;
	bool IsFading() const { return ActiveState == ESoUIMainMenuState::FadeIn || ActiveState == ESoUIMainMenuState::FadeOut; }

	// Updates the children widget visibilities from the current state
	void UpdateWidgetsVisibilities();

	// Helper method that sets the visibility to the
	// - ExternalLinksContainer
	// - TextControllerRecommended
	// - ButtonContainer
	void SetForcedVisibilityOfMainMenuElements(ESlateVisibility NewVisibility);

	//
	// Confirm analytics popup
	//
	bool CanHandleConfirmAnalyticsUICommand() const;
	bool OpenConfirmCollectAnalytics();

	//
	// Confirm quit popup
	//
	bool CanHandleConfirmQuitUICommand() const;

	UFUNCTION()
	void OnConfirmQuestionAnswered();
	bool OpenConfirmQuit();
	void OpenConfirmRestart();
	void CleanupAfterConfirmQuestion();

	//
	// Before main menu panel screen
	//
	bool CanHandleBeforeMainMenuUICommand() const;

	UFUNCTION(BlueprintCallable)
	bool OpenPanelBeforeMainMenu(USoInGameUIActivity* InBeforeMainMenuScreen);

	// Pressed on a button
	UFUNCTION()
	void OnHandleButtonPress(int32 SelectedChild, USoUIUserWidget* SoUserWidget);

	void OnContinueButtonPressed();
	void OnLoadButtonPressed();

	// closes menu, enters game
	void EnterGame(bool bFadeOutMenu);
	void QuitGame();

	// Closes any current subpanel
	void CloseAllChildrenWidgets();
	void CloseCurrentChildWidget();

	// Handle the event after the save slot loaded.
	UFUNCTION()
	void HandleSaveSlotLoaded(int32 SaveSlotIndex);

	// Refresh the button container, it displays different buttons depending on the current state
	void RefreshButtonContainer();
	void UpdateButtonsActivations();

	// Updates the animation fade
	void UpdateFade(float FadeValue, float BackgroundAlpha);

	TArray<ESoUIMainMenuButton>& GetButtonOptions();
	void RefreshButtonOptions();
	void RefreshExternalLinksButtonOptions();
	void ResetSelectedButton(int32 NewIndex = 0)
	{
		UnhighlightAllChildren();
		SetSelectedIndex(0);
	}

	void SetTitleVisibility(bool bVisible);

	UFUNCTION()
	void OnCreditsFadeOutFinished();

protected:
	/** Menu Buttons: */
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* Continue;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* RestartFromSK;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* RestartFromCP;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* Load;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* Challenges;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* Settings;

	// displays the credits, only visible in Main Game / Main menu (game is not loaded yet)
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* Credits;

	// Special button that only appears ingame that says to quit the game
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* ExitGameAlways;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* Exit;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButton* ChangeUser;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* MenuButtonsBackground;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* MainBackgroundImage;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* MainBackgroundImageVFX;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* MainBackgroundColor;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* MainBackgroundTitle;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USpacer* MainBackgroundTitleSpacer;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICredits* CreditsScreen;

	UPROPERTY(BlueprintReadOnly, Category = ">State")
	bool bPressedExitGameAlways = false;

	UPROPERTY(EditAnywhere, Category = ">Background")
	float BackgroundImageExtraYSize = 24.0f;

	// Map selected button index => ESoUIMainMenuButton (button type)
	// NOTE the order in UMG does not matter, this is the only order that does.
	// NOTE: the values here are all the values, they will be modified at runtime
	// See: RefreshButtonOptions
	TArray<ESoUIMainMenuButton> ButtonOptions = {
		ESoUIMainMenuButton::Continue,
		ESoUIMainMenuButton::RestartFromSK,
		ESoUIMainMenuButton::RestartFromCP,
		ESoUIMainMenuButton::Challenges,
		ESoUIMainMenuButton::Load,
		ESoUIMainMenuButton::Settings,
		ESoUIMainMenuButton::Credits,
		ESoUIMainMenuButton::Exit,
		ESoUIMainMenuButton::ExitGameAlways,
		ESoUIMainMenuButton::ChangeUser,
	};

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	URetainerBox* RootRetainer;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* BackgroundFade;

	/** Submenus: */
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISettings* SettingsPanel;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISaveSlotSelection* SaveSlotSelectionPanel;

	// UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	// USoUIChallenges* ChallengesPanel;

	// Used at first before the main is entered
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets")
	USoInGameUIActivity* BeforeMainMenuScreen;

	// Only shown first time the game is started, prompting the user to collect analytics or not
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIConfirmPanel* ConfirmCollectAnalytics;

	// Confirm quit panel that show Yes/No options
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIConfirmQuestion* ConfirmQuit = nullptr;

	// Current UI active state, is this menu opened? loading screen?
	ESoUIMainMenuState ActiveState = ESoUIMainMenuState::Root;

	// Shows the back button
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonImage* ButtonImageToolTipBack;

	// Death Count, only visible if game is started
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* DeathCountText;

	// Death Count, only visible if game is started
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UImage* DeathCountIcon;

	// Text that shows we should use a controller
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIContainerNamedSlot* TextControllerRecommended;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UPanelWidget* ExternalLinksContainer;

	// Map selected button index => ESoUIMainMenuExternalLinkButton (button type)
	// NOTE also change order in UMG
	TArray<ESoUIMainMenuExternalLinkButton> ExternalLinksButtonOptions = {
		ESoUIMainMenuExternalLinkButton::Discord,
		ESoUIMainMenuExternalLinkButton::MailingList,
		ESoUIMainMenuExternalLinkButton::Twitter,
		ESoUIMainMenuExternalLinkButton::SteamMainGame
	};

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ExternalLinkDiscord;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ExternalLinkTwitter;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ExternalLinkSteamMainGame;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets",  meta = (BindWidget))
	USoUIExternalLink* ExternalLinkMailingList;

	UPROPERTY(EditAnywhere, Category = ">MenuAnimation")
	UCurveFloat* FadeOutCurve;

	// Is this menu opened?
	bool bOpened = true;

	// Keep the state of the is mouse allowed
	bool bPreviousIsMouseAllowed = false;

	float FadeOutCounter = 0.0f;


	UPROPERTY()
	USoGameInstance* GameInstance = nullptr;


	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXMenuOpenFromGame = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXMenuReturnToGame = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXTeleportToCheckpoint = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXTeleportToSoulKeeper = nullptr;
};
