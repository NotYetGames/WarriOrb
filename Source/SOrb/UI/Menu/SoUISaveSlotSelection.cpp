// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.


#include "SoUISaveSlotSelection.h"

#include "Components/PanelWidget.h"

#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "FileMediaSource.h"
#include "TimerManager.h"

#include "FMODEvent.h"

#include "UI/General/Commands/SoUICommandTooltipArray.h"
#include "UI/General/SoUIVideoPlayer.h"
#include "UI/General/SoUIPressAndHoldConfirmation.h"
#include "UI/Menu/ConfirmPanels/SoUIConfirmQuestion.h"
#include "SoUISaveSlot.h"
#include "SoUIDifficultySelection.h"
#include "SaveFiles/SoWorldState.h"
#include "Basic/SoGameInstance.h"
#include "Basic/SoAudioManager.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Online/Analytics/SoAnalytics.h"
#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoADefault.h"
#include "SoBeforeGame/Public/SoLocalization.h"
#include "Basic/Helpers/SoPlatformHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUISaveSlotSelection, All, All);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::NativeConstruct()
{
	Super::NativeConstruct();

	if (DifficultySelection != nullptr)
		DifficultySelection->OnDifficultySelected().AddDynamic(this, &USoUISaveSlotSelection::OnDifficultySelected);

	OnSelectedChildChangedEvent().AddUObject(this, &ThisClass::OnSelectedChildChanged);
	OnNavigateOnPressedHandleChildEvent().BindUObject(this, &ThisClass::OnHandleButtonPress);

	// fill child array for navigation
	for (int32 SlotIndex = 0; SlotIndex < Container->GetChildrenCount(); ++SlotIndex)
		if (USoUISaveSlot* SaveSlot = Cast<USoUISaveSlot>(Container->GetChildAt(SlotIndex)))
			AddContainerChild(SaveSlot);

	if (ClearSlotConfirmation != nullptr)
	{
		ClearSlotConfirmation->SetTexts(
			FROM_STRING_TABLE_UI("save_slots_remove_confirm"),
			FROM_STRING_TABLE_UI("save_slots_remove_confirm_subtext")
		);
		ClearSlotConfirmation->OnQuestionAnsweredEvent().AddDynamic(this, &ThisClass::OnClearSaveSlotConfirmQuestionAnswered);
	}

	bCyclicNavigation = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::NativeDestruct()
{
	Super::NativeDestruct();

	if (DifficultySelection != nullptr)
		DifficultySelection->OnDifficultySelected().RemoveDynamic(this, &ThisClass::OnDifficultySelected);

	if (ClearSlotConfirmation != nullptr)
		ClearSlotConfirmation->OnQuestionAnsweredEvent().RemoveDynamic(this, &ThisClass::OnClearSaveSlotConfirmQuestionAnswered);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CutsceneStartCounter >= 0.0f)
	{
		CutsceneStartCounter -= InDeltaTime;
		if (CutsceneStartCounter < 0.0f)
		{
			ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
			USoUIVideoPlayer* VideoPlayer = Character->GetUIVideoPlayer();
			VideoPlayer->SetIsVideoLooping(false);
			USoGameSingleton& SoSingleton = USoGameSingleton::Get();
			VideoPlayer->StartVideo(SoSingleton.IntroMediaSourcePtr, SoSingleton.VideoMediaPlayerPtr, SoSingleton.VideoMediaTexturePtr, SoSingleton.IntroSounds.Get());
			VideoPlayer->OnVideoFinishedOrSkipped.AddDynamic(this, &USoUISaveSlotSelection::IntroFinished);
			VideoPlayer->SetSubtitles(&SoSingleton.IntroSubs);
			USoPlatformHelper::DisallowMouse(this);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISaveSlotSelection::OnUICommand_Implementation(ESoUICommand Command)
{
	if (!ISoUIEventHandler::Execute_IsOpened(this))
		return false;

	if (bIntroPlaying)
	{
		if (Command == ESoUICommand::EUC_ActionBack || Command == ESoUICommand::EUC_MainMenuBack)
		{
			ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
			USoUIVideoPlayer* VideoPlayer = Character->GetUIVideoPlayer();

			VideoPlayer->PauseVideo(!VideoPlayer->IsVideoPaused());
		}

		return true;
	}

	// Confirmation Open
	if (IsValid(ClearSlotConfirmation) && ISoUIEventHandler::Execute_IsOpened(ClearSlotConfirmation))
	{
		return ISoUIEventHandler::Execute_OnUICommand(ClearSlotConfirmation, Command);
	}

	// difficulty selection mode
	if (ISoUIEventHandler::Execute_IsOpened(DifficultySelection))
	{
		if (Command == ESoUICommand::EUC_MainMenuBack)
		{
			ISoUIEventHandler::Execute_Open(DifficultySelection, false);
			Container->SetVisibility(ESlateVisibility::Visible);
			RefreshCommandTooltips();
			return true;
		}
		else
			return ISoUIEventHandler::Execute_OnUICommand(DifficultySelection, Command);
	}

	switch (Command)
	{
		case ESoUICommand::EUC_Left:
		case ESoUICommand::EUC_Right:
		case ESoUICommand::EUC_Up:
		case ESoUICommand::EUC_Down:
		case ESoUICommand::EUC_MainMenuEnter:
			// Navigate on the buttons
			Navigate(Command);
			return true;

		case ESoUICommand::EUC_Action0:
			ProceedWithSlot(GetSelectedIndex());
			return true;

		case ESoUICommand::EUC_Action1:
			if (CanRemoveSelectedSlot())
			{
				Container->SetVisibility(ESlateVisibility::Collapsed);
				ISoUIEventHandler::Execute_Open(ClearSlotConfirmation, true);
				RefreshCommandTooltips();
				USoAudioManager::PlaySound2D(this, SFXClearSlot, true);
			}
			return true;

		case ESoUICommand::EUC_MainMenuBack:
			ISoUIEventHandler::Execute_Open(this, false);
			return true;

		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::Open_Implementation(bool bOpen)
{
	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (bOpen)
	{
		Container->SetVisibility(ESlateVisibility::Visible);
		if (ISoUIEventHandler::Execute_IsOpened(DifficultySelection))
			ISoUIEventHandler::Execute_Open(DifficultySelection, false);

		RefreshSlots();
		SetSelectedIndex(0);
		RefreshCommandTooltips();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::RefreshSlots()
{
	FSoWorldState::Get().GetAllSavesSlots(SaveSlotsData);

	const bool bGameStarted = USoGameInstance::GetInstance(this) ? USoGameInstance::GetInstance(this)->IsGameStarted() : false;

	for (int32 SlotIndex = 0; SlotIndex < Container->GetChildrenCount(); ++SlotIndex)
	{
		if (USoUISaveSlot* SaveSlot = Cast<USoUISaveSlot>(Container->GetChildAt(SlotIndex)))
		{
			if (SaveSlotsData.Contains(SlotIndex))
				SaveSlot->InitializeFromState(SaveSlotsData[SlotIndex], bGameStarted && FSoWorldState::Get().GetSlotIndex() == SlotIndex);
			else
				SaveSlot->InitializeAsNew();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::ProceedWithSlot(int32 SlotIndex)
{
	if (SaveSlotsData.Contains(SlotIndex))
	{
		if (CanLoadSelectedSlot())
		{
			USoAudioManager::PlaySound2D(this, SFXLoadSlot, true);
			LoadSaveSlot(SlotIndex);
		}
	}
	else
	{
		// Start new game on slot -> proceed to difficulty selection
		ISoUIEventHandler::Execute_Open(DifficultySelection, true);
		Container->SetVisibility(ESlateVisibility::Collapsed);
		RefreshCommandTooltips();

		USoAudioManager::PlaySound2D(this, SFXStartNewSlot, true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::LoadSaveSlot(const int32 SaveSlotIndex)
{
	UE_LOG(LogSoUISaveSlotSelection, Verbose, TEXT("Load save at Slot index = %d"), SaveSlotIndex);

	const auto& WorldState = FSoWorldState::Get();
	USoGameInstance* GameInstance = USoGameInstance::GetInstance(this);
	GameInstance->PauseGame(false);

	// Analytics
	GameInstance->GetAnalytics()->RecordUISaveLoad();

	// Load the current selected slot
	UE_LOG(LogSoUISaveSlotSelection, Verbose, TEXT("Loading the slot = %d"), SaveSlotIndex);
	GameInstance->LoadGameForChapter(SaveSlotIndex, false);

	// Update UI
	GameInstance->ResumeGame(true); // maybe it was brought up by someone else
	SaveSlotLoaded.Broadcast(SaveSlotIndex);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::RemoveSelectedSaveSlot()
{
	if (!CanRemoveSelectedSlot())
		return;

	const int32 SaveSlotIndex = GetSelectedIndex();
	USoGameInstance* GameInstance = USoGameInstance::GetInstance(this);

	UE_LOG(LogSoUISaveSlotSelection, Verbose, TEXT("Remove save at Slot index = %d"), SaveSlotIndex);

	// Analytics
	GameInstance->GetAnalytics()->RecordUISaveRemove();

	// Update file system
	UE_LOG(LogSoUISaveSlotSelection, Verbose, TEXT("Deleting save game at index = %d"), SaveSlotIndex);
	FSoWorldState& WorldState = FSoWorldState::Get();
	WorldState.DeleteGameInSlot(SaveSlotIndex);

	RefreshSlots();
	RefreshCommandTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::OnDifficultySelected(ESoDifficulty SelectedDifficulty)
{
	CachedDifficulty = SelectedDifficulty;

	const int32 SelectedSaveSlotIndex = GetSelectedIndex();
	UE_LOG(LogSoUISaveSlotSelection, Verbose, TEXT("StartNewGameIn at Slot index = %d"), SelectedSaveSlotIndex);

	const auto& WorldState = FSoWorldState::Get();
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (Character != nullptr && Character->SoActivity != nullptr)
		Character->SoActivity->SwitchActivity(Character->SoADefault);

	// Analytics
	USoGameInstance* GameInstance = USoGameInstance::GetInstance(this);
	GameInstance->PauseGame(false);
	GameInstance->GetAnalytics()->RecordUISaveStartNewGame();

	// // Save current slot if we are in game
	// TODO REMOVE THIS?!
	if (GameInstance->IsGameStarted())
	{
		UE_LOG(LogSoUISaveSlotSelection, Verbose, TEXT("Saving current slot = %d"), WorldState.GetSlotIndex());
		GameInstance->SaveGameForCurrentState();
	}

#if WARRIORB_WITH_VIDEO_INTRO
	if (Character != nullptr)
	{
		bIntroPlaying = true;
		USoGameInstance::Get(this).GetAudioManager()->FadeOutActiveMusic();
		BlackFadeRequest.Broadcast();
		CutsceneStartCounter = 1.0f;
	}
#else
	IntroFinished();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::IntroFinished()
{
#if WARRIORB_WITH_VIDEO_INTRO
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	USoUIVideoPlayer* VideoPlayer = Character->GetUIVideoPlayer();
	VideoPlayer->OnVideoFinishedOrSkipped.RemoveDynamic(this, &USoUISaveSlotSelection::IntroFinished);
	VideoPlayer->SetSubtitles(nullptr);
#endif

	const int32 SelectedSaveSlotIndex = GetSelectedIndex();
	// NOTE: saving the current state is done in the LoadGameForChapter (if a game is already loaded)
	// also saving the new state
	// Load the current selected slot
	UE_LOG(LogSoUISaveSlotSelection, Verbose, TEXT("Notify user of changes"));
	USoGameInstance* GameInstance = USoGameInstance::GetInstance(this);
	GameInstance->LoadGameForChapter(SelectedSaveSlotIndex, true, CachedDifficulty);

	// TODO: do we need this?
	// 	GameInstance->ResumeGame(true); // maybe it was brought up by someone else
	SaveSlotLoaded.Broadcast(SelectedSaveSlotIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::OnSelectedChildChanged(int32 OldIndex, int32 NewIndex)
{
	RefreshCommandTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::OnHandleButtonPress(int32 SelectedChild, USoUIUserWidget* SoUserWidget)
{
	ProceedWithSlot(SelectedChild);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::OnClearSaveSlotConfirmQuestionAnswered()
{
	if (ClearSlotConfirmation->IsAnswerYes())
	{
		RemoveSelectedSaveSlot();
		RefreshSlots();
	}

	ISoUIEventHandler::Execute_Open(ClearSlotConfirmation, false);

	Container->SetVisibility(ESlateVisibility::Visible);
	RefreshCommandTooltips();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISaveSlotSelection::RefreshCommandTooltips()
{
	if (CommandTooltips == nullptr)
		return;

	// Restore to default
	CommandTooltips->SetVisibility(ESlateVisibility::Visible);
	CommandTooltips->Clear();

	if (!ISoUIEventHandler::Execute_IsOpened(DifficultySelection) &&
		!ISoUIEventHandler::Execute_IsOpened(ClearSlotConfirmation))
	{
		if (CanLoadSelectedSlot())
			CommandTooltips->AddTooltipFromUICommand(FROM_STRING_TABLE_UI("save_slots_select"), ESoUICommand::EUC_Action0);

		// We can only remove non empty slots
		if (CanRemoveSelectedSlot())
			CommandTooltips->AddTooltipFromUICommand(FROM_STRING_TABLE_UI("save_slots_clear"), ESoUICommand::EUC_Action1);
	}

	// Nothing to show
	if (CommandTooltips->IsEmpty())
		CommandTooltips->SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISaveSlotSelection::CanRemoveSelectedSlot() const
{
	const int32 SlotIndex = GetSelectedIndex();
	if (USoGameInstance* GameInstance = USoGameInstance::GetInstance(this))
		if (GameInstance->IsGameStarted() && FSoWorldState::Get().GetSlotIndex() == SlotIndex)
			return false;

	return (SaveSlotsData.Contains(SlotIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISaveSlotSelection::CanLoadSelectedSlot() const
{
	const int32 SlotIndex = GetSelectedIndex();
	if (USoGameInstance* GameInstance = USoGameInstance::GetInstance(this))
		if (GameInstance->IsGameStarted() && FSoWorldState::Get().GetSlotIndex() == SlotIndex)
			return false;

	if (!SaveSlotsData.Contains(SlotIndex))
		return false;

	if (FSoWorldState::IsStateTableAGameCompleted(SaveSlotsData[SlotIndex]))
		return false;

	return true;
}
