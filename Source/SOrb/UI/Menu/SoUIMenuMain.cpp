// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIMenuMain.h"

#include "Components/Image.h"
#include "TimerManager.h"
#include "Components/RetainerBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Spacer.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Basic/SoGameMode.h"
#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoActivity.h"
#include "Character/SoPlayerProgress.h"

#include "Settings/SoGameSettings.h"
#include "SaveFiles/SoWorldState.h"
#include "SaveFiles/SoSaveHelper.h"

#include "Basic/SoGameInstance.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Basic/SoAudioManager.h"

#include "UI/Menu/Settings/SoUISettings.h"
#include "UI/Menu/SoUISaveSlotSelection.h"
#include "UI/Menu/SoUIChallenges.h"
#include "UI/General/Buttons/SoUIButton.h"
#include "UI/InGame/SoUIGameActivity.h"
#include "UI/General/Commands/SoUICommandTooltip.h"
#include "UI/General/SoUIContainerNamedSlot.h"
#include "UI/General/SoUIExternalLink.h"
#include "UI/General/SoUIConfirmPanel.h"
#include "UI/General/Buttons/SoUIButtonImage.h"
#include "UI/SoUIHelper.h"
#include "UI/SoUICredits.h"
#include "ConfirmPanels/SoUIConfirmQuestion.h"
#include "Localization/SoLocalization.h"

#include "Online/SoOnlineHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUIMenuMain, All, All);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIMenuMain::USoUIMenuMain(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bSkipDeactivatedChildren = true;
	bCyclicNavigation = true;
	bVerticalLayout = true;
	bExtraAttachedVerticalLayout = false;
	bTrackPressedInput = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	CreateChildrenAndSetTexts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::NativeConstruct()
{
	Super::NativeConstruct();
	GameInstance = USoGameInstance::GetInstance(this);
	check(GameInstance);
	RefreshButtonContainer();

	// temp fix
	bOpened = false;
	SetVisibility(ESlateVisibility::Collapsed);

	// Back button handle
	if (ButtonImageToolTipBack)
	{
		ButtonImageToolTipBack->OnPressedEvent().AddLambda([this]()
		{
			ISoUIEventHandler::Execute_OnUICommand(this, ESoUICommand::EUC_MainMenuBack);
		});
	}

	if (SaveSlotSelectionPanel)
		SaveSlotSelectionPanel->OnSaveSlotLoaded().AddDynamic(this, &ThisClass::HandleSaveSlotLoaded);

	if (ConfirmQuit)
		ConfirmQuit->OnQuestionAnsweredEvent().AddDynamic(this, &ThisClass::OnConfirmQuestionAnswered);

	if (CreditsScreen != nullptr)
		CreditsScreen->OnFadeOutFinished().AddDynamic(this, &ThisClass::OnCreditsFadeOutFinished);

	OnNavigateOnPressedHandleChildEvent().BindUObject(this, &ThisClass::OnHandleButtonPress);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::NativeDestruct()
{
	if (SaveSlotSelectionPanel)
		SaveSlotSelectionPanel->OnSaveSlotLoaded().RemoveDynamic(this, &USoUIMenuMain::HandleSaveSlotLoaded);

	if (ConfirmQuit)
		ConfirmQuit->OnQuestionAnsweredEvent().RemoveDynamic(this, &ThisClass::OnConfirmQuestionAnswered);

	if(CreditsScreen != nullptr)
		CreditsScreen->OnFadeOutFinished().RemoveDynamic(this, &ThisClass::OnCreditsFadeOutFinished);

	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CanHandleBeforeMainMenuUICommand())
	{
		// This is the safest and most sane place to call the benchmark as it has to be called after the SLATE Loading Screen is gone, otherwise crash happens
		if (GameInstance != nullptr && !GameInstance->IsLoadingScreenVisible())
			USoGameSettings::Get().RunAndApplyHardwareBenchmarkIfRequested();

		if (!BeforeMainMenuScreen->Update(InDeltaTime))
			ISoUIEventHandler::Execute_Open(this, true);
	}
	else if (ActiveState == ESoUIMainMenuState::FadeOut)
	{
		FadeOutCounter += InDeltaTime * 1.2;
		UpdateFade(FadeOutCurve->GetFloatValue(FMath::Clamp(FadeOutCounter, 0.0f, 1.0f)), FMath::Lerp(1.0f, 0.0f, FadeOutCounter));
		if (FadeOutCounter > 1.0f)
		{
			FadeOutCounter = 1.0f;
			SetActiveState(ESoUIMainMenuState::Root);
			UpdateFade(0.0f, 0.0f);
		}
	}
	else if (ActiveState == ESoUIMainMenuState::FadeIn)
	{
		FadeOutCounter -= InDeltaTime * 2;
		UpdateFade(FadeOutCurve->GetFloatValue(FMath::Clamp(FadeOutCounter, 0.0f, 1.0f)), FMath::Lerp(1.0f, 0.0f, FadeOutCounter));
		if (FadeOutCounter < 0.0f)
		{
			FadeOutCounter = 0.0f;
			SetActiveState(ESoUIMainMenuState::Root);
			UpdateFade(1.0f, 1.0f);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::OnUICommand_Implementation(ESoUICommand Command)
{
	Command = USoUIHelper::TryTranslateMenuCommandDirectionToGame(Command);

	//UE_LOG(LogSoUIMenuMain, Verbose, TEXT("USoUIMenuMain::Command = %s"), *FSoInputActionName::UICommandToActionName(Command).ToString());
	if (!GameInstance)
		return false;

	const bool bIsLoadingState = GameInstance->IsLoading();

	// Forward to proper subpanel
	if (!bIsLoadingState)
	{
		// Confirm Analytics
		if (CanHandleConfirmAnalyticsUICommand())
		{
			return ISoUIEventHandler::Execute_OnUICommand(ConfirmCollectAnalytics, Command);
		}

		// Before main menu warning
		if (CanHandleBeforeMainMenuUICommand())
		{
			return BeforeMainMenuScreen->HandleCommand(Command);
		}

		// Quit Game
		if (CanHandleConfirmQuitUICommand())
		{
			return ISoUIEventHandler::Execute_OnUICommand(ConfirmQuit, Command);
		}

		if (ActiveState == ESoUIMainMenuState::DisplayCredits)
		{
			CreditsScreen->HandleCommand(Command);
			return true;
		}
	}

	// Open menu, the first time
	// NOTE: even when the loading screen is opened
	if (!bOpened)
	{
		if (Command == ESoUICommand::EUC_MainMenuBack)
		{
			USoAudioManager::PlaySound2D(this, SFXMenuOpenFromGame);
			SetActiveState(ESoUIMainMenuState::FadeIn);
			FadeOutCounter = 1.0f;
			ISoUIEventHandler::Execute_Open(this, true);
		}
		return true;
	}

	// We are loading something, loading screen should be up
	if (bIsLoadingState)
		return false;

	bool bCommandHandled = false;
	const bool bGameStarted = GameInstance->IsGameStarted();
	bool bReturnToRoot = false;
	switch (ActiveState)
	{
		case ESoUIMainMenuState::Root:
		{
			switch (Command)
			{
				case ESoUICommand::EUC_Left:
				case ESoUICommand::EUC_Right:
				case ESoUICommand::EUC_Up:
				case ESoUICommand::EUC_Down:
				case ESoUICommand::EUC_MainMenuEnter:
					bCommandHandled = true;

					// Navigate on the buttons
					Navigate(Command);
					break;

				case ESoUICommand::EUC_MainMenuBack:
					bCommandHandled = true;
					if (bGameStarted)
						EnterGame(true);
					break;

				default:
					break;
			}
		}
		break;

		case ESoUIMainMenuState::SaveSlots:
		{
			bCommandHandled = true;

			// Forward UI command to save slots panel
			ISoUIEventHandler::Execute_OnUICommand(SaveSlotSelectionPanel, Command);

			// Check against ActiveState being changed by callbacks from loading saves
			if (ActiveState == ESoUIMainMenuState::SaveSlots)
				bReturnToRoot = !ISoUIEventHandler::Execute_IsOpened(SaveSlotSelectionPanel);
		}
		break;

		case ESoUIMainMenuState::Challenges:
		{
			bCommandHandled = true;

			// Forward UI command to the challenges panel
			//ISoUIEventHandler::Execute_OnUICommand(ChallengesPanel, Command);
			//
			//if (ActiveState == ESoUIMainMenuState::Challenges)
			//	bReturnToRoot = !ISoUIEventHandler::Execute_IsOpened(ChallengesPanel);
		}
		break;

		case ESoUIMainMenuState::Settings:
		{
			bCommandHandled = true;

			// Forward UI command to settings
			ISoUIEventHandler::Execute_OnUICommand(SettingsPanel, Command);

			// Was the settings closed? return back to main menu
			if (ActiveState == ESoUIMainMenuState::Settings)
				bReturnToRoot = !ISoUIEventHandler::Execute_IsOpened(SettingsPanel);
		}
		break;

		default:
			break;
	}

	if (bReturnToRoot)
	{
		SetActiveState(ESoUIMainMenuState::Root);
		USoAudioManager::PlaySound2D(this, USoGameSingleton::GetSFX(ESoSFX::MenuReturnToRoot));
	}

	return bCommandHandled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::Open_Implementation(bool bOpen)
{
	if (GameInstance == nullptr || bOpened == bOpen)
		return;

	if (ButtonImageToolTipBack)
		ButtonImageToolTipBack->SetVisibility(ESlateVisibility::Collapsed);

	// Enable/Disable mouse support
	if (bOpen)
	{
		UE_LOG(LogSoUIMenuMain, Verbose, TEXT("Opening Menu"));
		bPreviousIsMouseAllowed = USoPlatformHelper::IsMouseAllowed(this);
		USoPlatformHelper::AllowMouse(this);
	}
	else
	{
		UE_LOG(LogSoUIMenuMain, Verbose, TEXT("Closing Menu"));

		// Only disable if previous state was not to allow mouse
		if (!bPreviousIsMouseAllowed)
		{
			USoPlatformHelper::DisallowMouse(this);
		}
	}

	// Announce menu state change
	GameInstance->OnMenuStateChange(bOpen);

	// Initialize audio if not already.
	GameInstance->InitializeAudio();

	bPressedExitGameAlways = false;
	bOpened = bOpen;

	if (SoCharacter != nullptr)
		SoCharacter->SetGameInputBlockedByUI(bOpened);

	// Select first valid option
	// NOTE: here before opening any menu
	if (bOpen)
		RefreshButtonContainer();

	// do not hide yet if it's a "return to game" case so we can display an animation
	// aka let the fade out animation play out
	if (ActiveState != ESoUIMainMenuState::FadeOut)
		SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// Pause game
	USoPlatformHelper::SetGamePaused(this, bOpen && !GameInstance->IsMenu());

	if (bOpen)
	{
		if (ActiveState != ESoUIMainMenuState::FadeIn)
		{
			// instant fade in
			UpdateFade(1.0f, 1.0f);
			FadeOutCounter = 0.0f;
		}

		if (ISoUIEventHandler::Execute_IsOpened(SaveSlotSelectionPanel))
			ISoUIEventHandler::Execute_Open(SaveSlotSelectionPanel, false);

#if !WARRIORB_WITH_EDITOR
		//OpenConfirmCollectAnalytics();
		OpenPanelBeforeMainMenu(BeforeMainMenuScreen);
#endif // !WARRIORB_WITH_EDITOR
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::InstantOpen(bool bOpen)
{
	// Reset animations
	StopAllAnimations();
	if (ActiveState == ESoUIMainMenuState::FadeOut || ActiveState == ESoUIMainMenuState::FadeIn)
		SetActiveState(ESoUIMainMenuState::Root);

	ISoUIEventHandler::Execute_Open(this, bOpen);

	// Force visibility
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::CanHandleConfirmAnalyticsUICommand() const
{
	return ::IsValid(ConfirmCollectAnalytics) && ConfirmCollectAnalytics->CanHandleUICommand();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::OpenConfirmCollectAnalytics()
{
	if (!ConfirmCollectAnalytics)
		return false;

	if (USoGameSettings::Get().WasCollectAnalyticsSetByUser())
		return false;

	// Open the confirm panel
	ISoUIEventHandler::Execute_Open(ConfirmCollectAnalytics, true);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::CanHandleConfirmQuitUICommand() const
{
	return ::IsValid(ConfirmQuit) && ConfirmQuit->CanHandleUICommand();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::OnConfirmQuestionAnswered()
{
	if (ConfirmQuit->IsAnswerYes())
	{
		switch (ActiveState)
		{
			case ESoUIMainMenuState::ConfirmRestart:
				CleanupAfterConfirmQuestion();
				UE_LOG(LogSoUIMenuMain, Verbose, TEXT("Restarting demo"));
				GameInstance->LoadGameFromCurrentState(true);
				EnterGame(true);
				break;

			default:
				QuitGame();
		}
	}
	else if (!CanHandleConfirmQuitUICommand())
	{
		// Can no longer handle, cleanup
		CleanupAfterConfirmQuestion();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::OpenConfirmQuit()
{
	if (!ConfirmQuit)
		return false;

	// Set the correct texts
	const bool bGameStarted = GameInstance ? GameInstance->IsGameStarted() : false;

	if (bGameStarted)
	{
		if (bPressedExitGameAlways)
		{
			ConfirmQuit->SetTexts(
				FROM_STRING_TABLE_UI("quit_game_question"),
				FROM_STRING_TABLE_UI("progress_saved")
			);
		}
		else
		{
			ConfirmQuit->SetTexts(
				FROM_STRING_TABLE_UI("quit_to_menu_question"),
				FROM_STRING_TABLE_UI("progress_saved")
			);
		}
	}
	else
	{
		ConfirmQuit->SetTexts(
			FROM_STRING_TABLE_UI("quit_game_question"),
			FText::GetEmpty()
		);

		SetTitleVisibility(false);
	}

	// Open the confirm panel
	SetForcedVisibilityOfMainMenuElements(ESlateVisibility::Collapsed);
	ISoUIEventHandler::Execute_Open(ConfirmQuit, true);

	// Set directly to avoid widget visibilities update
	ActiveState = ESoUIMainMenuState::ConfirmQuit;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::OpenConfirmRestart()
{
	if (ConfirmQuit == nullptr)
		return;

	ConfirmQuit->SetTexts(
		FROM_STRING_TABLE_UI("restart_game_question"),
		FROM_STRING_TABLE_UI("restart_game_warning")
	);

	const bool bGameStarted = GameInstance ? GameInstance->IsGameStarted() : false;
	if (!bGameStarted)
	{
		SetTitleVisibility(false);
	}

	// Open the confirm panel
	SetForcedVisibilityOfMainMenuElements(ESlateVisibility::Collapsed);
	ISoUIEventHandler::Execute_Open(ConfirmQuit, true);

	// Set directly to avoid widget visibilities update
	ActiveState = ESoUIMainMenuState::ConfirmRestart;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::CleanupAfterConfirmQuestion()
{
	ISoUIEventHandler::Execute_Open(ConfirmQuit, false);

	// Reset to the current state
	bPressedExitGameAlways = false;
	SetActiveState(ESoUIMainMenuState::Root);

	if (MainBackgroundTitle)
	{
		const bool bGameStarted = GameInstance ? GameInstance->IsGameStarted() : false;
		SetTitleVisibility(!bGameStarted);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::CanHandleBeforeMainMenuUICommand() const
{
	return ::IsValid(BeforeMainMenuScreen) && BeforeMainMenuScreen->IsOpened();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::OpenPanelBeforeMainMenu(USoInGameUIActivity* InBeforeMainMenuScreen)
{
	BeforeMainMenuScreen = InBeforeMainMenuScreen;

	if (!BeforeMainMenuScreen)
		return false;

	if (GameInstance->IsBeforeMenuShownOnce())
		return false;

	// Open before main menu
	GameInstance->SetBeforeMenuShownOnce(true);
	BeforeMainMenuScreen->SetInGameActivityEnabled(nullptr, true);

#if PLATFORM_XBOXONE
	USoOnlineHelper::SpawnLocalPlayers(this);
#endif

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::CloseAllChildrenWidgets()
{
	if (SaveSlotSelectionPanel)
		ISoUIEventHandler::Execute_Open(SaveSlotSelectionPanel, false);

	//if (ChallengesPanel)
	//	ChallengesPanel->InstantOpen(false);

	if (SettingsPanel)
		SettingsPanel->InstantOpen(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::AddContainerButton(USoUIButton* Button)
{
	Button->SetVisibility(ESlateVisibility::Visible);
	Super::AddContainerButton(Button);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::SetActiveState(ESoUIMainMenuState State)
{
	ActiveState = State;

	switch (ActiveState)
	{
	case ESoUIMainMenuState::Root:
		CloseAllChildrenWidgets();
		SetVisibility(bOpened ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		break;

	case ESoUIMainMenuState::SaveSlots:
		ISoUIEventHandler::Execute_Open(SaveSlotSelectionPanel, true);
		break;

	case ESoUIMainMenuState::Challenges:
		// ISoUIEventHandler::Execute_Open(ChallengesPanel, true);
		break;

	case ESoUIMainMenuState::Settings:
		ISoUIEventHandler::Execute_Open(SettingsPanel, true);
		break;

	case ESoUIMainMenuState::DisplayCredits:
		CreditsScreen->FadeIn(true);
		// no need to hide anything on credits request
		return;

	default:
		break;
	}

	// Update visibilities
	UpdateWidgetsVisibilities();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::CreateChildrenAndSetTexts()
{
	RefreshExternalLinksButtonOptions();
	EmptyContainerChildren();
	TArray<FText> ChildrenTexts;

	for (const ESoUIMainMenuButton Option : ButtonOptions)
	{
		switch (Option)
		{
			case ESoUIMainMenuButton::Continue:
				AddContainerButton(Continue);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("continue"));
				break;
			case ESoUIMainMenuButton::RestartFromSK:
				AddContainerButton(RestartFromSK);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("teleport_to_sk"));
				break;
			case ESoUIMainMenuButton::RestartFromCP:
				AddContainerButton(RestartFromCP);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("teleport_to_cp"));
				break;
			case ESoUIMainMenuButton::Load:
				AddContainerButton(Load);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("saves"));
				break;
			case ESoUIMainMenuButton::Challenges:
				AddContainerButton(Challenges);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("custom_maps"));
				break;
			case ESoUIMainMenuButton::Settings:
				AddContainerButton(Settings);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("settings"));
				break;
			case ESoUIMainMenuButton::Credits:
				AddContainerButton(Credits);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("credits"));
				break;
			case ESoUIMainMenuButton::ExitGameAlways:
				AddContainerButton(ExitGameAlways);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("quit_game"));
				break;
			case ESoUIMainMenuButton::Exit:
				AddContainerButton(Exit);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("quit_game"));
				break;
			case ESoUIMainMenuButton::ChangeUser:
				AddContainerButton(ChangeUser);
				ChildrenTexts.Add(FROM_STRING_TABLE_UI("change_user"));
				break;
			default:
				checkNoEntry();
				break;
		}
	}
	check(NumContainerChildren() == ChildrenTexts.Num());
	check(NumContainerChildren() == ButtonOptions.Num());
	SetButtonsTexts(ChildrenTexts);

	Continue->Activate();
	if (Load != nullptr)
		Load->Activate();
	Challenges->Activate();
	Settings->Activate();
	Credits->Activate();
	Exit->Activate();

	if (ChangeUser != nullptr)
		ChangeUser->Activate();

	// Set External links
	EmptyExtraAttachedChildren();
	for (const ESoUIMainMenuExternalLinkButton Option : ExternalLinksButtonOptions)
	{
		switch (Option)
		{
		case ESoUIMainMenuExternalLinkButton::Discord:
			if (ExternalLinkDiscord)
				AddExtraAttachedChild(ExternalLinkDiscord);
			break;
		case ESoUIMainMenuExternalLinkButton::Twitter:
			if (ExternalLinkTwitter)
				AddExtraAttachedChild(ExternalLinkTwitter);
			break;
		case ESoUIMainMenuExternalLinkButton::SteamMainGame:
			if (ExternalLinkSteamMainGame)
				AddExtraAttachedChild(ExternalLinkSteamMainGame);
			break;

		case ESoUIMainMenuExternalLinkButton::MailingList:
			if (ExternalLinkMailingList)
				AddExtraAttachedChild(ExternalLinkMailingList);
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	// check(ExternalLinksButtonOptions.Num() == NumExtraAttachedChildren());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::UpdateButtonTexts()
{
	if (!GameInstance)
		return;

	const bool bGameStarted = GameInstance->IsGameStarted();
	if (Continue)
	{
		if (bGameStarted)
			Continue->SetButtonText(FROM_STRING_TABLE_UI("continue"));
		else
			Continue->SetButtonText(FROM_STRING_TABLE_UI("start_game"));
	}

	if (Exit)
	{
		if (bGameStarted)
			Exit->SetButtonText(FROM_STRING_TABLE_UI("quit_to_menu"));
		else
			Exit->SetButtonText(FROM_STRING_TABLE_UI("quit_game"));
	}

	// override button texts for demo
	if (USoPlatformHelper::IsDemo() && GameInstance->CanContinueGame())
	{
		Continue->SetButtonText(FROM_STRING_TABLE_UI("continue"));
		Load->SetButtonText(FROM_STRING_TABLE_UI("restart"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIMenuMain::AreExternalLinksVisible() const
{
	return ActiveState == ESoUIMainMenuState::Root || ActiveState == ESoUIMainMenuState::FadeIn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::UpdateWidgetsVisibilities()
{
	const bool bGameStarted = GameInstance ? GameInstance->IsGameStarted() : false;
	if (MainBackgroundColor)
		MainBackgroundColor->SetVisibility(bGameStarted ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (ButtonImageToolTipBack)
		ButtonImageToolTipBack->SetVisibility(ActiveState == ESoUIMainMenuState::Root || IsFading() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (MainBackgroundImage)
		MainBackgroundImage->SetVisibility(bGameStarted ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (MainBackgroundImageVFX)
		MainBackgroundImageVFX->SetVisibility(bGameStarted ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (DeathCountText)
		DeathCountText->SetVisibility(bGameStarted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (DeathCountIcon)
		DeathCountIcon->SetVisibility(bGameStarted ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bGameStarted && SoCharacter != nullptr && SoCharacter->GetPlayerProgress() != nullptr)
	{
		FNumberFormattingOptions Format;
		Format.SetUseGrouping(false);
		DeathCountText->SetText(FText::AsNumber(SoCharacter->GetPlayerProgress()->GetStatsFromAllSession().TotalDeathNum, &Format));
	}

	SetTitleVisibility(!bGameStarted && ActiveState != ESoUIMainMenuState::SaveSlots);

	if (TextControllerRecommended)
		TextControllerRecommended->SetShouldBeVisible(!bGameStarted);

	if (ExternalLinksContainer)
		ExternalLinksContainer->SetVisibility(AreExternalLinksVisible() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (ActiveState != ESoUIMainMenuState::SaveSlots)
		ShowContainer();
	else
		HideContainer();

	if (MenuButtonsBackground)
		MenuButtonsBackground->SetVisibility(ESlateVisibility::Visible);

	if (ExternalLinkSteamMainGame)
	{
		ExternalLinkSteamMainGame->SetVisibility(
			ExternalLinksButtonOptions.Contains(ESoUIMainMenuExternalLinkButton::SteamMainGame) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}

	if (ExternalLinkTwitter)
	{
		ExternalLinkTwitter->SetVisibility(
			ExternalLinksButtonOptions.Contains(ESoUIMainMenuExternalLinkButton::Twitter) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}

	if (ExternalLinkDiscord)
	{
		ExternalLinkDiscord->SetVisibility(
			ExternalLinksButtonOptions.Contains(ESoUIMainMenuExternalLinkButton::Discord) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}

	if (ExternalLinkMailingList)
	{
		ExternalLinkMailingList->SetVisibility(
			ExternalLinksButtonOptions.Contains(ESoUIMainMenuExternalLinkButton::MailingList) ?
			ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::SetForcedVisibilityOfMainMenuElements(ESlateVisibility NewVisibility)
{
	if (ExternalLinksContainer)
		ExternalLinksContainer->SetVisibility(NewVisibility);
	if (TextControllerRecommended)
		TextControllerRecommended->SetVisibility(NewVisibility);
	if (Container)
		Container->SetVisibility(NewVisibility);
	if (MenuButtonsBackground)
		MenuButtonsBackground->SetVisibility(NewVisibility);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::OnHandleButtonPress(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	if (SoCharacter == nullptr || SoUserWidget == nullptr || !IsValidIndex(SelectedChild))
		return;

	// SFX
	SoUserWidget->NavigateOnPressed(true);

	// External link handled
	if (IsIndexForExtraAttachedChildren(SelectedChild))
	{
		return;
	}

	// ignore button press
	if (ActiveState == ESoUIMainMenuState::DisplayCredits)
		return;

	const ESoUIMainMenuButton SelectedButton = GetButtonOptions()[SelectedChild];
	switch (SelectedButton)
	{
		case ESoUIMainMenuButton::Continue:
			OnContinueButtonPressed();
			break;

		case ESoUIMainMenuButton::RestartFromSK:
			EnterGame(true);
			SoCharacter->Revive(true);
			USoAudioManager::PlaySound2D(this, SFXTeleportToSoulKeeper);
			break;

		case ESoUIMainMenuButton::RestartFromCP:
			EnterGame(true);
			SoCharacter->Revive(false);
			USoAudioManager::PlaySound2D(this, SFXTeleportToCheckpoint);
			break;

		case ESoUIMainMenuButton::Load:
			OnLoadButtonPressed();
			break;

		case ESoUIMainMenuButton::Challenges:
			// SetActiveState(ESoUIMainMenuState::Challenges);
			break;

		case ESoUIMainMenuButton::Settings:
			SetActiveState(ESoUIMainMenuState::Settings);
			break;

		case ESoUIMainMenuButton::Credits:
			SetActiveState(ESoUIMainMenuState::DisplayCredits);
			break;

		case ESoUIMainMenuButton::ExitGameAlways:
			bPressedExitGameAlways = true;
			OpenConfirmQuit();
			break;

		case ESoUIMainMenuButton::Exit:
			OpenConfirmQuit();
			break;

		case ESoUIMainMenuButton::ChangeUser:
			USoOnlineHelper::ShowExternalLoginUI(GameInstance);
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::OnContinueButtonPressed()
{
	verify(GameInstance);

	// Load the proper map
	if (GameInstance->IsGameStarted())
	{
		UE_LOG(LogSoUIMenuMain, Verbose, TEXT("Not loading game because game is already started"));
		EnterGame(true);
	}
	else
	{
		if (USoPlatformHelper::IsDemo())
		{
			GameInstance->LoadGameFromCurrentState(false);
			EnterGame(false);
		}
		else
		{
			// Load game
			SetActiveState(ESoUIMainMenuState::SaveSlots);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::OnLoadButtonPressed()
{
	if (USoPlatformHelper::IsDemo())
	{
		OpenConfirmRestart();
	}
	else
	{
		// Load Game
		SetActiveState(ESoUIMainMenuState::SaveSlots);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::EnterGame(bool bFadeOutMenu)
{
	if (!GameInstance || !GameInstance->IsGameStarted())
		return;

	// Game entered
	if (bFadeOutMenu)
	{
		USoAudioManager::PlaySound2D(this, SFXMenuReturnToGame);
		SetActiveState(ESoUIMainMenuState::FadeOut);
		FadeOutCounter = 0.0f;
	}

	// Unpause/close this menu
	ISoUIEventHandler::Execute_Open(this, false);

	USoPlatformHelper::SetGamePaused(this, false);
	GameInstance->SetGameStarted(true);

	// if already in ui (maybe dialogue) music update is not needed
	if (SoCharacter->GetActivity() != EActivity::EA_InUI)
		SoCharacter->UpdateMusic(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::QuitGame()
{
	const bool bGameStarted = GameInstance ? GameInstance->IsGameStarted() : false;

	if (bGameStarted)
	{
		if (bPressedExitGameAlways)
		{
			// Quit
			// NOTE: save is done in PreExit in the Engine
			USoPlatformHelper::QuitGame(GetWorld());
		}
		else
		{
			// Return to main menu
			// NOTE: save is performed in teleport
			if (GameInstance)
				GameInstance->TeleportToMainMenu(true);
		}
	}
	else
	{
		// Quit
		USoPlatformHelper::QuitGame(GetWorld());
	}

	bPressedExitGameAlways = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::HandleSaveSlotLoaded(int32 SaveSlotIndex)
{
	EnterGame(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<ESoUIMainMenuButton>& USoUIMenuMain::GetButtonOptions()
{
	return ButtonOptions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::RefreshButtonOptions()
{
	ButtonOptions = { ESoUIMainMenuButton::Continue };

	// Add teleport options
	if (GameInstance->IsGameStarted())
	{
		if (GameInstance->CanHaveSoulKeeper())
			ButtonOptions.Add(ESoUIMainMenuButton::RestartFromSK);

		ButtonOptions.Add(ESoUIMainMenuButton::RestartFromCP);
	}

	// Not available in the demo

	if (USoPlatformHelper::IsDemo())
	{
		// Use as restart game
		if (GameInstance->CanContinueGame())
			ButtonOptions.Add(ESoUIMainMenuButton::Load);
	}
	else
	{
		// TODO fix this
		// if (USoGameSettings::Get().AreCustomMapsEnabled())
			// ButtonOptions.Add(ESoUIMainMenuButton::Challenges);

		if (GameInstance->IsGameStarted())
			ButtonOptions.Add(ESoUIMainMenuButton::Load);
	}

	ButtonOptions.Add(ESoUIMainMenuButton::Settings);

	if (!USoPlatformHelper::IsDemo())
		if (!GameInstance->IsGameStarted())
			ButtonOptions.Add(ESoUIMainMenuButton::Credits);

	if (USoPlatformHelper::IsConsole())
	{
		if (GameInstance->IsGameStarted())
			ButtonOptions.Add(ESoUIMainMenuButton::Exit); // this one brings you back to the menu
		else
		{
			if (!USoPlatformHelper::IsSwitch())
				ButtonOptions.Add(ESoUIMainMenuButton::ChangeUser);
		}
	}
	else
	{
		ButtonOptions.Add(ESoUIMainMenuButton::Exit);

		// Quit game only if game started
		if (GameInstance->IsGameStarted())
			ButtonOptions.Add(ESoUIMainMenuButton::ExitGameAlways);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::RefreshExternalLinksButtonOptions()
{
	// External link
	ExternalLinksButtonOptions = {
#if !PLATFORM_SWITCH
		ESoUIMainMenuExternalLinkButton::Discord,
		ESoUIMainMenuExternalLinkButton::MailingList,
		ESoUIMainMenuExternalLinkButton::Twitter
#endif
	};

	if (USoPlatformHelper::IsSteamBuildPirated() || (USoPlatformHelper::IsDemo() && !USoPlatformHelper::IsConsole()))
		ExternalLinksButtonOptions.Add(ESoUIMainMenuExternalLinkButton::SteamMainGame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::RefreshButtonContainer()
{
	if (Container == nullptr)
		return;

	RefreshButtonOptions();
	EmptyContainerChildren(true);

	for (const ESoUIMainMenuButton Option : GetButtonOptions())
	{
		switch (Option)
		{
		case ESoUIMainMenuButton::Continue:
			AddContainerButton(Continue);
			break;
		case ESoUIMainMenuButton::RestartFromSK:
			AddContainerButton(RestartFromSK);
			break;
		case ESoUIMainMenuButton::RestartFromCP:
			AddContainerButton(RestartFromCP);
			break;
		case ESoUIMainMenuButton::Load:
			AddContainerButton(Load);
			break;
		case ESoUIMainMenuButton::Challenges:
			AddContainerButton(Challenges);
			break;
		case ESoUIMainMenuButton::Settings:
			AddContainerButton(Settings);
			break;
		case ESoUIMainMenuButton::Credits:
			AddContainerButton(Credits);
			break;
		case ESoUIMainMenuButton::ExitGameAlways:
			AddContainerButton(ExitGameAlways);
			break;
		case ESoUIMainMenuButton::Exit:
			AddContainerButton(Exit);
			break;

		case ESoUIMainMenuButton::ChangeUser:
			AddContainerButton(ChangeUser);
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	UpdateButtonsActivations();
	SelectFirstValidChild();
	UpdateWidgetsVisibilities();
	UpdateButtonTexts();

	checkf(ButtonOptions.Num() == NumContainerChildren(), TEXT("Learn how to properly code"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::UpdateButtonsActivations()
{
	// Continue is disabled if the game is completed
	bool bIsContinueActive = true;
	const bool bGameStarted = GameInstance->IsGameStarted();

	if (USoPlatformHelper::IsDemo())
	{
		if (GameInstance->IsMenu() || !bGameStarted)
		{
			bIsContinueActive = !FSoWorldState::Get().IsGameCompleted();
		}
	}
	if (Continue)
		Continue->SetIsActive(bIsContinueActive);

	const bool bTeleportBlocked = SoCharacter != nullptr && SoCharacter->SoActivity != nullptr && SoCharacter->SoActivity->BlocksTeleportRequest();
	const bool bTeleportAllowed = !bTeleportBlocked && bGameStarted && SoCharacter != nullptr;

	// Activate/Deactivate restart buttons
	if (RestartFromSK)
		RestartFromSK->SetIsActive(bTeleportAllowed && SoCharacter->IsSoulkeeperActive() && SoCharacter->CanUseSoulkeeperAtLocation());

	if (RestartFromCP)
		RestartFromCP->SetIsActive(bTeleportAllowed && SoCharacter->GetActiveCheckpointLocationName() != NAME_None);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::UpdateFade(float FadeValue, float BackgroundAlpha)
{
	static const FName SoFadeStrParamName = FName("FadeStr");
	if (RootRetainer->GetEffectMaterial() != nullptr)
		RootRetainer->GetEffectMaterial()->SetScalarParameterValue(SoFadeStrParamName, FadeValue);

	BackgroundFade->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, BackgroundAlpha));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::SetTitleVisibility(bool bVisible)
{
	if (MainBackgroundTitle != nullptr)
		MainBackgroundTitle->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (MainBackgroundTitleSpacer != nullptr)
		MainBackgroundTitleSpacer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIMenuMain::OnCreditsFadeOutFinished()
{
	SetActiveState(ESoUIMainMenuState::Root);
}
