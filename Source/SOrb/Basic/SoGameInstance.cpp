// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoGameInstance.h"

#include "EngineUtils.h"
#include "Misc/CoreDelegates.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "LevelSequenceActor.h"
#include "Components/StaticMeshComponent.h"

#if WARRIORB_WITH_ONLINE
#include "OnlineSubsystem.h"
#endif

#include "UI/Menu/SoUIMenuMain.h"

#include "Enemy/EActions/SoEAction.h"
#include "Enemy/SoEnemyVoiceManager.h"

#include "Settings/Input/SoInputHelper.h"
#include "Settings/SoGameSettings.h"

#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoAWaitForActivitySwitch.h"
#include "Character/SoCharStates/SoATeleport.h"
#include "Character/SoPlayerController.h"
#include "Character/SoPlayerProgress.h"

#include "Levels/SoLevelHelper.h"
#include "Levels/SoLevelManager.h"

#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Basic/Helpers/SoStringHelper.h"
#include "Basic/SoCheatManager.h"

#include "Effects/SoEffectBase.h"
#include "Basic/SoGameMode.h"
#include "SaveFiles/SoWorldState.h"
#include "SoGameViewportClient.h"
#include "Projectiles/SoProjectile.h"

#include "Online/Analytics/SoAnalytics.h"
#include "Online/Analytics/SoAnalyticsComponent.h"
#include "Online/Achievements/SoAchievementManager.h"
#include "Online/SoOnlineHelper.h"


#include "SoLocalization.h"

#if WARRIORB_WITH_VIDEO_DEMO
#include "UI/General/SoUIVideoPlayer.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "FileMediaSource.h"
#endif // WARRIORB_WITH_VIDEO_DEMO

#include "FMODEvent.h"
#include "FMODBus.h"
#include "FMODVCA.h"

#include "INYLoadingScreenModule.h"
#include "SoGameSingleton.h"
#include "SplineLogic/SoMarker.h"

#if WARRIORB_WITH_DISCORD
#include "DiscordHelper.h"
#endif // WARRIORB_WITH_DISCORD


DEFINE_LOG_CATEGORY_STATIC(LogSoGameInstance, All, All)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameInstance::USoGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameInstance::~USoGameInstance()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::Init()
{
	UE_LOG(LogSoGameInstance, Log, TEXT("Init"));

	// Crashing
	CrashHandler.Initialize(this);

	// Disable some stupid stuff
#if !WARRIORB_WITH_EDITOR
	USoPlatformHelper::DisableScreenMessages();
#endif
	USoPlatformHelper::DisableVerifyGC();
	USoPlatformHelper::DisableAILogging();

	// Must update the runtime data if we are playing in Editor
// #if WITH_EDITOR
	// USoGameSingleton::Get().UpdateRuntimeStreamData();
// #endif

	LoadingScreenInstance = INYLoadingScreenModule::Get().GetInstance();

	// Log stuff about the game
	UE_LOG(LogSoGameInstance, Log, TEXT("%s"), *USoPlatformHelper::ToStringPlatformContext());

	// Command line overrides
	{
		FString CheatsPassword;
		if (FParse::Value(FCommandLine::Get(), TEXT("CheatsPassword="), CheatsPassword))
		{
			UE_LOG(LogSoGameInstance, Verbose, TEXT("CommandlineOverride: CheatsPassword = `%s`"), *CheatsPassword);

			if (USoCheatManager::IsAllowCheatsPasswordValid(CheatsPassword))
			{
				UE_LOG(LogSoGameInstance, Log, TEXT("Enabling cheats because of command line override"));
				SetAllowCheats(true);
			}
		}
	}

	Super::Init();

	// Game settings
	USoGameSettings::Get().SetWorldContextObject(this);

	// Check if we are in steam
#if WARRIORB_WITH_STEAM
	USoPlatformHelper::InitializedSteam();
	if (USoPlatformHelper::IsSteamInitialized())
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("Steam is initialized"));
		UE_LOG(
			LogSoGameInstance,
			Log,
			TEXT("AppId = %s, Language = %s, IsSteamBigPicture = %d"),
			*USoPlatformHelper::GetCurrentSteamAppID(), *USoPlatformHelper::GetSteamCurrentGameLanguage(), USoPlatformHelper::IsSteamBigPicture()
		);
	}
	else
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("Steam is NOT initialized"));
	}
	//UE_LOG(LogSoGameInstance, Log, TEXT("IsSteamBuildPirated = %d"), USoPlatformHelper::IsSteamBuildPirated());
#endif // WARRIORB_WITH_STEAM

#if WARRIORB_WITH_DISCORD
	UDiscordHelper::Initialize(USoPlatformHelper::GetDiscordClientID(USoPlatformHelper::IsDemo()));

	UDiscordHelper::UpdatePlayActivity(TEXT("State"), TEXT("Details"));

	#if WARRIORB_WITH_STEAM
	UDiscordHelper::RegisterSteam(USoPlatformHelper::GetSteamAppID(USoPlatformHelper::IsDemo()));
	#endif // WARRIORB_WITH_STEAM

#endif // WARRIORB_WITH_DISCORD

	// Speed run competition
	if (FSoConsoleCommands::IsSpeedRunCompetitionMode())
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("SpeedRunCompetitionMode Enabled"));
	}

	// NOTE: do not init audio here because it is too early in the loading
	// NOTE: do not init achievements here because the PlayerController is not loaded yet
	InitializeAnalytics();

	// TODO: focus receive/lost does not also work on linux
	// Listen for gamepad/controller changes. TODO: does not seem to be on linux
	FCoreDelegates::OnControllerConnectionChange.AddUObject(this, &ThisClass::HandleOnGamepadConnectionChange);
	//FCoreDelegates::OnControllerPairingChange.AddUObject(this, &ThisClass::HandleOnGamepadPariringChange);
	// NOTE: FCoreDelegates::OnControllerPairingChange seems to not used
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMapWithWorld);

	if (USoPlatformHelper::IsDemo())
	{
		UseSavesForDemo();
	}

#if WARRIORB_WITH_ONLINE
	if (USoPlatformHelper::IsXboxOne())
	{
		if (const auto OnlineSub = IOnlineSubsystem::Get())
		{
			const auto IdentityInterface = OnlineSub->GetIdentityInterface();
			if (IdentityInterface.IsValid())
			{
				IdentityInterface->AddOnControllerPairingChangedDelegate_Handle(FOnControllerPairingChangedDelegate::CreateUObject(this, &USoGameInstance::HandleControllerPairingChanged));

				for (int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
				{
					IdentityInterface->AddOnLoginStatusChangedDelegate_Handle(i, FOnLoginStatusChangedDelegate::CreateUObject(this, &USoGameInstance::HandleUserLoginChanged));
				}
			}
		}
	}
#endif

	ApplicationWillDeactivateDelegate = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &ThisClass::HandleApplicationWillDeactivate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::Shutdown()
{
	if (ApplicationWillDeactivateDelegate.IsValid())
	{
		FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(ApplicationWillDeactivateDelegate);
		ApplicationWillDeactivateDelegate.Reset();
	}

	UE_LOG(LogSoGameInstance, Log, TEXT("Shutdown"));
	FCoreDelegates::OnControllerConnectionChange.RemoveAll(this);
	CrashHandler.Shutdown();

	ShutdownAudio();
	ShutdownAnalytics();

	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	LoadingScreenInstance.Reset();
	Super::Shutdown();
	// NOTE: Do not save here as the World is already destroyed (nullptr)
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StartGameInstance()
{
	// TODO do not load default map here, load the current player chapter name

	// NOTE: this is only called in non editor builds
	Super::StartGameInstance();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnPreLoadMap(const FString& MapName)
{
	UE_LOG(LogSoGameInstance, Verbose, TEXT("OnPreLoadMap MapName = `%s`"), *MapName);
	bIsTransitioningWorlds = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnPostLoadMapWithWorld(UWorld* InLoadedWorld)
{
	UE_LOG(LogSoGameInstance, Verbose, TEXT("OnPostLoadMapWithWorld MapName = `%s`"), *InLoadedWorld->GetMapName());
	bIsTransitioningWorlds = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	return Super::HandleOpenCommand(Cmd, Ar, InWorld);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::LoadComplete(float LoadTime, const FString& MapName)
{
	// NOTE: This seems to be called after GameMode::StartPlay and every actor BeginPlay
	Super::LoadComplete(LoadTime, MapName);
	UE_LOG(LogSoGameInstance, Verbose, TEXT("LoadComplete: Loaded MapName = `%s` in LoadTime = %f"), *MapName, LoadTime);

	// Reopen menu and pause game
	bool bNewMapLoaded = false;
	if (State == ESoGameInstanceState::LoadingChapter)
	{
		State = ESoGameInstanceState::LoadingChapterCompleted;
		bNewMapLoaded = true;
		LoadGameForCurrentChapter(false);
	}
	else if (State == ESoGameInstanceState::LoadingEpisode)
	{
		State = ESoGameInstanceState::LoadingEpisodeCompleted;
		bNewMapLoaded = true;
		LoadGameForCurrentEpisode(false);
	}
	else if (State == ESoGameInstanceState::LoadingMenu)
	{
		State = ESoGameInstanceState::LoadingMenuCompleted;
		bNewMapLoaded = true;
	}

	if (bNewMapLoaded)
		ShowLoadingScreen();

	// Open main menu again because the game is not started yet
	OpenMenuInstant();
	// PauseGame(!IsGameStarted());
	InitializeAudio();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameInstance* USoGameInstance::GetInstance(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
		return nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		return Cast<USoGameInstance>(World->GetGameInstance());

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::AllowCheats() const
{
	// Always allow cheats in editor (PIE now supports networking)
	return GIsEditor || bAllowCheats;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::SetAllowCheats(bool bAllow)
{
	bAllowCheats = true;
	if (auto* GameMode = ASoGameMode::GetInstance(this))
		GameMode->ReloadConsoleCommands();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::BeforeGameModeStartPlay()
{
	// Init the audio system before applying settings
	InitializeAudio();

	// Figure out the current level
	SetStateToCurrentLevelType();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::AfterGameModeStartPlay()
{
	InitializeAchievements();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::BeforeGameModeEndPlay()
{
	ShutdownAchievements();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnLoadingCompleted()
{
	if (bWasDeactivatedDuringLoading)
	{
		bWasDeactivatedDuringLoading = false;
		PauseGame(true, false);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::Tick(float DeltaSeconds)
{
	if (State == ESoGameInstanceState::LoadingChapterCompleted)
	{
		if (!SafeUpdateLoadingScreen(DeltaSeconds, IsGameStarted()))
		{
			UE_LOG(LogSoGameInstance, Log, TEXT("Loading new chapter map finished"));
			State = ESoGameInstanceState::Default;
			VerifyStateMatchesCurrentLevelType();
			OnLoadingCompleted();
		}
	}
	else if (State == ESoGameInstanceState::LoadingEpisodeCompleted)
	{
		if (!SafeUpdateLoadingScreen(DeltaSeconds, true))
		{
			UE_LOG(LogSoGameInstance, Log, TEXT("Loading new episode map finished"));
			State = ESoGameInstanceState::PlayingEpisode;
			VerifyStateMatchesCurrentLevelType();
			OnLoadingCompleted();
		}
	}
	else if (State == ESoGameInstanceState::LoadingMenuCompleted)
	{
		if (!SafeUpdateLoadingScreen(DeltaSeconds, false))
		{
			UE_LOG(LogSoGameInstance, Log, TEXT("Loading menu level finished"));
			State = ESoGameInstanceState::InMainMenu;
			bWasDeactivatedDuringLoading = false;
		}
	}

	if (AudioManager)
		AudioManager->Tick(DeltaSeconds);

	if (AchievementManager)
		AchievementManager->Tick(DeltaSeconds);

	// Loading Screen
	if (EnterSequenceTimer > 0.0f)
	{
		EnterSequenceTimer -= DeltaSeconds;
		if (EnterSequenceTimer <= 0.0f)
		{
			EnterSequenceTimer = -1.0f;
			StartEnterSequence();
		}
	}

#if WARRIORB_WITH_DISCORD
	UDiscordHelper::RunCallbacks();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StaticMeshHideRequest(AStaticMeshActor* StaticMeshActor, bool bHide)
{
	if (StaticMeshActor == nullptr)
		return;

	for (int32 i = 0; i < StaticMeshHideRequests.Num(); ++i)
	{
		if (StaticMeshHideRequests[i].Actor == StaticMeshActor)
		{
			StaticMeshHideRequests[i].RequestCount += bHide ? 1 : -1;
			if (StaticMeshHideRequests[i].RequestCount <= 0)
			{
				StaticMeshHideRequests.RemoveAtSwap(i);
				StaticMeshActor->GetStaticMeshComponent()->SetVisibility(true);
			}

			return;
		}
	}

	if (bHide)
	{
		FSoHideRequests& Request = StaticMeshHideRequests.Add_GetRef({});
		Request.Actor = StaticMeshActor;
		Request.RequestCount = 1;
		StaticMeshActor->GetStaticMeshComponent()->SetVisibility(false);
	}
}

#if WARRIORB_WITH_ONLINE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::HandleLoginUIClosed(TSharedPtr<const FUniqueNetId> UniqueId, const int ControllerIndex, const struct FOnlineError& Error)
{
	UE_LOG(LogTemp, Warning, TEXT("HandleLoginUIClosed: UniqueId: %s ControllerIndex: %d"), *UniqueId->ToString(), ControllerIndex);

	APlayerController* PC = USoStaticHelper::GetPlayerController(this);

	if (PC == nullptr || PC->GetLocalPlayer() == nullptr)
		return;

	FUniqueNetIdRepl CurrentNetID = PC->GetLocalPlayer()->GetUniqueNetIdFromCachedControllerId();
	UE_LOG(LogTemp, Warning, TEXT("Current UniqueId: %s"), *(CurrentNetID.IsValid() ? CurrentNetID->ToString() : FString("INVALID")));

	// no switch
	if (!UniqueId.IsValid() || !(*UniqueId).IsValid())
	{
		return;
	}

	if (PC->GetLocalPlayer()->GetUniqueNetIdFromCachedControllerId() != UniqueId)
	{
		if (USoOnlineHelper::bWaitForFirstControllerInput)
		{
			USoOnlineHelper::DestroyLocalPlayers(FindLocalPlayerFromUniqueNetId(UniqueId));
			USoOnlineHelper::bWaitForFirstControllerInput = false;
		}

		PC->GetLocalPlayer()->SetControllerId(ControllerIndex);
		FSoWorldState::Get().SetUserIndex(ControllerIndex);
		FSoWorldState::Get().ReloadGameMetaData();
		// CachedControllerNetID = UniqueId;
		// SetBeforeMenuShownOnce(false);
		TeleportToMainMenu(false, true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId)
{
	const bool bSignedOut = (LoginStatus != ELoginStatus::LoggedIn);
	UE_LOG(LogOnline, Log, TEXT("HandleUserLoginChanged: bSignedOut: %i"), (int)bSignedOut);

	if (!bSignedOut)
		return;

	if (LocalPlayers.Num() > 1)
	{
		UE_LOG(LogOnline, Log, TEXT("HandleUserLoginChanged: login screen - ignored"));
		return;
	}

	if (LocalPlayers.Num() == 0)
		return;

	FUniqueNetIdRepl OtherUniqueNetId = LocalPlayers[0]->GetPreferredUniqueNetId();
	if (OtherUniqueNetId.IsValid() &&
		*OtherUniqueNetId == UserId)
	{
		if (!IsLoadingMenu())
		{
			FSoWorldState::Get().SetUserIndex(-1);
			SetBeforeMenuShownOnce(false);
			TeleportToMainMenu(false, true);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::HandleControllerPairingChanged(int GameUserIndex, const FUniqueNetId& PreviousUser, const FUniqueNetId& NewUser)
{
	UE_LOG(LogOnlineGame, Log, TEXT("USoGameInstance::HandleControllerPairingChanged GameUserIndex %d PreviousUser '%s' NewUser '%s'"),
		GameUserIndex, *PreviousUser.ToString(), *NewUser.ToString());

	if ( PreviousUser.IsValid() && !NewUser.IsValid() )
	{
		return;
	}

	if ( !PreviousUser.IsValid() && NewUser.IsValid() )
	{
		return;
	}

	// Find the local player currently being controlled by this controller
	ULocalPlayer * ControlledLocalPlayer	= FindLocalPlayerFromControllerId( GameUserIndex );

	// See if the newly assigned profile is in our local player list
	ULocalPlayer * NewLocalPlayer			= FindLocalPlayerFromUniqueNetId( NewUser );

	// If the local player being controlled is not the target of the pairing change, then give them a chance
	// to continue controlling the old player with this controller
	if ( ControlledLocalPlayer != nullptr && ControlledLocalPlayer != NewLocalPlayer )
	{
		if (const auto OnlineSub = IOnlineSubsystem::Get())
		{
			const auto IdentityInterface = OnlineSub->GetIdentityInterface();
			check(IdentityInterface.IsValid());

			// does not seem to work because the netid is controllerid related and everything gets confusing and meh
			// let's just return to the menu

			//for (int32 i = 0; i < MAX_LOCAL_PLAYERS; ++i)
			//{
			//	TSharedPtr<const FUniqueNetId> PlayerID = IdentityInterface->GetUniquePlayerId(i);
			//	if (PlayerID.IsValid() && *PlayerID == NewUser)
			//	{
			//		USoStaticHelper::GetPlayerController(this)->GetLocalPlayer()->SetControllerId(i);
			//		return;
			//	}
			//}

			FSoWorldState::Get().SetUserIndex(-1);
			SetBeforeMenuShownOnce(false);
			TeleportToMainMenu(false, true);
		}
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::HandleApplicationWillDeactivate()
{
	if (IsLoading())
	{
		bWasDeactivatedDuringLoading = true;
	}
	PauseGame(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::UseSavesForDemo()
{
	FSoWorldState::Get().UseForEpisode(0, USoLevelHelper::GetDemoMapName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::LoadGameForDemo(bool bNewGame)
{
	// Demo Game
	return LoadGameForEpisode(USoLevelHelper::GetDemoMapName(), bNewGame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::LoadGameFromCurrentState(bool bNewGame)
{
	if (USoPlatformHelper::IsDemo())
	{
		LoadGameForDemo(bNewGame);
	}
	else
	{
		// Normal game
		if (IsChapter() || IsBrowsingMenu())
		{
			// Chapter
#if !WARRIORB_WITH_EDITOR
			// Load the default save game in non editor builds (also loads the metadata)
			LoadGameForCurrentChapter(bNewGame);
#endif
		}
		else if (IsEpisode())
		{
			// Episode
			LoadGameForCurrentEpisode(bNewGame);
		}
	}

	bIsSavingOrLoading = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::SaveGameForCurrentState(bool bForce)
{
#if !WARRIORB_WITH_EDITOR
	if (IsChapter())
	{
		// Chapter
		SaveGameForCurrentChapter(bForce);
	}
	else if (IsEpisode())
	{
		// Episode
		SaveGameForCurrentEpisode(bForce);
	}
#endif
	bIsSavingOrLoading = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::CanSaveGame(FString& OutMessage) const
{
	if (bIsSavingOrLoading)
	{
		OutMessage = TEXT(" bIsSavingOrLoading = true. Ignoring.");
		return false;
	}
	if (!IsGameStarted())
	{
		OutMessage = TEXT("Game is NOT started. Ignoring.");
	}

	if (ASoCharacter* Char = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		if (!Char->IsSaveAllowed())
		{
			OutMessage = TEXT("Character is not allowing use to save. Ignoring.");
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::SaveGameForCurrentChapter(bool bForce)
{
	return SaveGameForChapter(FSoWorldState::Get().GetSlotIndex(), bForce);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::SaveGameForCurrentEpisode(bool bForce)
{
	return SaveGameForEpisode(FSoWorldState::Get().GetEpisodeName(), bForce);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::SaveGameForChapter(int32 SlotIndex, bool bForce)
{
	{
		FString WarningMessage;
		if (!bForce && !CanSaveGame(WarningMessage))
		{
			UE_LOG(LogSoGameInstance, Warning, TEXT("SaveGameForChapter - %s"), *WarningMessage);
			return false;
		}
	}

	// Must make sure we are in a chapter
	if (!IsChapter())
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("SaveGameForChapter - !IsChapter(). Ignoring."));
		return false;
	}

	OnSaveToSlot.Broadcast();

	// Save
	bIsSavingOrLoading = true;
	FSoWorldState::Get().UseForChapter(SlotIndex);
	ASoGameMode::Get(this).TriggerPreSaveGame();
	const ESoSaveStatus ReturnStatus = FSoWorldState::Get().SaveGame();
	bIsSavingOrLoading = false;

	if (FSoWorldState::IsSaveStatusSuccessful(ReturnStatus))
		return true;

	FSoWorldState::LogSaveStatusIfError(
		ReturnStatus,
		FString::Printf(TEXT("USoGameInstance::SaveGameForChapter, SlotIndex = %d"), SlotIndex)
	);
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::SaveGameForEpisode(FName EpisodeName, bool bForce)
{
	{
		FString WarningMessage;
		if (!bForce && !CanSaveGame(WarningMessage))
		{
			UE_LOG(LogSoGameInstance, Warning, TEXT("SaveGameForEpisode - %s"), *WarningMessage);
			return false;
		}
	}

	// Make sure we are in an episode
	if (!IsEpisode())
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("SaveGameForEpisode - !IsEpisode() || !IsGameStarted(). Ignoring."));
		return false;
	}
	if (!USoLevelHelper::IsValidEpisodeName(EpisodeName))
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("SaveGameForEpisode - EpisodeName = %s does NOT exist"), *EpisodeName.ToString());
		return false;
	}

	// Save
	bIsSavingOrLoading = true;
	FSoWorldState::Get().UseForEpisode(0, EpisodeName);
	ASoGameMode::Get(this).TriggerPreSaveGame();
	const ESoSaveStatus ReturnStatus = FSoWorldState::Get().SaveGame();
	bIsSavingOrLoading = false;

	if (FSoWorldState::IsSaveStatusSuccessful(ReturnStatus))
		return true;

	FSoWorldState::LogSaveStatusIfError(
		ReturnStatus,
		FString::Printf(TEXT("USoGameInstance::SaveGameForEpisode, EpisodeName = %s"), *EpisodeName.ToString())
	);
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::LoadGameForCurrentChapter(bool bNewGame)
{
	UE_LOG(LogSoGameInstance, Verbose, TEXT("LoadGame"));
	if (bIsSavingOrLoading)
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("LoadGame - bIsSavingOrLoading = true. Ignoring."));
		return false;
	}

	const auto& WorldState = FSoWorldState::Get();
	return LoadGameForChapter(WorldState.GetSlotIndex(), bNewGame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::LoadGameForCurrentEpisode(bool bNewGame)
{
	UE_LOG(LogSoGameInstance, Verbose, TEXT("LoadGameForCurrentEpisode"));
	if (bIsSavingOrLoading)
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("LoadGameForCurrentEpisode - bIsSavingOrLoading = true. Ignoring."));
		return false;
	}

	return LoadGameForEpisode(FSoWorldState::Get().GetEpisodeName(), bNewGame);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::LoadGameForChapter(int32 SlotIndex, bool bNewGame, ESoDifficulty DifficultyForNewGame)
{
	FString ThisContext = FString::Printf(
		TEXT("LoadGameForChapter(SlotIndex = %d bNewGame = %d)"),
		SlotIndex, bNewGame
	);
	UE_LOG(LogSoGameInstance, Log, TEXT("%s"), *ThisContext);

	// Check Name
	if (bIsSavingOrLoading)
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("%s - bIsSavingOrLoading = true. Ignoring."), *ThisContext);
		return false;
	}

	// Mark as started
	SetGameStarted(true);

	// Save current world state first
	if (IsPlaying())
		SaveGameForCurrentState();

	// Load game world state
	bIsSavingOrLoading = true;
	FSoWorldState::Get().UseForChapter(SlotIndex);
	if (bNewGame)
	{
		// New
		FSoWorldState::Get().ResetToDefault(DifficultyForNewGame);
		FSoWorldState::Get().SaveGame();
	}
	else
	{
		// Existing
		const ESoSaveStatus ReturnStatus = FSoWorldState::Get().LoadGame();
		if (!FSoWorldState::IsSaveStatusSuccessful(ReturnStatus))
		{
			bIsSavingOrLoading = false;
			FSoWorldState::LogSaveStatusIfError(ReturnStatus,ThisContext);
			return false;
		}
	}

	// Set ChapterName and checkpoint from the save
	// TODO: make customizable?
	const FName ChapterName = FSoWorldState::Get().GetMapName();
	const FName CheckpointName = FSoWorldState::Get().GetCheckpointLocation();
	ThisContext = FString::Printf(
		TEXT("LoadGameForChapter(SlotIndex = %d, ChapterName = %s, CheckpointName = %s, bNewGame = %d)"),
		SlotIndex, *ChapterName.ToString(), *CheckpointName.ToString(), bNewGame
	);
	if (!USoLevelHelper::IsValidChapterName(ChapterName))
	{
		// Fail this time
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - ChapterName does NOT exist"), *ThisContext);
		return false;
	}

#if !WARRIORB_WITH_EDITOR
	// If loaded chapter is different we must teleport to that chapter before anything else.
	const FName CurrentChapterName = USoLevelHelper::GetChapterNameFromObject(this);
	// if (!USoLevelHelper::IsValidChapterName(CurrentChapterName) || CurrentChapterName != ChapterName)
	if (State != ESoGameInstanceState::LoadingChapterCompleted)
	{
		// UE_LOG(
		// 	LogSoGameInstance,
		// 	Log,
		// 	TEXT("%s - CurrentChapterName (%s) != ChapterName (%s). Teleporting to new chapter"),
		// 	*ThisContext, *CurrentChapterName.ToString(), *ChapterName.ToString()
		// );

		bIsSavingOrLoading = false;
		if (!TeleportToChapter(ChapterName, CheckpointName, false))
		{
			UE_LOG(LogSoGameInstance, Error, TEXT("%s - Could not teleport to ChapterName"), *ThisContext);
			return false;
		}

		return true;
	}
#endif

	// We are in the same chapter
	UE_LOG(LogSoGameInstance, Log, TEXT("%s - We are in the same chapter."), *ThisContext);
	StartEnterSequenceFromSameLevel();
	ASoGameMode::Get(this).TriggerPostLoadGame();

	SetGameStarted(true);
	bIsSavingOrLoading = false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::LoadGameForEpisode(FName EpisodeName, bool bNewGame)
{
	const FString ThisContext = FString::Printf(
		TEXT("LoadGameForEpisode(EpisodeName = %s, bNewGame = %d)"),
		*EpisodeName.ToString(), bNewGame
	);
	UE_LOG(LogSoGameInstance, Log, TEXT("%s"), *ThisContext);

	// Check Name
	if (bIsSavingOrLoading)
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("%s - bIsSavingOrLoading = true. Ignoring."), *ThisContext);
		return false;
	}
	if (!USoLevelHelper::IsValidEpisodeName(EpisodeName))
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - EpisodeName does NOT exist"), *ThisContext);
		return false;
	}

	// Mark as started
	SetGameStarted(true);

	// Save current world state first
	if (IsPlaying())
		SaveGameForCurrentState();

	// Load new game world state
	bIsSavingOrLoading = true;
	FSoWorldState::Get().UseForEpisode(0, EpisodeName);
	if (bNewGame)
	{
		// New
		FSoWorldState::Get().ResetToDefault();
		FSoWorldState::Get().SaveGame();
	}
	else
	{
		// Existing
		const ESoSaveStatus ReturnStatus = FSoWorldState::Get().LoadGame();
		if (!FSoWorldState::IsSaveStatusSuccessful(ReturnStatus))
		{
			bIsSavingOrLoading = false;
			FSoWorldState::LogSaveStatusIfError(ReturnStatus,ThisContext);
			return false;
		}
	}

	// If loaded episode is different we must teleport to that episode before anything else.
	const FName CurrentEpisodeName = USoLevelHelper::GetEpisodeNameFromObject(this);
	if (!USoLevelHelper::IsValidEpisodeName(CurrentEpisodeName) || CurrentEpisodeName != EpisodeName)
	{
		UE_LOG(
			LogSoGameInstance,
			Log,
			TEXT("%s - CurrentMapName (%s) != EpisodeName (%s). Teleporting to new episode"),
			*ThisContext, *CurrentEpisodeName.ToString(), *EpisodeName.ToString()
		);

		bIsSavingOrLoading = false;
		if (!TeleportToEpisode(EpisodeName, false))
		{
			UE_LOG(LogSoGameInstance, Error, TEXT("%s - Could not teleport to EpisodeName"), *ThisContext);
			return false;
		}

		return true;
	}

	// We return here after we are teleported into the right episode. See LoadComplete()
	// We are in the same episode
	UE_LOG(LogSoGameInstance, Log, TEXT("%s - We are in the same episode."), *ThisContext);
	StartEnterSequenceFromSameLevel();
	ASoGameMode::Get(this).TriggerPostLoadGame();

	bIsSavingOrLoading = false;
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::TeleportToChapter(FName ChapterName, FName CheckpointName, bool bSave)
{
	const FString ThisContext = FString::Printf(
		TEXT("TeleportToChapter(ChapterName = %s, CheckpointName = %s, bSave = %d)"),
		*ChapterName.ToString(), *CheckpointName.ToString(), bSave
	);
	UE_LOG(LogSoGameInstance, Log, TEXT("%s"), *ThisContext);

	// Check Name
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - World is null"), *ThisContext);
		return false;
	}
	if (!USoLevelHelper::IsValidChapterName(ChapterName))
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - ChapterName is Invalid"), *ThisContext);
		return false;
	}

	// We are already in the same chapter, ignore
	// const FName CurrentChapterName = USoLevelHelper::GetChapterNameFromObject(this);
	// if (ChapterName == CurrentChapterName)
	// {
	// 	UE_LOG(
	// 		LogSoGameInstance,
	// 		Warning,
	// 		TEXT("%s - CurrentChapterName (%s) == ChapterName (%s). Ignoring"),
	// 		*ThisContext, *CurrentChapterName.ToString(), *ChapterName.ToString()
	// 	);
	// 	// TODO: teleport to checkpoint?
	// 	return false;
	// }

	LastLoadedMapName = ChapterName;
	if (!ShowLoadingScreen())
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - Can't open loading screen"), *ThisContext);
		return false;
	}
	if (!PauseGame(false, false))
		UE_LOG(LogSoGameInstance, Log, TEXT("%s - Could not pause game"), *ThisContext);

	// Save current world state first then clear the old data
	if (bSave && IsPlaying())
	{
		SaveGameForCurrentState(true);
		// NOTE: we can't clear the data here
	}

	// Teleport to Chapter Level
	verify(!IsLoading());
	State = ESoGameInstanceState::LoadingChapter;

	// NOTE: this does not affect any actors
	// Set chapter name and default checkpoint
	FSoWorldState::Get().UseForChapter(FSoWorldState::Get().GetSlotIndex());
#if !WARRIORB_WITH_EDITOR
	FSoWorldState::Get().LoadGame();
#endif
	FSoWorldState::Get().SetMapName(ChapterName);
	FSoWorldState::Get().SetCheckpointLocation(CheckpointName);
#if !WARRIORB_WITH_EDITOR
	FSoWorldState::Get().SaveGame();
#endif

	const bool bReturnStatus = FSoLevelManager::Get().TeleportToChapter(this, ChapterName);
	if (bReturnStatus)
	{
		SetGameStarted(true);
	}
	else
	{
		ResetState();
		ResetEpisode();
	}

	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::TeleportToEpisode(FName EpisodeName, bool bSave)
{
	const FString ThisContext = FString::Printf(
		TEXT("TeleportToEpisode(EpisodeName = %s, bSave = %d)"),
		*EpisodeName.ToString(), bSave
	);
	UE_LOG(LogSoGameInstance, Log, TEXT("%s"), *ThisContext);

	// Check Name
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - World is null"), *ThisContext);
		return false;
	}
	if (!USoLevelHelper::IsValidEpisodeName(EpisodeName))
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - EpisodeName is Invalid"), *ThisContext);
		return false;
	}

	// We area already in the same episode ignore
	const FName CurrentEpisodeName = USoLevelHelper::GetEpisodeNameFromObject(this);
	if (CurrentEpisodeName == EpisodeName)
	{
		UE_LOG(
			LogSoGameInstance,
			Warning,
			TEXT("%s - EpisodeName (%s) == EpisodeName (%s). Ignoring"),
			*ThisContext, *CurrentEpisodeName.ToString(), *EpisodeName.ToString()
		);
		return false;
	}

	LastLoadedMapName = EpisodeName;
	if (!ShowLoadingScreen())
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("%s - Can't open loading screen"), *ThisContext);
		return false;
	}
	if (!PauseGame(false, false))
		UE_LOG(LogSoGameInstance, Log, TEXT("%s - Could not pause game"), *ThisContext);

	// Save current world state first then clear the old data
	if (bSave && IsPlaying())
	{
		SaveGameForCurrentState();
		FSoWorldState::Get().ResetToDefault();
	}

	// Teleport to Episode level
	verify(!IsLoading());
	State = ESoGameInstanceState::LoadingEpisode;

	FSoWorldState::Get().UseForEpisode(0, EpisodeName);

	const bool bReturnStatus = FSoLevelManager::Get().TeleportToEpisode(this, EpisodeName);
	if (bReturnStatus)
	{
		SetGameStarted(true);
	}
	else
	{
		ResetState();
		ResetEpisode();
	}

	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::TeleportToMainMenu(bool bSave, bool bForceReloadIfAlreadyIn)
{
	UE_LOG(LogSoGameInstance, Verbose, TEXT("TeleportToMainMenu"));

	if (!bForceReloadIfAlreadyIn && IsBrowsingMenu())
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("TeleportToMainMenu - We are already in the menu level"));
		return false;
	}
	LastLoadedMapName = NAME_None;
	if (State == ESoGameInstanceState::LoadingChapterCompleted ||
		State == ESoGameInstanceState::LoadingEpisodeCompleted)
	{
		if (LoadingScreenInstance->IsLoadingScreenVisible())
		{
			LoadingScreenInstance->SetWaitForAnyKeyInput(false);
			LoadingScreenInstance->HideLoadingScreen();
			bWasDeactivatedDuringLoading = false;
		}
	}
	if (!ShowLoadingScreen())
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("TeleportToChapter - Can't open chapters loading screen"));
		return false;
	}
	if (!PauseGame(false, false))
		UE_LOG(LogSoGameInstance, Warning, TEXT("TeleportToChapter -  Could not pause game"));

	// Save current world state first then clear the old data
	if (bSave && IsPlaying())
	{
		SaveGameForCurrentState();
		FSoWorldState::Get().ResetToDefault();
	}

	// Teleport to menu
	// verify(!IsLoading()); // is possible now
	State = ESoGameInstanceState::LoadingMenu;
	const bool bReturnStatus = FSoLevelManager::Get().TeleportToMainMenuLevel(this);
	if (bReturnStatus)
	{
		// Like a new start
		SetGameStarted(false);
	}
	else
	{
		ResetState();
	}

	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ClearConfigData()
{
	ActionLists.Empty();
	StrikeMaps.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ReinitializeActionList(FName ListName, UObject* Outer, TArray<USoEAction*>& OutActions)
{
	OutActions.Empty();

	// We are in menu, ignore
	if (IsMenu())
		return;

	// not yet loaded, let's load it
	FDlgConfigParser* ActionParser = ActionLists.Find(ListName);
	if (ActionParser == nullptr)
	{
		const FString Path = FPaths::ProjectContentDir() + TEXT("SO/AIConfig/ActionLists/") + ListName.ToString() + ".sc";
		ActionParser = &ActionLists.Add(ListName, FDlgConfigParser{ Path,"So" });
	}

	if (ActionParser != nullptr)
	{
		ActionParser->ResetParser();

		if (ActionParser->IsValidFile())
		{
			FSoEActions Actions;
			ActionParser->ReadProperty(FSoEActions::StaticStruct(), &Actions, this); // duplicate object was shallow copy, easiest way to do this is to initialize from text for everyone :(
			OutActions = Actions.Array;
		}
		else
			UE_LOG(LogSoGameInstance, Warning, TEXT("Failed to read action list %s - (.sc file is missing or empty)"), *ListName.ToString());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ReinitializeStrikeMap(FName MapName, TMap<FName, FSoStrike>& StrikeMap)
{
	StrikeMap.Empty();

	// We are in menu, ignore
	if (IsMenu())
		return;

	// not yet loaded, let's load it - TODO: maybe preload in BeginPlay()???
	FSoEStrikes* StrikesPtr = StrikeMaps.Find(MapName);
	if (StrikesPtr == nullptr)
	{
		const FString Path = FPaths::ProjectContentDir() + TEXT("SO/AIConfig/StrikeMaps/") + MapName.ToString() + ".sc";
		FDlgConfigParser Parser(Path, "So");
		if (Parser.IsValidFile())
		{
			FSoEStrikes& Strikes = StrikeMaps.Add(MapName);
			Parser.ReadProperty(FSoEStrikes::StaticStruct(), &Strikes, nullptr);
			StrikesPtr = &Strikes;
		}
		else
			UE_LOG(LogSoGameInstance, Warning, TEXT("Failed to find strike %s - (.sc file is missing or empty)"), *MapName.ToString());
	}

	if (StrikesPtr != nullptr)
		StrikeMap = StrikesPtr->StrikeMap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEffectBase* USoGameInstance::ClaimEffect(TSubclassOf<USoEffectBase> Class)
{
	USoEffectBase* Effect = ClaimObject(EffectPool, Class);
	return Effect == nullptr ? NewObject<USoEffectBase>(this, Class) : Effect;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile* USoGameInstance::ClaimProjectile(TSubclassOf<ASoProjectile> Class)
{
	ASoProjectile* Projectile = ClaimObject(ProjectilePool, Class);
	if (Projectile == nullptr)
		Projectile = Cast<ASoProjectile>(GetWorld()->SpawnActor(Class));

	return Projectile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ClearObjectPools()
{
	ProjectilePool.Empty();
	EffectPool.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ReturnEffect(USoEffectBase* Effect) { ReturnObject(EffectPool, Effect); }
void USoGameInstance::ReturnProjectile(ASoProjectile* Projectile) { ReturnObject(ProjectilePool, Projectile); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::InitializeAnalytics()
{
	if (AnalyticsInstance != nullptr)
		return;

	const auto& GameSettings = USoGameSettings::Get();
	const bool bCollectGameAnalytics = GameSettings.CanCollectGameAnalytics();
	const bool bWaitForAnalyticsToSend = GameSettings.CanWaitForAnalyticsToSend();
	double PollWaitSeconds = 2.0;
	double PollProcessEventsSeconds = 8.0;
	GameSettings.GetAnalyticsPollTimers(PollWaitSeconds, PollProcessEventsSeconds);

	AnalyticsInstance = NewObject<USoAnalytics>(this);
	AnalyticsInstance->StartSession(bCollectGameAnalytics, bWaitForAnalyticsToSend, PollWaitSeconds, PollProcessEventsSeconds);

	// TODO make this triggerable from the game settings
	// if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	// 	if (USoAnalyticsComponent* Analytics = SoCharacter->GetAnalyticsComponent())
	// 		Analytics->SetCanCollectAnalytics(bCollectGameAnalytics);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ShutdownAnalytics()
{
	if (AnalyticsInstance)
		AnalyticsInstance->EndSession();

	AnalyticsInstance = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAnalytics* USoGameInstance::GetAnalytics()
{
	InitializeAnalytics();
	return AnalyticsInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::InitializeAchievements()
{
	// Skip if already initialized
	if (AchievementManager && AchievementManager->IsInitialized())
		return true;

	if (AchievementManager == nullptr)
		AchievementManager = NewObject<USoAchievementManager>(this);

	if (APlayerController* Controller = USoStaticHelper::GetPlayerController(this))
	{
		return AchievementManager->Initialize(Controller);
	}
	else
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("InitializeAchievements Can't get PlayerController. Most likely you called this too early and the World is not initialized"));
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ShutdownAchievements()
{
	if (AchievementManager != nullptr)
	{
		AchievementManager->Shutdown();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAchievementManager* USoGameInstance::GetAchievementManager() const
{
	return AchievementManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAudioManager* USoGameInstance::GetAudioManager()
{
	if (AudioManager == nullptr)
		InitializeAudio();

	ensure(AudioManager != nullptr);
	return AudioManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEnemyVoiceManager* USoGameInstance::GetEnemyVoiceManager()
{
	if (EnemyVoiceManager == nullptr)
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("Initialize EnemyVoiceManager"));
		EnemyVoiceManager = NewObject<USoEnemyVoiceManager>(this);
	}

	return EnemyVoiceManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::InitializeAudio()
{
	if (AudioManager == nullptr)
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("Initialize Audio"));
		AudioManager = NewObject<USoAudioManager>(this);
		verify(AudioManager != nullptr);
		AudioManager->Initialize();
		AudioManager->SetMusic(USoGameSingleton::Get().MenuMusicPtr.Get(), true);
		AudioManager->SetMusicRestartParams(
			USoGameSingleton::Get().MinWaitBeforeMusicRestart,
			USoGameSingleton::Get().MaxWaitBeforeMusicRestart
		);
		AudioManager->SetPlaybackSpeedIgnoreList(USoGameSingleton::Get().BusesToIgnoreForSpeedChangeArrayPtr);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ShutdownAudio()
{
	if (AudioManager != nullptr)
	{
		AudioManager->Shutdown();
		AudioManager = nullptr;
		UE_LOG(LogSoGameInstance, Log, TEXT("Shutdown Audio"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::HandleOnGamepadConnectionChange(bool bConnected, FPlatformUserId UserID, int32 GamepadID)
{
	UE_LOG(
		LogSoGameInstance,
		Log,
		TEXT("Gamepad connection status changed:") LINE_TERMINATOR
		TEXT("\tbConnected = %d") LINE_TERMINATOR
		TEXT("\tUserID = %d") LINE_TERMINATOR
		TEXT("\tGamepadID = %d") LINE_TERMINATOR,
		bConnected, UserID, GamepadID
	);

	if (IsPlaying() && !bConnected)
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("Pausing game because controller disconnected"));
		PauseGame(true, false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::HandleOnGamepadPariringChange(int32 ControllerIndex, FPlatformUserId NewUserPlatformId, FPlatformUserId OldUserPlatformId)
{
	// NOTE: this seems to be not used
	//UE_LOG(LogSoGameInstance, Log,
	//	TEXT("Gamepad pairging changed:") LINE_TERMINATOR
	//	TEXT("\tControllerIndex = %d") LINE_TERMINATOR
	//	TEXT("\tNewUserPlatformId = %d") LINE_TERMINATOR
	//	TEXT("\tOldUserPlatformId = %d") LINE_TERMINATOR
	//	TEXT("\tFirstControllerName = %s") LINE_TERMINATOR,
	//	ControllerIndex, NewUserPlatformId, OldUserPlatformId, *USoPlatformHelper::GetFirstGamepadName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::CanHaveSoulKeeper() const
{
	if (IsEpisode())
	{
		FSoEpisodeMapParams EpisodeData;
		if (USoLevelHelper::GetEpisodeData(USoLevelHelper::GetEpisodeNameFromObject(this), EpisodeData))
		{
			return EpisodeData.bCanHaveSoulKeeper;
		}

		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::CanContinueGame() const
{
	return IsGameStarted() || FSoWorldState::Get().DoesSaveExist();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::SetGameStarted(bool bStarted)
{
	// Can't start in menu level
	if (bStarted && IsMenu())
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("Could not SetGameStarted = %d. We are in the menu level. Ignoring."), bStarted);
		return;
	}
	if (bGameStarted == bStarted)
		return;

	bGameStarted = bStarted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OpenMenuInstant()
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		if (USoUIMenuMain* MainMenu = Character->GetUIMainMenu())
			MainMenu->InstantOpen(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::PauseGame(bool bOpenMainMenu, bool bForce)
{
	// If we are forcing the pause game then we do not care if the game is paused or not
	//if (!bForce && !bGameStarted)
	//	return false;
	if (!bForce && USoPlatformHelper::IsGamePaused(this))
		return false;

	return SetGamePaused(bOpenMainMenu, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::ResumeGame(bool bCloseMainMenu, bool bForce)
{
	if (!bForce && !USoPlatformHelper::IsGamePaused(this))
		return false;

	// Reset to settings values
	USoGameSettings::Get().ApplyGameSettings(true);
	return SetGamePaused(bCloseMainMenu, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::SetGamePaused(bool bToggleMenu, bool bPause)
{
	// Only if different
	if (USoPlatformHelper::IsGamePaused(this) != bPause)
	{
		if (!USoPlatformHelper::SetGamePaused(this, bPause))
		{
			UE_LOG(LogSoGameInstance, Warning, TEXT("Could not SetGamePaused = %s"), bPause ? TEXT("true") : TEXT("false"));
			return false;
		}
	}

	// Bring up the menu
	if (bToggleMenu)
	{
		if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		{
			if (USoUIMenuMain* MainMenu = Character->GetUIMainMenu())
			{
				MainMenu->InstantOpen(bPause);
				return true;
			}
		}
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ResetEpisode()
{
	// TODO: maybe first valid episode? wth do we need this?
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoGameInstance::GameInstanceStateToString(ESoGameInstanceState InstanceState)
{
	FString EnumValue;
	if (USoStringHelper::ConvertEnumToString<ESoGameInstanceState>(TEXT("ESoGameInstanceState"), InstanceState, false, EnumValue))
		return EnumValue;

	return EnumValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::SetGameViewportClient(USoGameViewportClient* GameViewportClient)
{
	Viewport = GameViewportClient;
	if (Viewport == nullptr)
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("Could not set GameViewportClient"));
	}
	else
	{
		Viewport->OnFadingChanged.AddUObject(this, &ThisClass::OnViewportClientFadingChanged);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::ShowLoadingScreen()
{
	UE_LOG(LogSoGameInstance, Verbose, TEXT("ShowLoadingScreen"));
	verify(LoadingScreenInstance.IsValid());

	const bool bWasHidden = !LoadingScreenInstance->IsLoadingScreenVisible();

	// first time it was visible, play music
	if (bWasHidden)
	{
		// TODO: fix the fadein by waiting for it?
		//if (Viewport)
			//Viewport->FadeIn(0.5f);

		StartLoadingScreenMusic();

		if (const ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			SoCharacter->OnMainLoadingScreenShow.Broadcast();
	}

	if (!LoadingScreenInstance->ShowLoadingScreen())
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("ShowLoadingScreen: Can't show loading screen for some reason"));
		return false;
	}

	if (bWasHidden)
	{
		// update loading screen tip
		FSoLevelEnterParams EnterParams;
		if (USoLevelHelper::GetMapEnterParams(LastLoadedMapName, EnterParams))
		{
			TArray<FText>& LoadingTexts = EnterParams.RandomLoadingScreenTexts.Num() > 0 ? EnterParams.RandomLoadingScreenTexts : USoGameSingleton::Get().GeneralLoadingScreenTexts;
			FText Tip;
			if (LoadingTexts.Num() > 0)
			{
				Tip = LoadingTexts[FMath::RandHelper(LoadingTexts.Num())];
			}
			else
			{
				Tip = FROM_STRING_TABLE_UI("input_left");
			}

			LoadingScreenInstance->UpdateLoadingScreenTip(Tip);
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::HideLoadingScreen()
{
	UE_LOG(LogSoGameInstance, Verbose, TEXT("HideLoadingScreen"));
	verify(LoadingScreenInstance.IsValid());

	LoadingScreenInstance->HideLoadingScreen();
	StopLoadingScreenMusic();
	FocusGameUI();

	if (Viewport)
		Viewport->FadeOut(0.5f);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::IsLoadingScreenVisible() const
{
	return LoadingScreenInstance.IsValid() && LoadingScreenInstance->IsLoadingScreenVisible();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::SafeUpdateLoadingScreen(float DeltaSeconds, bool bResumeGame)
{
	// Open chapter loading screen if not already
	if (!IsLoadingScreenVisible())
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("SafeUpdateLoadingScreen: Loading screen is not open. Reopening it."));
		ShowLoadingScreen();
	}

	// Finished loading
	if (!TickLoadingScreen(DeltaSeconds))
	{
		UE_LOG(LogSoGameInstance, Verbose, TEXT("SafeUpdateLoadingScreen: Finished loading. Hiding loading screen."));
		HideLoadingScreen();

		// Only resume when the game was started
		if (bResumeGame)
			ResumeGame(true, true);

		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::TickLoadingScreen(float DeltaSeconds)
{
	static float DeltaSecondsCounter = 0;
	DeltaSecondsCounter += DeltaSeconds;

	/** TODO: explain if works */
	const FSoSplinePoint PlayerSplineLoc = USoStaticHelper::GetPlayerSplineLocation(this);
	const bool bCheckedClaimedLevelsOnly = PlayerSplineLoc.GetSpline() != nullptr;
	bool bAllLoaded;
	const bool bCanClaimSplineLocation = FSoLevelManager::Get().ClaimSplineLocation(PlayerSplineLoc, bAllLoaded);

	// No level is still loading OR the claimed spline has all the levels we need loaded
	const bool bGameDone = (bCheckedClaimedLevelsOnly && bCanClaimSplineLocation) || !USoLevelHelper::IsAnyLevelLoading(this);

	// Wait for input
	if (bGameDone)
	{
		const FName MapName = USoLevelHelper::GetMapNameFromObject(this);
		const FString MapNameString = MapName.ToString();
		const bool bHasEnterSequence = CanPlayEnterSequenceForMap(MapName);
		if (bHasEnterSequence != LoadingScreenInstance->IsWaitForAnyKeyInput())
		{
			// Changed
			LoadingScreenInstance->SetWaitForAnyKeyInput(bHasEnterSequence);

			// Set Data
			if (bHasEnterSequence)
			{
				TSoftObjectPtr<UTexture2D> Image;
				FText Title;
				FText Description;
				USoLevelHelper::GetMapImageTitleDescription(
					MapName,
					Image, Title, Description
				);
				LoadingScreenInstance->SetWaitForAnyKeyInputData(Image, Title, Description);
			}
		}
	}

	if (bGameDone && LoadingScreenInstance->CanHideLoadingScreen() && State != ESoGameInstanceState::LoadingMenu)
	{
		// Reset time since last input as we are loading
		if (DeltaSecondsCounter > 1.f)
		{
			ResetTimeSinceLastInput();
			DeltaSecondsCounter = 0.f;
		}

		SetupEnterSequence();

		return false;
	}

	ResetTimeSinceLastInput();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::DidEnterSequencePlayForMap(FName MapName) const
{
	const ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!SoCharacter)
		return false;

	return IDlgDialogueParticipant::Execute_GetBoolValue(SoCharacter, MapName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::SetMapAlreadyEnteredOnce(FName MapName, bool bValue)
{
	ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!SoCharacter)
		return;

	IDlgDialogueParticipant::Execute_ModifyBoolValue(SoCharacter, MapName, bValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::CanPlayEnterSequenceForMap(FName MapName) const
{
	return IsMapEnteredTheFirstTime(MapName) && HasEnterSequenceData(MapName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::IsMapEnteredTheFirstTime(FName MapName) const
{
	return
		!DidEnterSequencePlayForMap(MapName) &&
		FSoWorldState::Get().IsCheckpointLocationDefault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::GetEnterSequenceData(FName MapName, ALevelSequenceActor*& OutLevelSequenceActor, AActor*& OutLevelEnterCamera, FSoLevelEnterParams& OutEnterParams)
{
	if (!USoLevelHelper::GetMapEnterParams(MapName, OutEnterParams))
		return false;

	// Get the data from the chapter params
	OutLevelSequenceActor = OutEnterParams.Sequence.Get();
	OutLevelEnterCamera = OutEnterParams.Camera.Get();

	return IsValid(OutLevelSequenceActor) && IsValid(OutLevelEnterCamera);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::HasEnterSequenceData(FName MapName) const
{
	FSoLevelEnterParams Params;
	if (!USoLevelHelper::GetMapEnterParams(MapName, Params))
		return false;

	return Params.Sequence.IsValid() && Params.Camera.IsValid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::SetupEnterSequence()
{
	bShouldStartLevelEnterSequence = false;
	ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!SoCharacter)
		return;

	const FName MapName = USoLevelHelper::GetMapNameFromObject(this);

	// Does not have any start sequence
	if (!IsMapEnteredTheFirstTime(MapName))
		return;

	// Mark start sequence as played
	SetMapAlreadyEnteredOnce(MapName, true);

	// Get the data for the enter sequence
	FSoLevelEnterParams LevelEnterParams;
	ALevelSequenceActor* LevelSequenceActor = nullptr;
	AActor* LevelEnterCamera = nullptr;
	const bool bHasEnterData = GetEnterSequenceData(MapName, LevelSequenceActor, LevelEnterCamera, LevelEnterParams);

	// Init stuff so level sequence happens after teleport
	if (bHasEnterData)
	{
		bShouldStartLevelEnterSequence = true;
		SoCharacter->SetCameraFadeOutBlocked(true);

		// TODO this is a map name actually
		OnMapStart.Broadcast(MapName, LevelEnterParams.DisplayTextOnBlackFadeDuringFirstMapStart);
		SoCharacter->ChangeUIVisibility.Broadcast(false);
		SoCharacter->FadeFromBlackInstant.Broadcast();

		EnterSequenceTimer = 4.0f;
		SoCharacter->GetPlayerProgress()->StopGatheringPlayerData();
	}
	else
	{
		SoCharacter->ChangeUIVisibility.Broadcast(false);
		OnMapStart.Broadcast(MapName, LevelEnterParams.DisplayTextOnBlackFadeDuringFirstMapStart);
		SoCharacter->FadeFromBlackInstant.Broadcast();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StartEnterSequence()
{
	ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!SoCharacter || !bShouldStartLevelEnterSequence)
	{
		return;
	}

	// Get the data for the enter sequence
	const FName MapName = USoLevelHelper::GetMapNameFromObject(this);
	FSoLevelEnterParams LevelEnterParams;
	ALevelSequenceActor* LevelSequenceActor = nullptr;
	AActor* LevelEnterCamera = nullptr;
	const bool bHasEnterData = GetEnterSequenceData(MapName, LevelSequenceActor, LevelEnterCamera, LevelEnterParams);

	// Init stuff so level sequence happens after teleport
	if (bHasEnterData && LevelSequenceActor->SequencePlayer)
	{
		LevelSequenceActor->SequencePlayer->JumpToSeconds(0.0f);
		LevelSequenceActor->SequencePlayer->Play();
		SoCharacter->FadeFromBlackInstant.Broadcast();
		ASoMarker* TelTargetAfterSequence = LevelEnterParams.TeleportAfterCutscene.Get();
		SoCharacter->SoASoAWaitForActivitySwitch->Enter(
			LevelSequenceActor,
			LevelEnterCamera,
			LevelEnterParams.Animation,
			TelTargetAfterSequence,
			TelTargetAfterSequence == nullptr,
			true
		);

		SoCharacter->GetPlayerProgress()->StopGatheringPlayerData();

		if (LevelEnterParams.HideCharacterTime > 0.0f)
		{
			SoCharacter->HideCharacterBP(true);
			GetWorld()->GetTimerManager().SetTimer(
				SoCharacter->ShowCharacterTimer,
				SoCharacter,
				&ASoCharacter::ShowCharacter,
				LevelEnterParams.HideCharacterTime
			);
		}
	}

	SoCharacter->SetCameraFadeOutBlocked(false);
	bShouldStartLevelEnterSequence = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StartEnterSequenceFromSameLevel()
{
	// SoOwner->GetSoMovement()->GetSplineLocation().GetSpline() != nullptr
	if (!IsPlaying())
		return;

	ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!SoCharacter)
		return;

	const FName MapName = USoLevelHelper::GetMapNameFromObject(this);
	if (!CanPlayEnterSequenceForMap(MapName))
		return;

	FSoLevelEnterParams LevelEnterParams;
	USoLevelHelper::GetMapEnterParams(MapName, LevelEnterParams);

	ALevelSequenceActor* LevelSequenceActor = nullptr;
	AActor* LevelEnterCamera = nullptr;

	if (LevelEnterParams.Sequence.IsValid())
		LevelSequenceActor = LevelEnterParams.Sequence.Get();

	if (LevelEnterParams.Camera.IsValid())
		LevelEnterCamera = LevelEnterParams.Camera.Get();

	// init stuff so level sequence happens after teleport
	if (LevelSequenceActor != nullptr && LevelEnterCamera != nullptr)
	{
		SetMapAlreadyEnteredOnce(MapName, true);
		SoCharacter->SoATeleport->SetSequence(
			LevelSequenceActor,
			LevelEnterCamera,
			LevelEnterParams.Animation,
			LevelEnterParams.HideCharacterTime,
			LevelEnterParams.TeleportAfterCutscene.Get()
		);

		SoCharacter->GetPlayerProgress()->StopGatheringPlayerData();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnViewportClientFadingChanged(bool bIsFading)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::ResetTimeSinceLastInput()
{
	if (ASoPlayerController* Controller = ASoPlayerController::GetInstance(this))
		Controller->ResetTimeSinceLastInput();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnGameViewportFocusLost()
{
	if (USoPlatformHelper::IsGameInForeground(this))
	{
		UE_LOG(LogSoGameInstance, Verbose, TEXT("OnGameViewportFocusLost: Ignoring because game window is NOT in background"));
		return;
	}

	UE_LOG(LogSoGameInstance, Log, TEXT("Focus LOST"));
#if !WARRIORB_WITH_EDITOR
	if (!IsLoadingScreenVisible() && USoGameSettings::Get().CanPauseGameWhenUnfocused())
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("Pausing game because window is in background"));
		PauseGame(true, true);
	}
#endif

	// Try to mute when in background
	// TODO: maybe use FApp::GetUnfocusedVolumeMultiplier/SetUnfocusedVolumeMultiplier
	if (!USoAudioManager::IsAudioMuted() && !IsVideoLooping())
	{
		if (USoGameSettings::Get().IsAudioMutedWhenUnfocused())
		{
			bAudioMutedBecauseOfLostFocus = true;
			USoAudioManager::MuteAudio();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnGameViewportFocusReceived()
{
	if (USoPlatformHelper::IsGameInBackground(this))
	{
		UE_LOG(LogSoGameInstance, Verbose, TEXT("OnGameViewportFocusReceived: Ignoring because game windows is NOT in foreground"));
		return;
	}

	UE_LOG(LogSoGameInstance, Log, TEXT("Focus RECEIVED"));
	// Sanity check
	// Sometimes the game is paused but the main menu is closed???
	// BUG: if I press continue then I alt tab quickly, the game stays paused but the main menu is not on screen but visibility and bOpen are true
	//if (USoPlatformHelper::IsGamePaused(this) && !IsMainMenuOpened())
		//OpenMainMenu();

	if (bAudioMutedBecauseOfLostFocus)
	{
		bAudioMutedBecauseOfLostFocus = false;
		USoAudioManager::UnMuteAudio();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameInstance::IsMainMenuOpened() const
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		return Character->IsMainMenuOpened();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OpenMainMenu() const
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		if (USoUIMenuMain* MainMenu = Character->GetUIMainMenu())
			MainMenu->InstantOpen(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::FocusGameUI()
{
	//UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);

	// Don't switch the window on game shutdown
	//if (GameEngine && !GIsRequestingExit)
	//{
	//	GameEngine->SwitchGameWindowToUseGameViewport();
	//}

	//FSlateApplication::Get().SetAllUserFocusToGameViewport();
	//if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	//{
	//	if (USoUISystem* UI = Character->GetUISystem())
	//	{
	//		FSlateApplication::Get().SetAllUserFocus(UI->TakeWidget());
	//	}
	//}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StartVideoLoopPlayback()
{
#if WARRIORB_WITH_VIDEO_DEMO
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!Character)
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("StartVideoLoopPlayback: ASoCharacter is nullptr. Aborting"));
		return;
	}

	USoUIVideoPlayer* VideoPlayer = Character->GetUIVideoPlayer();
	if (!VideoPlayer)
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("StartVideoLoopPlayback: VideoPlayer is nullptr. Aborting"));
		return;
	}

	PreviousState = State;
	State = ESoGameInstanceState::PlayingVideoLoop;
	PauseGame(false, true);
	if (!IsAudioMuted())
	{
		bAudioMutedBecauseOfVideoPlayback = true;
		MuteAudio();
	}

	VideoPlayer->SetVideoTextureSource(USoGameSingleton::Get().VideoMediaTexturePtr.Get());
	VideoPlayer->SetVideoMediaPlayer(USoGameSingleton::Get().VideoMediaPlayerPtr.Get());
	VideoPlayer->SetVideoMediaSource(USoGameSingleton::Get().DemoMediaSourcePtr.Get());
	VideoPlayer->StartVideo();
#endif // WARRIORB_WITH_VIDEO_DEMO
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StopVideoLoopPlayback()
{
#if WARRIORB_WITH_VIDEO_DEMO
	if (!IsVideoLooping())
		return;

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (!Character)
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("StopVideoLoopPlayback: ASoCharacter is nullptr. Aborting"));
		return;
	}

	USoUIVideoPlayer* VideoPlayer = Character->GetUIVideoPlayer();
	if (!VideoPlayer)
	{
		UE_LOG(LogSoGameInstance, Error, TEXT("StopVideoLoopPlayback: VideoPlayer is nullptr. Aborting"));
		return;
	}

	State = PreviousState;
	VideoPlayer->StopVideo();

	ResumeGame(false, true);
	if (bAudioMutedBecauseOfVideoPlayback)
	{
		bAudioMutedBecauseOfVideoPlayback = false;
		UnMuteAudio();
	}
#endif // WARRIORB_WITH_VIDEO_DEMO
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StartLoadingScreenMusic()
{
	InitializeAudio();
	if (AudioManager)
		AudioManager->StartLoadingScreenMode(USoGameSingleton::Get().LoadingMusicPtr.Get());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::StopLoadingScreenMusic()
{
	InitializeAudio();
	if (AudioManager)
	{
		AudioManager->StopLoadingScreenMode();
		if (ASoCharacter* Char = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			Char->UpdateMusic(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnMenuStateChange(bool bOpened)
{
	USoAudioManager::SetPausedMenuBuses(bOpened);
	OnMenuStateChanged.Broadcast(bOpened);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::OnDialogueStateChange(bool bOpened)
{
	USoGameSingleton::Get().VCA_AmbientDialogue.Target = bOpened ? 0.0f : 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::VerifyStateMatchesCurrentLevelType()
{
	switch (State)
	{
	case ESoGameInstanceState::Default:
	case ESoGameInstanceState::LoadingChapter:
	case ESoGameInstanceState::LoadingChapterCompleted:
		if (!USoLevelHelper::IsInChapterLevel(this))
		{
			UE_LOG(LogSoGameInstance, Error, TEXT("VerifyStateMatchesCurrentLevelType State is Chapter but Current Level is = %s"), *USoLevelHelper::GetMapNameStringFromObject(this));
		}
		break;

	case ESoGameInstanceState::PlayingEpisode:
	case ESoGameInstanceState::LoadingEpisode:
	case ESoGameInstanceState::LoadingEpisodeCompleted:
		if (!USoLevelHelper::IsInEpisodeLevel(this))
		{
			UE_LOG(LogSoGameInstance, Error, TEXT("VerifyStateMatchesCurrentLevelType State is Episode but Current Level is = %s"), *USoLevelHelper::GetMapNameStringFromObject(this));
		}
		break;

	case ESoGameInstanceState::InMainMenu:
	case ESoGameInstanceState::LoadingMenu:
	case ESoGameInstanceState::LoadingMenuCompleted:
		if (!USoLevelHelper::IsInMenuLevel(this))
		{
			UE_LOG(LogSoGameInstance, Error, TEXT("VerifyStateMatchesCurrentLevelType State is Menu but Current Level is = %s"), *USoLevelHelper::GetMapNameStringFromObject(this));
		}
		break;

	default:
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameInstance::SetStateToCurrentLevelType()
{
	if (IsLoading())
	{
		UE_LOG(LogSoGameInstance, Warning, TEXT("SetStateToCurrentLevelType can't set state because we are loading."));
		return;
	}

	// We started in editor or by some magic the game started this way
	if (USoLevelHelper::IsInEpisodeLevel(this))
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("SetStateToCurrentLevelType detected we are in a Episode level"));
		State = ESoGameInstanceState::PlayingEpisode;
	}
	else if (USoLevelHelper::IsInChapterLevel(this))
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("SetStateToCurrentLevelType detected we are in a Chapter level"));
		State = ESoGameInstanceState::Default;
	}
	else if (USoLevelHelper::IsInMenuLevel(this))
	{
		UE_LOG(LogSoGameInstance, Log, TEXT("SetStateToCurrentLevelType detected we are in a Menu level"));
		State = ESoGameInstanceState::InMainMenu;
	}
	else
	{
		if (USoPlatformHelper::IsGamePaused(this)) // && IsMainMenuOpened())
		{
			UE_LOG(LogSoGameInstance, Log, TEXT("SetStateToCurrentLevelType Game is Paused and Main Menu is open. Assuming main menu level!"));
			State = ESoGameInstanceState::InMainMenu;
		}
		else
		{
			UE_LOG(LogSoGameInstance, Warning, TEXT("SetStateToCurrentLevelType could not detect any level, what could it be?"));
		}
	}
}
