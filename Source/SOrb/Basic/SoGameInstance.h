// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Engine/GameInstance.h"

#if WARRIORB_WITH_ONLINE
#include "GameFramework/OnlineReplStructs.h"
#include "OnlineSubsystemTypes.h"
#endif

// DlgConfigParser
#include "IO/DlgConfigParser.h"

// FSoCrashHandler
#include "SoCrashHandler.h"

// Enums/Structs
#include "Enemy/SoEnemyDataTypes.h"
#include "Levels/SoLevelTypes.h"
#include "Basic/SoDifficulty.h"
#include "Effects/SoEffectBase.h"

// Loading screen instance
#include "INYLoadingScreenInstance.h"

#include "SoGameInstance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoSaveNotify);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSoMapStartEvent, FName, MapName, FText, DisplayText);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoMenuStateChange, bool, bOpend);


class UFMODEvent;
class ASoCharacter;
class USoUILoadingScreen;
class UFMODVCA;
class USoAudioManager;
class USoAchievementManager;
class USoGameViewportClient;
class AStaticMeshActor;
class USoEAction;
class UFMODBus;
class USoEnemyVoiceManager;
class ASoProjectile;
class USoAnalytics;

// Array wrappers for pools:

USTRUCT(BlueprintType)
struct FSoEffectInstanceArray
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	TArray<USoEffectBase*> Instances;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoProjectileArray
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	TArray<ASoProjectile*> Instances;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoHideRequests
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
	AStaticMeshActor* Actor = nullptr;

	UPROPERTY()
	int32 RequestCount = 0;
};


UENUM()
enum class ESoGameInstanceState : uint8
{
	// Chapter
	// Default, usual game play in a chapter
	Default = 0,

	// Demo
	PlayingVideoLoop,

	LoadingChapter,
	LoadingChapterCompleted,

	// Episode
	PlayingEpisode,
	LoadingEpisode,
	LoadingEpisodeCompleted,

	// Menu level
	InMainMenu,
	LoadingMenu,
	LoadingMenuCompleted
};



UCLASS(Blueprintable, BlueprintType, HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	USoGameInstance(const FObjectInitializer& ObjectInitializer);
	~USoGameInstance();

	// UGameInstance Implementation
	void Init() override;
	void Shutdown() override;
	void StartGameInstance() override;
	bool HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld) override;
	void LoadComplete(float LoadTime, const FString& MapName) override;

	//
	// Own Methods
	//

	UFUNCTION(BlueprintPure, DisplayName = "Get So Game Instance", meta = (WorldContext = "WorldContextObject"))
	static USoGameInstance* GetInstance(const UObject* WorldContextObject);
	static USoGameInstance& Get(const UObject* WorldContextObject)
	{
		check(IsValid(WorldContextObject));
		auto* Instance = GetInstance(WorldContextObject);
		check(IsValid(Instance));
		return *Instance;
	}

	bool AllowCheats() const;
	void SetAllowCheats(bool bAllow);

	// Custom Start play, is only called in game mode
	void BeforeGameModeStartPlay();
	void AfterGameModeStartPlay();
	void BeforeGameModeEndPlay();

	// Custom tick for game instance, only ticks in game mode (when a world is set)
	void Tick(float DeltaSeconds);

	UFUNCTION(BlueprintCallable)
	void StaticMeshHideRequest(AStaticMeshActor* StaticMeshActor, bool bHide);

	//
	// Online
	//
#if WARRIORB_WITH_ONLINE
	void HandleLoginUIClosed(TSharedPtr<const class FUniqueNetId> UniqueId, const int ControllerIndex, const struct FOnlineError& Error);
	void HandleUserLoginChanged(int32 GameUserIndex, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UserId);
	void HandleControllerPairingChanged(int GameUserIndex, const FUniqueNetId& PreviousUser, const FUniqueNetId& NewUser);
#endif
	void HandleApplicationWillDeactivate();

	//
	// Load/Save the chapter/episode
	//
	bool CanSaveGame(FString& OutMessage) const;

	bool SaveGameForCurrentChapter(bool bForce = false);
	bool SaveGameForCurrentEpisode(bool bForce = false);
	bool SaveGameForChapter(int32 SlotIndex, bool bForce = false);
	bool SaveGameForEpisode(FName EpisodeName, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = ">Level")
	void SaveGameForCurrentState(bool bForce = false);

	// Only used with the demo
	void UseSavesForDemo();
	bool LoadGameForDemo(bool bNewGame);

	// NOTE: all of these save the game if it playing a game
	bool LoadGameForCurrentChapter(bool bNewGame);
	bool LoadGameForCurrentEpisode(bool bNewGame);
	bool LoadGameForChapter(int32 SlotIndex, bool bNewGame, ESoDifficulty DifficultyForNewGame = ESoDifficulty::Intended);
	bool LoadGameForEpisode(FName EpisodeName, bool bNewGame);

	// Loads game from current state
	void LoadGameFromCurrentState(bool bNewGame);

	//
	// Teleport
	//

	/**
	 * Move/Teleport character to the new chapter map
	 * NOTE: by moving to the new level everything in the actors is reset
	 * And a new World is set. Only USoGameInstance has the variables still set
	 */
	UFUNCTION(BlueprintCallable, Category = ">Level")
	bool TeleportToChapter(FName ChapterName, FName CheckpointName, bool bSave);

	// Move/Teleport character to the new episode. If it is already the current map it just ignore it and teleports to the checkpoint
	UFUNCTION(BlueprintCallable, Category = ">Level")
	bool TeleportToEpisode(FName EpisodeName, bool bSave);

	// Teleport to main menu level
	UFUNCTION(BlueprintCallable, Category = ">Level")
	bool TeleportToMainMenu(bool bSave, bool bForceReloadIfAlreadyIn = false);

	//
	// Analytics
	//

	/** Initializes/Shutdown the Analytics system */
	void InitializeAnalytics();
	void ShutdownAnalytics();

	// Gets the Analytics singleton
	USoAnalytics* GetAnalytics();

	//
	// Achievements
	//

	/** Initializes/Shutdown the Achievement system */
	bool InitializeAchievements();
	void ShutdownAchievements();

	USoAchievementManager* GetAchievementManager() const;

	//
	// Audio
	//

	USoAudioManager* GetAudioManager();
	USoEnemyVoiceManager* GetEnemyVoiceManager();

	/** Initializes/Shutdown the Audio system */
	void InitializeAudio();
	void ShutdownAudio();

	//
	// Other
	//

	/** has to be called in BeginPlay() cause PIE mode */
	void ClearConfigData();

	/**
	 * Called to get an action set based on name - a template is stored in ActionSets map so it is read from config only once
	 * @PARAM ListName: list identifier, also used in filename (file is located in "So/AiConfig/ActionLists/" + ListName.ToString() + ".sc")
	 * @PARAM Outer: should be the owner of the array - used to as outer when the new actions are constructed
	 */
	void ReinitializeActionList(FName ListName, UObject* Outer, TArray<USoEAction*>& OutActions);

	/**
	 * Called to initialize a strike map - a template is stored in ActionSets map so it is read from config only once
	 * @PARAM MapName: list identifier, also used in filename (file is located in "So/AiConfig/StrikeMaps/" + ListName.ToString() + ".sc")
	 * @PARAM StrikeMap: (Out), array, cleared and filled based on the associated data
	 */
	void ReinitializeStrikeMap(FName MapName, TMap<FName, FSoStrike>& StrikeMap);

	USoEffectBase* ClaimEffect(TSubclassOf<USoEffectBase> Class);
	ASoProjectile* ClaimProjectile(TSubclassOf<ASoProjectile> Class);

	void ClearObjectPools();
	void ReturnEffect(USoEffectBase* Effect);
	void ReturnProjectile(ASoProjectile* Projectile);


	// Is game started at least once? This is false initially but if the player presses continue or loads any save slot it will be true.
	bool IsGameStarted() const { return bGameStarted; }
	void SetGameStarted(bool bStarted);

	void OpenMenuInstant();

	// Resumes/Pauses the game and brings up the UI menu
	UFUNCTION(BlueprintCallable, Category = ">Pause")
	bool PauseGame(bool bOpenMainMenu = false, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = ">Pause")
	bool ResumeGame(bool bCloseMainMenu = false, bool bForce = false);

	// bBeforeMenuShownOnce:
	void SetBeforeMenuShownOnce(const bool bValue) { bBeforeMenuShownOnce = bValue; }
	bool IsBeforeMenuShownOnce() const { return bBeforeMenuShownOnce; }

	// State actions
	static FString GameInstanceStateToString(ESoGameInstanceState InstanceState);
	FString GetCurrentStateAsString() const { return GameInstanceStateToString(State); }

	// Should we show the button? Depending on the current state if in episode or story act
	bool CanHaveSoulKeeper() const;

	// Can we continue the current game?
	bool CanContinueGame() const;

	// Loading
	FORCEINLINE bool IsLoading() const { return IsLoadingChapter() || IsLoadingEpisode() || IsLoadingMenu(); }
	FORCEINLINE bool IsPlaying() const { return IsPlayingChapter() || IsPlayingEpisode(); }

	UFUNCTION(BlueprintPure, Category = ">LoadingScreen")
	bool IsLoadingScreenVisible() const;

	bool ShouldStartLevelEnterSequence() const { return bShouldStartLevelEnterSequence; }

	// Episode
	FORCEINLINE bool IsEpisode() const { return IsLoadingEpisode() || IsPlayingEpisode();  }
	bool IsPlayingEpisode() const
	{
		return State == ESoGameInstanceState::PlayingEpisode;
	}
	FORCEINLINE bool IsLoadingEpisode() const
	{
		return State == ESoGameInstanceState::LoadingEpisode || State == ESoGameInstanceState::LoadingEpisodeCompleted;
	}

	// Chapter
	FORCEINLINE bool IsChapter() const { return IsLoadingChapter() || IsPlayingChapter(); }
	FORCEINLINE bool IsPlayingChapter() const { return State == ESoGameInstanceState::Default; }
	FORCEINLINE bool IsLoadingChapter() const
	{
		return State == ESoGameInstanceState::LoadingChapter || State == ESoGameInstanceState::LoadingChapterCompleted;
	}

	// Menu
	FORCEINLINE bool IsMenu() const { return IsBrowsingMenu() || IsLoadingMenu(); }
	FORCEINLINE bool IsBrowsingMenu() const { return State == ESoGameInstanceState::InMainMenu; }
	FORCEINLINE bool IsLoadingMenu() const
	{
		return State == ESoGameInstanceState::LoadingMenu || State == ESoGameInstanceState::LoadingMenuCompleted;
	}

	FORCEINLINE bool IsVideoLooping() const
	{
		return State == ESoGameInstanceState::PlayingVideoLoop;
	}

	// Called from the USoGameViewportClient
	void OnGameViewportFocusLost();
	void OnGameViewportFocusReceived();

	// Video
	void StartVideoLoopPlayback();
	void StopVideoLoopPlayback();

	// Tries to query the game settings, returns true by default.
	bool IsSavingOrLoading() const { return bIsSavingOrLoading; }

	void StartLoadingScreenMusic();
	void StopLoadingScreenMusic();

	UFUNCTION(BlueprintCallable, Category = Dlg)
	void OnDialogueStateChange(bool bOpened);

	void OnMenuStateChange(bool bOpened);

	// Tries to set the State to Match the current level type
	void SetStateToCurrentLevelType();

	void SetGameViewportClient(USoGameViewportClient* GameViewportClient);

protected:
	UFUNCTION()
	void OnPreLoadMap(const FString& MapName);

	UFUNCTION()
	void OnPostLoadMapWithWorld(UWorld* InLoadedWorld);

	bool SetGamePaused(bool bToggleMenu, bool bPause);
	FORCEINLINE void ResetState()
	{
		State = ESoGameInstanceState::Default;
		SetStateToCurrentLevelType();
	}
	void ResetEpisode();

	//
	// Loading Screen
	//

	bool ShowLoadingScreen();
	bool HideLoadingScreen();

	// return -> false means that the loading screen was closed.
	bool SafeUpdateLoadingScreen(float DeltaSeconds, bool bResumeGame);
	bool TickLoadingScreen(float DeltaSeconds);

	bool DidEnterSequencePlayForMap(FName MapName) const;
	void SetMapAlreadyEnteredOnce(FName MapName, bool bValue);
	bool CanPlayEnterSequenceForMap(FName MapName) const;
	bool IsMapEnteredTheFirstTime(FName MapName) const;
	bool GetEnterSequenceData(FName MapName, ALevelSequenceActor*& OutLevelSequenceActor, AActor*& OutLevelEnterCamera, FSoLevelEnterParams& OutEnterParams);
	bool HasEnterSequenceData(FName MapName) const;

	UFUNCTION(BlueprintCallable)
	void SetupEnterSequence();
	void StartEnterSequence();

	// Variant that only works if we are playing
	void StartEnterSequenceFromSameLevel();

	//
	// UI
	//

	UFUNCTION()
	void OnViewportClientFadingChanged(bool bIsFading);

	bool IsMainMenuOpened() const;
	void OpenMainMenu() const;
	void FocusGameUI();

	void OnLoadingCompleted();

	//
	// Other
	//

	// Helper
	void ResetTimeSinceLastInput();

	// Callback when controllers disconnected / reconnected
	void HandleOnGamepadConnectionChange(bool bConnected, FPlatformUserId UserID, int32 GamepadID);
	void HandleOnGamepadPariringChange(int32 ControllerIndex, FPlatformUserId NewUserPlatformId, FPlatformUserId OldUserPlatformId);

	/** Either gets an object from the pool or constructs one if there is none */
	template <typename ObjectType, typename CacheMapType>
	ObjectType* ClaimObject(CacheMapType& Cache, TSubclassOf<ObjectType> Class);

	/** Objects acquired by ClaimEffect should be returned to pool via this function */
	template <typename ObjectType, typename CacheMapType>
	void ReturnObject(CacheMapType& Cache, ObjectType* Object);

	// Checks if the current state matches the world
	void VerifyStateMatchesCurrentLevelType();

protected:

	//
	// AI configs
	//

	TMap<FName, FDlgConfigParser> ActionLists;

	UPROPERTY()
	TMap<FName, FSoEStrikes> StrikeMaps;


	//
	// Object pools/Objects
	//

	UPROPERTY()
	TArray<FSoHideRequests> StaticMeshHideRequests;

	UPROPERTY()
	TMap<TSubclassOf<USoEffectBase>, FSoEffectInstanceArray> EffectPool;

	UPROPERTY()
	TMap<TSubclassOf<ASoProjectile>, FSoProjectileArray> ProjectilePool;


	//
	// Audio
	//

	UPROPERTY()
	USoAudioManager* AudioManager = nullptr;

	UPROPERTY()
	USoEnemyVoiceManager* EnemyVoiceManager = nullptr;

	// Volumes
	float VolumeMaster = 1.0f;
	float VolumeSFX = 1.0f;
	float VolumeMusic = 1.0f;

	UPROPERTY()
	bool bAudioMutedBecauseOfLostFocus = false;

	UPROPERTY()
	bool bAudioMutedBecauseOfVideoPlayback = false;


	//
	// UI
	//

	USoGameViewportClient* Viewport = nullptr;

	TSharedPtr<INYLoadingScreenInstance> LoadingScreenInstance = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bShouldStartLevelEnterSequence = false;

	UPROPERTY(BlueprintReadOnly)
	float EnterSequenceTimer = -1.0f;

	// For BeforeMainMenu
	UPROPERTY()
	bool bBeforeMenuShownOnce = false;

	//
	// Achievements
	//
	UPROPERTY()
	USoAchievementManager* AchievementManager = nullptr;


	//
	// Analytics
	//

	UPROPERTY()
	USoAnalytics* AnalyticsInstance = nullptr;


	//
	// Other
	//

	// Used for demo video playback, do we need it anywhere else?
	UPROPERTY()
	ESoGameInstanceState PreviousState = ESoGameInstanceState::Default;

	UPROPERTY()
	ESoGameInstanceState State = ESoGameInstanceState::Default;

	// True if the game was started, by started it means the player can move in the game:
	// Pressed continue, loaded save, started new game, started in editor.
	// Set by UI.
	UPROPERTY(BlueprintReadOnly, Category = ">State")
	bool bGameStarted = false;


	// Is a saving or loading operation in progress? Flag created so that save/load are mutually exclusive
	UPROPERTY(BlueprintReadOnly, Category = ">State")
	bool bIsSavingOrLoading = false;

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoSaveNotify OnSaveToSlot;

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoMapStartEvent OnMapStart;

	UPROPERTY(BlueprintAssignable, Category = ">Events")
	FSoMenuStateChange OnMenuStateChanged;

	// Custom crash handler, on the stack seems to be the safest
	UPROPERTY()
	FSoCrashHandler CrashHandler;

	// Are we moving between worlds?
	bool bIsTransitioningWorlds = false;
	FName LastLoadedMapName = NAME_None;

	FDelegateHandle ApplicationWillDeactivateDelegate;

	bool bWasDeactivatedDuringLoading = false;

	//
	// Cheats
	//

	// Value user configurable
	bool bAllowCheats = WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ObjectType, typename CacheMapType>
ObjectType* USoGameInstance::ClaimObject(CacheMapType& Cache, TSubclassOf<ObjectType> Class)
{
	auto* ArrayPtr = Cache.Find(Class);
	if (ArrayPtr == nullptr)
		Cache.Add(Class);
	else if (ArrayPtr->Instances.Num() != 0)
		return ArrayPtr->Instances.Pop();

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ObjectType, typename CacheMapType>
void USoGameInstance::ReturnObject(CacheMapType& Cache, ObjectType* Object)
{
	auto* ArrayPtr = Cache.Find(Object->GetClass());
	if (ArrayPtr != nullptr)
		ArrayPtr->Instances.Push(Object);
	else
		Cache.Add(Object->GetClass(), { { Object } });
}
