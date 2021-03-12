// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoGameMode.h"
#include "EngineUtils.h"
#include "SplineLogic/SoEditorGameInterface.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "TimerManager.h"

#include "DlgDialogueParticipant.h"
#include "DlgMemory.h"
#include "IDlgSystemModule.h"
#include "IO/DlgConfigParser.h"

#include "Online/Analytics/SoAnalyticsHelper.h"

#include "HAL/IConsoleManager.h"
#include "Basic/SoEventHandler.h"
#include "Basic/SoGameInstance.h"
#include "Settings/SoGameSettings.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerController.h"
#include "Enemy/SoEnemy.h"
#include "Enemy/SoEnemyHelper.h"
#include "Character/SoCharStates/SoAInUI.h"
#include "SplineLogic/SoSplineWalker.h"
#include "Basic/SoTimerManager.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Public/UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif // WITH_EDITOR

#include "SaveFiles/SoWorldState.h"
#include "Levels/SoLevelHelper.h"
#include "Levels/SoLevelManager.h"
#include "Online/SoOnlineHelper.h"
#include "INYLoadingScreenModule.h"
#include "Helpers/SoDateTimeHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoGameMode, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoGameMode::ASoGameMode()
{
	SoTimerManager = new FSoTimerManager{};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoGameMode::~ASoGameMode()
{
	delete SoTimerManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoGameMode* ASoGameMode::GetInstance(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
		return nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		return Cast<ASoGameMode>(World->GetAuthGameMode());

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoCharacter* ASoGameMode::GetSoCharacter()
{
	if (!IsValid(CachedSoCharacter))
	{
		CachedSoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	}
	return CachedSoCharacter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::StartPlay()
{
	UE_LOG(LogSoGameMode, Log, TEXT("StartPlay"));

	// Level unloading in the same frame
	if (auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("s.UnregisterComponentsTimeLimit")))
	{
		CVar->Set(-1.0f);
	}

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	// Cleanup
	auto& WorldState = FSoWorldState::Get();
#if WITH_EDITOR
	// Refresh, useful for Editor mode mostly
	WorldState.ResetToDefault();
#endif

	// Reset for PIE mode, New Level opened
	// has to be called before the BeginPlay() calls!
	FSoLevelManager::Get().Empty();
	FDlgMemory::GetInstance()->Empty();

	// Init cloud saves
	TSharedPtr<const FUniqueNetId> UserID = USoOnlineHelper::GetUniqueNetIDFromObject(this);
	WorldState.GetCloudSavesManager().Initiallize(UserID);

	// GameInstance
	GameInstance = USoGameInstance::GetInstance(this);
	verify(GameInstance);
	GameInstance->BeforeGameModeStartPlay();

	// Init Settings
	auto& Settings = USoGameSettings::Get();
#if WITH_EDITOR
	Settings.BeforeGameModeStartPlay();
#else
	static bool bWasMenuLoadedAlready = false;
	if (GameInstance->IsMenu() && !bWasMenuLoadedAlready)
	{
		bWasMenuLoadedAlready = true;
		Settings.BeforeGameModeStartPlay();
	}
#endif

	// Set backup of saves.
	WorldState.SetAutoBackupDeletedSaves(Settings.IsAutoBackupDeletedSavesEnabled());
	WorldState.SetAutoBackupBeforeSave(Settings.IsAutoBackupBeforeSaveEnabled());

	// Init enemy groups
	if (!GameInstance->IsMenu())
	{
		FDlgConfigParser ConfigParser("So");
		ConfigParser.InitializeParser(FSoEditorGameInterface::GetEnemyGroupDataConfigPath(GetWorld()));
		ConfigParser.ReadAllProperty(FSoEnemyGroups::StaticStruct(), &EnemyGroupInitData);
		EnemyGroupActualValues = EnemyGroupInitData;
		DestroyedEnemies.Empty();
	}

	// BEFORE BeginPlays()!!!
	if (ASoCharacter* Character = GetSoCharacter())
	{
		Character->OnPlayerRematerialized.AddDynamic(this, &ThisClass::OnPlayerRematerialize);
		Character->OnPlayerRespawn.AddDynamic(this, &ThisClass::OnPlayerRespawn);
		Character->OnPlayerSplineChanged.AddDynamic(this, &ThisClass::OnPlayerSplineChanged);
	}

	// call BeginPlay()-s on each actor
	Super::StartPlay();

	// called *after* the splines had their begin play call
	FSoEditorGameInterface::LoadCameraData(GetWorld());

	// Register plugin console commands
	if (IDlgSystemModule::IsAvailable())
	{
		IDlgSystemModule::Get().RegisterConsoleCommands(this);
	}
	else
	{
		UE_LOG(LogSoGameMode, Error, TEXT("DlgSystemModule is not Available"));
	}
	if (INYLoadingScreenModule::IsAvailable())
	{
		INYLoadingScreenModule::Get().RegisterConsoleCommands(this);
	}
	else
	{
		UE_LOG(LogSoGameMode, Error, TEXT("NYLoadingScreenModule is not Available"));
	}

	// Register game console commands
	ConsoleCommands.RegisterAllCommands(this);

	GameInstance->AfterGameModeStartPlay();

	OnPostBeginPlay.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Tick game instance
	GameInstance->Tick(DeltaSeconds);

	if (ASoCharacter* Character = GetSoCharacter())
	{
		const auto& Settings = USoGameSettings::Get();
		const float IdleTimeSecondsBeforeGamePause = Settings.GetIdleTimeSecondsBeforeGamePause();
		const bool bPauseGameOnIdle = Settings.CanPauseGameOnIdle();
		const bool bVideoDemoStartVideoOnIdle = Settings.CanVideoDemoStartVideoOnIdle();
		const float VideoDemoIdleTimeSecondsStartVideo = Settings.GetVideoDemoIdleTimeSecondsStartVideo();

		// Pause game if the user is idle
#if !WARRIORB_WITH_EDITOR
		if (GameInstance->IsGameStarted() &&
			bPauseGameOnIdle &&
			!GameInstance->IsLoading() &&
			IdleTimeSecondsBeforeGamePause > KINDA_SMALL_NUMBER &&
			Character->CanPauseOnIdle())
		{
			const float IdleTimeSeconds = Character->GetSoPlayerController()->GetTimeSecondsSinceLastInput();
			if (IdleTimeSeconds > IdleTimeSecondsBeforeGamePause)
			{
				UE_LOG(LogSoGameMode, Log, TEXT("Pausing game because it was idle for more than %f seconds"), IdleTimeSecondsBeforeGamePause);

				// should reset idle time
				GameInstance->PauseGame(true);
			}
		}
#endif // !WARRIORB_WITH_EDITOR

		// Demo idle video loop
#if WARRIORB_WITH_VIDEO_DEMO
		if (bVideoDemoStartVideoOnIdle && VideoDemoIdleTimeSecondsStartVideo > KINDA_SMALL_NUMBER && !GameInstance->IsVideoLoopingState())
		{
			const float IdleTimeSeconds = Character->GetSoPlayerController()->GetTimeSecondsSinceLastInput();
			if (IdleTimeSeconds > VideoDemoIdleTimeSecondsStartVideo)
			{
				UE_LOG(LogSoGameMode, Log, TEXT("Demo: Starting video loop because of idle for about %f seconds"), VideoDemoIdleTimeSecondsStartVideo);
				Character->StartVideoLoopPlayback();
			}
		}
#endif // WARRIORB_WITH_VIDEO_DEMO

		// Update Levels
		const FSoSplinePoint PlayerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Character);
		FSoLevelManager::Get().Update(PlayerSplineLocation, DeltaSeconds);

		const EActivity CharActivity = Character->GetActivity();
		if (bShouldEnterGroupDefeated && CharActivity != EActivity::EA_Dead &&
										 CharActivity != EActivity::EA_FallToDeath &&
										 CharActivity != EActivity::EA_HitReact &&
										 CharActivity != EActivity::EA_Teleport)
		{
			bShouldEnterGroupDefeated = false;
			Character->SoAInUI->Enter(this, GroupDestroyedUIClass, true);
		}
	}
	else
	{
		// Character not present yet, load all
		FSoLevelManager::Get().ClaimAndLoadAllLevels(GetWorld());
	}

	// Stop executing everything beyond this point if the playworld is paused
#if WITH_EDITOR
	if (GUnrealEd && GUnrealEd->PlayWorld && GUnrealEd->PlayWorld->bDebugPauseExecution)
	{
		return;
	}
#endif

	if (!IsPaused())
	{
		SoTimerManager->Tick(DeltaSeconds);

		for (const FSoFlyingEnemyPositionRules& Rule : FlyingEnemyPositionRules)
			USoEnemyHelper::ForceDistanceOnFlyingEnemies(this, Rule.FlyingEnemyClass, Rule.MinDistance, Rule.MaxCorrectionSpeed * DeltaSeconds);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogSoGameMode, Log, TEXT("EndPlay"));

	FSoWorldState::Get().GetCloudSavesManager().Shutdown();
	GameInstance->BeforeGameModeEndPlay();

	GetWorld()->GetTimerManager().ClearTimer(EnemyGroupDestroyedNotificationTimer);

	if (ASoCharacter* Character = GetSoCharacter())
	{
		Character->OnPlayerRematerialized.RemoveDynamic(this, &ThisClass::OnPlayerRematerialize);
		Character->OnPlayerRespawn.RemoveDynamic(this, &ThisClass::OnPlayerRespawn);
		Character->OnPlayerSplineChanged.RemoveDynamic(this, &ThisClass::OnPlayerSplineChanged);
	}

	CachedSoCharacter = nullptr;
	GameInstance->ClearObjectPools();

	Super::EndPlay(EndPlayReason);

	// Unregister all the console commands
	if (IDlgSystemModule::IsAvailable())
	{
		IDlgSystemModule::Get().UnregisterConsoleCommands();
	}

	ConsoleCommands.UnRegisterAllCommands();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoGameMode::AllowCheats(APlayerController* Controller)
{
	return GameInstance && GameInstance->AllowCheats();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoTimerManager& ASoGameMode::GetTimerManager() const
{
	return *SoTimerManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::PreSaveGame() const
{
	UE_LOG(LogSoGameMode, Log, TEXT("PreSaveGame"));

	// Notify subscribers that they need to save their stuff right now
	OnPreSave.Broadcast();

	// Dialogue history
	FSoWorldState::Get().SetDlgHistory(FDlgMemory::GetInstance()->GetHistoryMaps());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::PostLoadGame()
{
	UE_LOG(LogSoGameMode, Log, TEXT("PostLoadGame"));
	GetWorld()->GetTimerManager().ClearTimer(EnemyGroupDestroyedNotificationTimer);

	EnemyGroupActualValues = EnemyGroupInitData;
	DestroyedEnemies.Empty();

	// Dialogue history
	FDlgMemory::GetInstance()->SetHistoryMap(FSoWorldState::Get().GetDlgHistory());

	// Notify listeners we loaded something
	// 1. The OnPostLoad listeners
	// 2. The uobjects that implement the ISoEventHandler and were subscribed
	OnPostLoad.Broadcast();
	for (UObject* EventHandler : PostLoadHandlers)
		if (EventHandler != nullptr)
			ISoEventHandler::Execute_HandleSoPostLoad(EventHandler);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::OnEnemyDestroyed(ASoEnemy* Enemy, FName Group, FVector WorldLocation)
{
	if (!ensure(Enemy))
	{
		UE_LOG(LogSoGameMode, Warning, TEXT("ASoGameMode::OnEnemyDestroyed called with invalid enemy!"));
		return;
	}

	OnEnemyDied.Broadcast(Enemy);

	if (Group == NAME_None)
	{
		return;
	}

	if (FSoEnemyArray* ArrayPtr = EnemiesAlive.Find(Group))
	{
		if (ArrayPtr->Enemies.RemoveSwap(Enemy) != 1)
			UE_LOG(LogSoGameMode, Warning, TEXT("EnemiesAlive does not contain exactly one enemy from the destroyed one (%s / %s)"), *Enemy->GetName(), *Group.ToString());
	}

	int32* ValuePtr = EnemyGroupActualValues.EnemyGroupMap.Find(Group);
	if (ValuePtr == nullptr)
	{
		UE_LOG(LogSoGameMode, Warning, TEXT("ASoGameMode::OnEnemyDestroyed called with %s, but the map does not contain it!"), *Group.ToString());
		return;
	}

	if (!ensure(*ValuePtr > 0))
		return;


	// count destroyed enemies per class
	bool bAlreadyKilledOneLikeThis = false;
	FSoEnemyCounts* Counts = DestroyedEnemies.Find(Group);
	if (Counts == nullptr)
		Counts = &DestroyedEnemies.Add(Group, {});

	for (FSoEnemyCount& EnemyCount : Counts->Counts)
		if (EnemyCount.Class == Enemy->GetClass())
		{
			bAlreadyKilledOneLikeThis = true;
			EnemyCount.Count += 1;
			if (Enemy->IsPlacedInLevel())
				EnemyCount.PlacedInLevelCount += 1;
		}

	if (!bAlreadyKilledOneLikeThis)
	{
		Counts->Counts.Add({ Enemy->GetClass(), 1, Enemy->IsPlacedInLevel() ? 1 : 0 });
	}



	(*ValuePtr) = (*ValuePtr) - 1;
	UE_LOG(LogSoGameMode, Display, TEXT("Enemy group %s: %d left"), *Group.ToString(), *ValuePtr);
	if ((*ValuePtr) == 0)
	{
		UE_LOG(LogSoGameMode, Display, TEXT("Enemy group %s cleared"), *Group.ToString());
		IDlgDialogueParticipant::Execute_ModifyBoolValue(USoStaticHelper::GetPlayerCharacterAsActor(this), Group, true);
		USoAnalyticsHelper::RecordGameplayMilestone(this, Group, true);

		OnEnemyGroupDefeated.Broadcast(Group);

		LastDestroyedGroupName = Group;
		float Delay = 2.0f;
		if (float* DelayOverridePtr = EnemyGroupDefeatedDelayOverride.Find(LastDestroyedGroupName))
			Delay = *DelayOverridePtr;
		GetWorld()->GetTimerManager().SetTimer(
			EnemyGroupDestroyedNotificationTimer,
			this,
			&ThisClass::OnGroupDestroyed,
			USoDateTimeHelper::NormalizeTime(Delay),
			false
		);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::IncreaseEnemyCountInGroup(FName Group, int32 Num)
{
	if (Group == NAME_None)
		return;

	int32* ValuePtr = EnemyGroupActualValues.EnemyGroupMap.Find(Group);
	if (ValuePtr == nullptr)
	{
		UE_LOG(LogSoGameMode, Warning, TEXT("ASoGameMode::IncreaseEnemyCountInGroup called with %s, but the map does not contain it!"), *Group.ToString());
		return;
	}

	(*ValuePtr) = (*ValuePtr) + Num;

	// we assume here that all dynamically spawned ones are regenerated
	// placed in level ones are just frozen, so the ones already killed won't respawn
	// the spawner respawns everyone on reenter
	if (FSoEnemyCounts* EnemyCounts = DestroyedEnemies.Find(Group))
	{
		for (FSoEnemyCount& EnemyCount : EnemyCounts->Counts)
			EnemyCount.Count = EnemyCount.PlacedInLevelCount;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::RegisterEnemy(ASoEnemy* Enemy, FName Group)
{
	if (Group == NAME_None || Enemy == nullptr)
	{
		return;
	}

	FSoEnemyArray* ArrayPtr = EnemiesAlive.Find(Group);
	if (ArrayPtr == nullptr)
		ArrayPtr = &EnemiesAlive.Add(Group);

	ArrayPtr->Enemies.AddUnique(Enemy);
	Enemies.AddUnique(Enemy);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::UnregisterEnemy(ASoEnemy* Enemy, FName Group)
{
	if (Group == NAME_None || Enemy == nullptr)
	{
		return;
	}

	FSoEnemyArray* ArrayPtr = EnemiesAlive.Find(Group);
	if (ArrayPtr != nullptr)
		ArrayPtr->Enemies.RemoveSwap(Enemy);

	Enemies.RemoveSwap(Enemy);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TArray<ASoEnemy*>& ASoGameMode::GetEnemiesFromGroup(FName Group)
{
	static TArray<ASoEnemy*> EmptyArray;

	if (FSoEnemyArray* ArrayPtr = EnemiesAlive.Find(Group))
		return ArrayPtr->Enemies;

	return EmptyArray;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::OverrideGroupDefeatedDelay(FName GroupName, float OverrideTime)
{
	EnemyGroupDefeatedDelayOverride.Add(GroupName, OverrideTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::OnGroupDestroyed()
{
	if (ASoCharacter* Character = GetSoCharacter())
	{
		const EActivity CharActivity = Character->GetActivity();
		if (CharActivity != EActivity::EA_Dead &&
			CharActivity != EActivity::EA_FallToDeath &&
			CharActivity != EActivity::EA_HitReact &&
			CharActivity != EActivity::EA_Teleport)
		{
			Character->SoAInUI->Enter(this, GroupDestroyedUIClass, true);
		}
		else
		{
			bShouldEnterGroupDefeated = true;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoGameMode::IsEnemyGroupStillActive(FName Group) const
{
	if (Group == NAME_None)
		return true;

	AActor* Character = USoStaticHelper::GetPlayerCharacterAsActor(this);
	return Character == nullptr || !IDlgDialogueParticipant::Execute_GetBoolValue(Character, Group);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::SubscribeToSoPostLoad(UObject* Object)
{
	if (Object == nullptr)
		return;

	if (Object->GetClass()->ImplementsInterface(USoEventHandler::StaticClass()))
	{
		PostLoadHandlers.AddUnique(Object);
		// called immediately because the state might be changed already
		ISoEventHandler::Execute_HandleSoPostLoad(Object);
	}
	else
		UE_LOG(LogSoGameMode, Warning, TEXT("SubscribeToSoPostLoad failed: %s does not implement the USoEventHandler interface!"), *Object->GetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::UnsubscribeFromSoPostLoad(UObject* Object)
{
	PostLoadHandlers.RemoveSwap(Object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::SubscribeToPlayerRematerialize(UObject* Object)
{
	if (Object == nullptr)
		return;

	if (Object->GetClass()->ImplementsInterface(USoEventHandler::StaticClass()))
		PlayerRematerializedHandlers.AddUnique(Object);
	else
		UE_LOG(LogSoGameMode, Warning, TEXT("SubscribeToPlayerRematerialize failed: %s does not implement the USoEventHandler interface!"), *Object->GetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::UnsubscribeFromPlayerRematerialize(UObject* Object)
{
	if (Object == nullptr)
		return;

	if (bPlayerRematerializedHandlersArrayBlock)
		PlayerRematerializedUnsubscribeRequests.Add(Object);
	else
		PlayerRematerializedHandlers.RemoveSingleSwap(Object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::SubscribeToPlayerRespawn(UObject* Object)
{
	if (Object == nullptr)
		return;

	if (Object->GetClass()->ImplementsInterface(USoEventHandler::StaticClass()))
		PlayerRespawnHandlers.AddUnique(Object);
	else
		UE_LOG(LogSoGameMode, Warning, TEXT("SubscribeToPlayerRespawn failed: %s does not implement the USoEventHandler interface!"), *Object->GetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::UnsubscribeFromPlayerRespawn(UObject* Object)
{
	if (bPlayerRespawnHandlersArrayBlock)
		PlayerRespawnUnsubscribeRequests.Add(Object);
	else
		PlayerRespawnHandlers.RemoveSingleSwap(Object);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::SubscribeWhitelistedSplines(UObject* Object, const TArray<TAssetPtr<ASoSpline>>& WhitelistedSplines)
{
	if (Object == nullptr)
		return;

	verify(bSplineChangedNotificationsInProgress == false);

	if (Object->GetClass()->ImplementsInterface(USoEventHandler::StaticClass()))
	{
		FSoSplineArray& SplineArray = SplineGroupChangedHandlers.Add(Object, {});
		for (auto SplinePtr : WhitelistedSplines)
			if (ASoSpline* Spline = SplinePtr.Get())
				SplineArray.Splines.Add(Spline);

		if (ASoCharacter* Character = GetSoCharacter())
		{
			const FSoSplinePoint PlayerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Character);
			if (WhitelistedSplines.Contains(PlayerSplineLocation.GetSpline()))
				ISoEventHandler::Execute_HandleWhitelistedSplinesEntered(Object);
		}
	}
	else
		UE_LOG(LogSoGameMode, Warning, TEXT("SubscribeWhitelistedSplines failed: %s does not implement the USoEventHandler interface!"), *Object->GetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::UnsubscribeWhitelistedSplines(UObject* Object)
{
	verify(bSplineChangedNotificationsInProgress == false);
	SplineGroupChangedHandlers.Remove(Object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::DisplayDamageText(const FVector& HitLocation, int32 DmgAmount, bool bPhysical, bool bOnPlayer, bool bCritical)
{
	if (USoGameSettings::Get().IsDisplayDamageTextsEnabled())
		OnDisplayDamageText.Broadcast(HitLocation, DmgAmount, bPhysical, bOnPlayer, bCritical);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::DisplayText(const FVector& Location, ESoDisplayText DisplayText)
{
	if (USoGameSettings::Get().IsDisplayDamageTextsEnabled())
		OnDisplayText.Broadcast(Location, DisplayText);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FSoEnemyCount>& ASoGameMode::GetDestroyedEnemyCount(FName Group)
{
	if (FSoEnemyCounts* Count = DestroyedEnemies.Find(Group))
	{
		return Count->Counts;
	}

	static TArray<FSoEnemyCount> Empty;
	return Empty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 ASoGameMode::CalculateDestroyedEnemyCount(FName Group)
{
	int32 Count = 0;
	if (FSoEnemyCounts* EnemyCounts = DestroyedEnemies.Find(Group))
	{
		for (FSoEnemyCount& EnemyCount : EnemyCounts->Counts)
			Count += EnemyCount.Count;
	}

	return Count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::OnPlayerRematerialize()
{
	bPlayerRematerializedHandlersArrayBlock = true;

	for (UObject* EventHandler : PlayerRematerializedHandlers)
		if (EventHandler != nullptr)
			ISoEventHandler::Execute_HandlePlayerRematerialize(EventHandler);

	for (UObject* EventHandler : PlayerRematerializedUnsubscribeRequests)
		PlayerRematerializedHandlers.RemoveSingleSwap(EventHandler);
	PlayerRematerializedUnsubscribeRequests.Empty();

	bPlayerRematerializedHandlersArrayBlock = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::OnPlayerRespawn()
{
	EnemyGroupActualValues = EnemyGroupInitData;
	DestroyedEnemies.Empty();

	bPlayerRespawnHandlersArrayBlock = true;

	for (UObject* EventHandler : PlayerRespawnHandlers)
		if (EventHandler != nullptr)
			ISoEventHandler::Execute_HandlePlayerRespawn(EventHandler);

	for (UObject* EventHandler : PlayerRespawnUnsubscribeRequests)
		PlayerRespawnHandlers.RemoveSingleSwap(EventHandler);
	PlayerRespawnUnsubscribeRequests.Empty();

	bPlayerRespawnHandlersArrayBlock = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoGameMode::OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline)
{
	bSplineChangedNotificationsInProgress = true;
	for (auto& Pair : SplineGroupChangedHandlers)
	{
		const bool bWasIn = Pair.Value.Splines.Contains(OldSpline);
		const bool bIsIn = Pair.Value.Splines.Contains(NewSpline);
		if (bWasIn != bIsIn)
		{
			if (bWasIn)
				ISoEventHandler::Execute_HandleWhitelistedSplinesLeft(Pair.Key);
			else
				ISoEventHandler::Execute_HandleWhitelistedSplinesEntered(Pair.Key);
		}
	}
	bSplineChangedNotificationsInProgress = false;
}
