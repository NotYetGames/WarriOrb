// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoEnemySpawner.h"

#include "TimerManager.h"

#include "SoEnemySpawnHandler.h"
#include "SoEnemy.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameMode.h"
#include "SplineLogic/SoEditorGameInterface.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoEnemySpawner::ASoEnemySpawner(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASoEnemySpawner, SoGroupName))
	{
		if (SoGroupName != NAME_None)
			FSoEditorGameInterface::FixEnemyGroupName(SoGroupName);

		FSoEditorGameInterface::BuildEnemyGroupDataConfigFile(GetWorld());
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::OnConstruction(const FTransform& Transform)
{
	if (SpawnGroups.Num() != 0)
	{
		SoEnemyToSpawnNum = 0;

		for (const auto& SpawnGroup : SpawnGroups)
			for (const auto& Entry : SpawnGroup.Entry)
				SoEnemyToSpawnNum += Entry.Count;
	}

	Super::OnConstruction(Transform);

	for (auto* SpawnHandler : SpawnHandlers)
		if (SpawnHandler != nullptr)
			SpawnHandler->SetSoGroupName(SoGroupName);

	for (auto& SpawnGroup : SpawnGroups)
		for (auto& Entry : SpawnGroup.Entry)
			if (Entry.SpawnerOverride != nullptr)
				Entry.SpawnerOverride->SetSoGroupName(SoGroupName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	SpawnHandlers.RemoveAll([](const ASoEnemySpawnHandler* Ptr) { return Ptr == nullptr; });
	OnReload();

	ASoGameMode::Get(this).OnPostLoad.AddDynamic(this, &ASoEnemySpawner::OnReload);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateAndReset();
	ASoGameMode::Get(this).OnPostLoad.RemoveDynamic(this, &ASoEnemySpawner::OnReload);

	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::Trigger_Implementation(const FSoTriggerData& TriggerData)
{
	if (ASoGameMode::Get(this).IsEnemyGroupStillActive(SoGroupName))
		Activate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::OnSpawnedDied(ASoEnemy* Victim)
{
	SpawnedDiedCounter += 1;

	SpawnedEnemiesAlive.Remove(Victim);
	AllEnemiesAlive.Remove(Victim);
	SpawnedEnemiesDeadTemp.Add(Victim);
	if (bSpawnNextOnSpawnedDied)
		Spawn();

	OnSpawnedDiedBP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::OnNotSpawnedDied(ASoEnemy* Victim)
{
	Victim->OnDeathNotify.RemoveDynamic(this, &ASoEnemySpawner::OnNotSpawnedDied);
	NotSpawnedEnemiesWatched.Remove(Victim);
	AllEnemiesAlive.Remove(Victim);
	if (SpawnedEnemiesAlive.Num() == 0 && NotSpawnedEnemiesWatched.Num() == 0 && !ShouldSpawnMore())
		Finish();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::Spawn()
{
	if (GetWorld() == nullptr)
		return;

	if (!SpawnInternal() && SpawnedEnemiesAlive.Num() == 0 && NotSpawnedEnemiesWatched.Num() == 0)
	{
		Finish();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::Finish()
{
	bFinished = true;
	OnFinishedSpawningAll(false);

	if (Music != nullptr)
		if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
			SoCharacter->ClearMusicOverrideFromRequester(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemySpawner::ShouldSpawnMore_Implementation()
{
	return ActiveGroup < SpawnGroups.Num();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemySpawner::SpawnInternal_Implementation()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Spawn - SpawnInternal_Implementation"), STAT_Spawn_SpawnInternal_Implementation, STATGROUP_SoEnemy);

	if (ActiveGroup >= SpawnGroups.Num())
		return false;

	// 1. check group - even if it fails we still have enemies to spawn (but we have to wait in this case)
	if (MaxSpawnedCreatureNum <= SpawnedEnemiesAlive.Num())
		return true;

	bool bAnyCandidateNotUsed = false;

	int32 ToSpawnNum = 0;
	TArray<int32> Candidates;
	TArray<ASoEnemySpawnHandler*> CandidateHandlers;
	for (int32 i = 0; i < SpawnGroups[ActiveGroup].Entry.Num(); ++i)
		if (SpawnGroups[ActiveGroup].Entry[i].AlreadySpawned < SpawnGroups[ActiveGroup].Entry[i].Count)
		{
			ToSpawnNum += 1;

			ASoEnemySpawnHandler* SpawnHandlerOverride = SpawnGroups[ActiveGroup].Entry[i].SpawnerOverride;
			if (SpawnHandlerOverride != nullptr &&
				SpawnHandlerOverride->IsEnemyCompatible(SpawnGroups[ActiveGroup].Entry[i].Class) &&
				SpawnHandlerOverride->CanSpawn())
			{
				Candidates.Add(i);
				CandidateHandlers.Add(SpawnHandlerOverride);

				if (HandlersUsedInCurrentSpawnGroup.Find(SpawnHandlerOverride) == INDEX_NONE)
					bAnyCandidateNotUsed = true;
			}
			else
			{
				for (ASoEnemySpawnHandler* SpawnHandler : SpawnHandlers)
					if (SpawnHandler->IsEnemyCompatible(SpawnGroups[ActiveGroup].Entry[i].Class) && SpawnHandler->CanSpawn())
					{
						Candidates.Add(i);
						CandidateHandlers.Add(SpawnHandler);

						if (HandlersUsedInCurrentSpawnGroup.Find(SpawnHandler) == INDEX_NONE)
							bAnyCandidateNotUsed = true;
					}
			}
		}

	// no more unit in this group
	if (ToSpawnNum == 0)
	{
		if (SpawnedEnemiesAlive.Num() == 0)
		{
			// group is done, time to spawn the next group
			ActiveGroup += 1;
			HandlersUsedInCurrentSpawnGroup.Empty();
			return SpawnInternal();
		}
		return true;
	}

	// there are candidates, but no free spawner -> wait
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogSoEnemyAI, Display, TEXT("No spawnerhandler could take the job, spawn is delayed!"));
		GetWorld()->GetTimerManager().SetTimer(SpawnDelayTimer, this, &ASoEnemySpawner::Spawn, 1.0f);
		return true;
	}

	int32 SpawnIndex = FMath::RandHelper(Candidates.Num());
	check(Candidates.Num() == CandidateHandlers.Num());

	if (CandidateHandlers.Num() != 1)
	{
		if (LastUsedSpawnHandler == CandidateHandlers[SpawnIndex])
			SpawnIndex = (SpawnIndex + 1) % Candidates.Num();

		if (bAnyCandidateNotUsed)
			while (HandlersUsedInCurrentSpawnGroup.Find(CandidateHandlers[SpawnIndex]) != INDEX_NONE)
				SpawnIndex = (SpawnIndex + 1) % Candidates.Num();
	}

	if (SpawnCreature(CandidateHandlers[SpawnIndex], SpawnGroups[ActiveGroup].Entry[Candidates[SpawnIndex]].Class))
	{
		SpawnGroups[ActiveGroup].Entry[Candidates[SpawnIndex]].AlreadySpawned += 1;
		LastUsedSpawnHandler = CandidateHandlers[SpawnIndex];
		HandlersUsedInCurrentSpawnGroup.AddUnique(LastUsedSpawnHandler);
	}
	else
	{
		UE_LOG(LogSoEnemyAI, Warning, TEXT("SpawnHandler failed to spawn a creature! The whole Spawn process is terminated to avoid endless loop!"));
		return false;
	}

	if (SpawnedEnemiesAlive.Num() < MaxSpawnedCreatureNum)
		return SpawnInternal();

	// function will be called again the next time an already spawned creature dies
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemySpawner::SpawnCreature(ASoEnemySpawnHandler* SpawnHandler, TSubclassOf<ASoEnemy> Class)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Spawn - SpawnCreature"), STAT_Spawn_SpawnCreature, STATGROUP_SoEnemy);

	ASoEnemy* SpawnedCreature = SpawnHandler->SpawnCreature(Class);
	if (SpawnedCreature != nullptr)
	{
		SpawnedCreature->SetSoSpawner(this);
		SpawnedCreature->SetSoGroupName(SoGroupName);
		SpawnedCreature->SetPlacedInLevel(false);
		SpawnedCreature->OnDeathNotify.AddDynamic(this, &ASoEnemySpawner::OnSpawnedDied);
		SpawnedEnemiesAlive.Add(SpawnedCreature);
		AllEnemiesAlive.Add(SpawnedCreature);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemySpawner::SpawnCreatureFromRandomHandler(TSubclassOf<ASoEnemy> Class)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Spawn - SpawnCreatureFromRandomHandler"), STAT_Spawn_SpawnCreatureFromRandomHandler, STATGROUP_SoEnemy);

	TArray<ASoEnemySpawnHandler*> PossibleSpawnHandlers;

	for (ASoEnemySpawnHandler* SpawnHandler : SpawnHandlers)
		if (SpawnHandler != nullptr && SpawnHandler->IsEnemyCompatible(Class) && SpawnHandler->CanSpawn())
			PossibleSpawnHandlers.Add(SpawnHandler);

	if (PossibleSpawnHandlers.Num() == 0)
		return false;

	return SpawnCreature(PossibleSpawnHandlers[FMath::RandRange(0, PossibleSpawnHandlers.Num() - 1)], Class);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::DestroySpawnedCreatures(bool bFadeOutAlive)
{
	for (auto* Enemy : SpawnedEnemiesAlive)
		if (Enemy != nullptr)
		{
			if (bFadeOutAlive)
				Enemy->FadeOutAndDestroy();
			else
				Enemy->Destroy();
		}

	for (auto* Enemy : SpawnedEnemiesDeadTemp)
		if (Enemy != nullptr)
			Enemy->Destroy();

	SpawnedEnemiesDeadTemp.Empty();
	SpawnedEnemiesAlive.Empty();
	AllEnemiesAlive.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::OnPlayerRespawn()
{
	DeactivateAndReset();
	OnPlayerRespawnBP();

	if (bAutoRestartOnPlayerDeath || bAutoActivate)
		Activate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::Activate()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Spawn - Activate"), STAT_Spawn_Activate, STATGROUP_SoEnemy);
	if (!bActivated && !bFinished)
	{
		SpawnedDiedCounter = 0;

		if (EnemyGroupDefeatedDelayOverride > KINDA_SMALL_NUMBER)
			ASoGameMode::Get(this).OverrideGroupDefeatedDelay(SoGroupName, EnemyGroupDefeatedDelayOverride);

		for (ASoEnemy* Enemy : NotSpawnedEnemiesAttached)
			if (Enemy->GetActivity() != ESoEnemyActivity::EEA_Dead)
			{
				NotSpawnedEnemiesWatched.Add(Enemy);
				Enemy->SetSoSpawner(this);
				AllEnemiesAlive.Add(Enemy);
				Enemy->OnDeathNotify.AddDynamic(this, &ASoEnemySpawner::OnNotSpawnedDied);
			}

		OnActivatedBP();

		bActivated = true;

		ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (SoCharacter != nullptr)
		{
			SoCharacter->OnPlayerRespawn.AddDynamic(this, &ASoEnemySpawner::OnPlayerRespawn);
			if (Music != nullptr)
				SoCharacter->SetMusicOverride(Music, true, this);
		}
		Spawn();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::DeactivateAndCancelKilledOnes()
{
	if (!bActivated || bFinished)
		return;

	ASoGameMode::Get(this).IncreaseEnemyCountInGroup(SoGroupName, SpawnedDiedCounter);
	DestroySpawnedCreatures(true);
	DeactivateAndReset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::DeactivateAndReset()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Spawn - DeactivateAndReset"), STAT_Spawn_DeactivateAndReset, STATGROUP_SoEnemy);

	for (ASoEnemy* Enemy : NotSpawnedEnemiesWatched)
		Enemy->OnDeathNotify.RemoveDynamic(this, &ASoEnemySpawner::OnNotSpawnedDied);
	NotSpawnedEnemiesWatched.Empty();
	AllEnemiesAlive.Empty();

	if (bActivated)
	{
		if (Music != nullptr)
			if (ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
				SoCharacter->ClearMusicOverrideFromRequester(this);

		GetWorld()->GetTimerManager().ClearTimer(SpawnDelayTimer);
		DestroySpawnedCreatures();
		ActiveGroup = 0;
		HandlersUsedInCurrentSpawnGroup.Empty();
		bActivated = false;

		for (auto& Group : SpawnGroups)
			for (int32 i = 0; i < Group.Entry.Num(); ++i)
				Group.Entry[i].AlreadySpawned = 0;

		ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (SoCharacter != nullptr)
			SoCharacter->OnPlayerRespawn.RemoveDynamic(this, &ASoEnemySpawner::OnPlayerRespawn);
		DeactivateAndResetBP();
	}
	bFinished = false;
	SpawnedDiedCounter = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoEnemySpawner::OnReload()
{
	DeactivateAndReset();

	if (ASoGameMode::Get(this).IsEnemyGroupStillActive(SoGroupName))
	{
		if (bAutoActivate)
			Activate();
	}
	else
	{
		OnFinishedSpawningAll(true);
		bFinished = false;
	}
}
