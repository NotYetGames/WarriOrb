// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SplineLogic/SoPlayerSpline.h"
#include "Levels/SoLevelManager.h"
#include "SaveFiles/Stats/SoPlayerProgressStats.h"
#include "SaveFiles/SoWorldStateEpisodeTypes.h"

#include "SoPlayerProgress.generated.h"

struct FSoPlayerProgressStats;
class USoItemTemplateRuneStone;
class USoGameInstance;
class USoOnlineRichPresence;
class ASoCharacter;
class USoItemTemplate;

// Actor component that follows the player and updates the current stats
// NOTE: that USoAnalyticsComponent collects additional data that is sent to the GameAnalytics servers
// This class just updates the stats that are also useful for the running of the game or displayed/used inside the game
// These are stored inside the metadata of the save
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SORB_API USoPlayerProgress : public UActorComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	USoPlayerProgress();

	// UActorComponent Interface

	void InitializeComponent() override;
	void UninitializeComponent() override;

	// Called when the game starts
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Own methods

	UFUNCTION(BlueprintCallable)
	void StartGatheringPlayerData() { SetComponentTickEnabled(true); }

	UFUNCTION(BlueprintCallable)
	void StopGatheringPlayerData() { SetComponentTickEnabled(false); };

	// called before the game state is serialized (aka saved)
	UFUNCTION()
	void OnSave();

	// called after the game state is reloaded (aka loaded)
	UFUNCTION()
	void OnLoad();

	UFUNCTION()
	void OnPlayerRematerialize();

	UFUNCTION()
	void OnPlayerRespawn();

	// Called if the player took damage but didn't die. I hope that it won't happen that often - OnDeath should be called all the time
	void OnHpLost(const FSoDmg& Damage);

	// Called every time the player dies. That meant to be damn often.
	void OnDeath(const bool bSoulKeeperActive);

	// Called every time we use a spell
	void OnSpellUsed(const USoItemTemplateRuneStone* UsedSpell);

	// Called every time the users explicitly (from the UI) equips an item
	void OnEquippedItem(const USoItemTemplate* Item);

	// Called every time the user explicitly (from the UI) unequips an item
	void OnUnEquippedItem(const USoItemTemplate* Item);

	// Gets the player stats from all session
	const FSoPlayerProgressStats& GetStatsFromAllSession() const { return ProgressStats; }
	FSoPlayerProgressStats& GetStatsFromAllSession() { return ProgressStats; }
	//const FSoPlayerProgressStats& GetStatsFromCurrentSession() const { return CurrentSessionStats; }
	FSoPlayerProgressStats& GetTempAnalyticsStats() { return TempAnalyticsStats; }
	const FSoPlayerProgressStats& GetTempAnalyticsStats() const { return TempAnalyticsStats; }

	FORCEINLINE void ResetTempAnalyticsStats() { TempAnalyticsStats = {}; }

	DECLARE_EVENT(USoPlayerProgress, FSoPlayerProgressPreSaveLoad)
	FSoPlayerProgressPreSaveLoad& OnPreLoad() { return PreLoadEvent; }
	FSoPlayerProgressPreSaveLoad& OnPreSave() { return PreSaveEvent; }

	UFUNCTION(BlueprintCallable)
	void MarkCurrentEpisodeAsCompleted();

	UFUNCTION(BlueprintPure)
	float GetCurrentPlayTimeSeconds() const;

protected:
	// Tick
	void UpdateSpline(float DeltaSeconds);
	void UpdateChapter(float DeltaSeconds);

	// Helper methods
	ASoPlayerSpline* GetOwnerSpline();

	void IncreaseTotalPlayTimeSeconds(float DeltaTime);

	FORCEINLINE void IncrementTotalDeathNum()
	{
		ProgressStats.TotalDeathNum++;
		TempAnalyticsStats.TotalDeathNum++;
	}

	FORCEINLINE void IncrementTotalDeathWithSoulKeeperNum()
	{
		ProgressStats.TotalDeathWithSoulKeeperNum++;
		TempAnalyticsStats.TotalDeathWithSoulKeeperNum++;
	}

	FORCEINLINE void IncrementTotalDeathWithCheckpointNum()
	{
		ProgressStats.TotalDeathWithCheckpointNum++;
		TempAnalyticsStats.TotalDeathWithCheckpointNum++;
	}

	FORCEINLINE void IncreaseTotalLostHP(ESoDmgType DamageType, int32 LostHP)
	{
		ProgressStats.IncreaseTotalLostHP(DamageType, LostHP);
		TempAnalyticsStats.IncreaseTotalLostHP(DamageType, LostHP);
	}

	FORCEINLINE void IncrementSplineEnterNum(FName SplineName)
	{
		ProgressStats.IncrementMapSplineEnterNum(OwnerChapterName, SplineName);
		TempAnalyticsStats.IncrementMapSplineEnterNum(OwnerChapterName, SplineName);
	}

	FORCEINLINE void IncreaseSplineTimeSpent(FName SplineName, float DeltaSeconds)
	{
		ProgressStats.IncreaseMapSplineTimeSpent(OwnerChapterName, SplineName, DeltaSeconds);
		TempAnalyticsStats.IncreaseMapSplineTimeSpent(OwnerChapterName, SplineName, DeltaSeconds);
	}

	FORCEINLINE void AddSplineDeathEntry(FName SplineName, const FSoSplineDeathContext& DeathContext)
	{
		ProgressStats.AddMapSplineDeathEntry(OwnerChapterName, SplineName, DeathContext);
		TempAnalyticsStats.AddMapSplineDeathEntry(OwnerChapterName, SplineName, DeathContext);
	}

	FORCEINLINE void IncreaseSplineLostHP(FName SplineName, ESoDmgType DamageType, int32 LostHP)
	{
		ProgressStats.IncreaseMapSplineLostHP(OwnerChapterName, SplineName, DamageType, LostHP);
		TempAnalyticsStats.IncreaseMapSplineLostHP(OwnerChapterName, SplineName, DamageType, LostHP);
	}

	FORCEINLINE void UpdateSplineFirstEnterTime(FName SplineName)
	{
		ProgressStats.UpdateMapSplineTimeFirstEnter(OwnerChapterName, SplineName, ProgressStats.TotalPlayTimeSeconds);
		// NOTE not interested in updating spline first time enter data for the TempAnalyticsStats
	}

	FORCEINLINE void SetSplineLength(FName SplineName, float Length)
	{
		ProgressStats.SetMapSplineLength(OwnerChapterName, SplineName, Length);
		// NOTE not interested in updating spline first time enter data for the TempAnalyticsStats
	}

	FORCEINLINE void SpellCasted(FName SpellName)
	{
		ProgressStats.SpellCasted(SpellName);
		TempAnalyticsStats.SpellCasted(SpellName);
	}

protected:
	// Our master
	UPROPERTY(BlueprintReadOnly)
	ASoCharacter* SoOwner = nullptr;

	// Should we spline stats?
	UPROPERTY(BlueprintReadOnly)
	bool bCollectSplineStats = true;

	// Stats for the current player, contains data from all sessions, loaded from/to save file.
	UPROPERTY(BlueprintReadOnly)
	FSoPlayerProgressStats ProgressStats;

	// Current player stats, tracks only some variables for the current session, not saved on disk
	// NOTE: This is discarded on session end.
	// UPROPERTY(BlueprintReadOnly)
	//FSoPlayerProgressStats CurrentSessionStats;

	// Temporary stats for the analytics, that will get reset every time the analytics sends the data
	// Will get reset on save/load
	UPROPERTY(BlueprintReadOnly)
	FSoPlayerProgressStats TempAnalyticsStats;

	// the spline location of the owner player
	FSoSplinePoint OwnerSplineLocation;

	// Current Chapter Name
	FName OwnerChapterName;

	/** Broadcasts before loading anything in OnLoad */
	FSoPlayerProgressPreSaveLoad PreLoadEvent;

	/** Broadcasts before saving anything in OnSave */
	FSoPlayerProgressPreSaveLoad PreSaveEvent;

	// Cached game instance
	UPROPERTY()
	USoGameInstance* GameInstance;

	UPROPERTY()
	USoOnlineRichPresence* OnlineRichPresence;
};
