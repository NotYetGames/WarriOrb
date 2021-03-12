// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Misc/SecureHash.h"

#include "SoPlayerProgressMapStats.h"
#include "SoPlayerProgressItemStats.h"

#include "SoPlayerProgressStats.generated.h"


UENUM(BlueprintType)
enum class ESoMapProgressState : uint8
{
	None,

	// Could also be in the middle of the map
	Started,

	Completed
};

// Hold the stats for the player progress
USTRUCT(BlueprintType)
struct FSoPlayerProgressStats
{
	GENERATED_USTRUCT_BODY()
public:
	FSoPlayerProgressStats()
	{
		// Initialize the damage type table
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	bool operator==(const FSoPlayerProgressStats& Other) const;

	FORCEINLINE void UpdateTotalAverageFPS(int32 NewAverageFPS)
	{
		if (NewAverageFPS < 0)
			return;

		if (TotalAverageFPS > 0)
			TotalAverageFPS = FMath::RoundToInt((TotalAverageFPS + NewAverageFPS) / 2.f);
		else
			TotalAverageFPS = NewAverageFPS;
	}

	FORCEINLINE void IncreaseTotalLostHP(ESoDmgType DamageType, int32 LostHP)
	{
		if (LostHP < 0)
			return;

		TotalLostHp += LostHP;
		if (!TotalLostHpByDamageTypeTable.Contains(DamageType))
			TotalLostHpByDamageTypeTable.Add(DamageType, 0);

		TotalLostHpByDamageTypeTable[DamageType] += LostHP;
	}

	// Is this Empty? (aka default)
	FORCEINLINE bool IsEmpty() const
	{
		return TotalDeathNum == 0 &&
			TotalDeathWithSoulKeeperNum == 0 &&
			TotalDeathWithCheckpointNum == 0 &&
			FMath::IsNearlyEqual(TotalLostHp, 0.f, KINDA_SMALL_NUMBER) &&
			FMath::IsNearlyEqual(TotalPlayTimeSeconds, 0.f, KINDA_SMALL_NUMBER);
	}

	// Contains the MapName:Milestone
	FORCEINLINE bool HasMapMilestone(FName MapName, FName MilestoneName) const
	{
		return MapsProgressTable.Contains(MapName) ? MapsProgressTable.FindChecked(MapName).HasMilestone(MilestoneName) : false;
	}

	FORCEINLINE void AddMapMilestone(FName MapName, FName MilestoneName, float TimeSeconds = INDEX_NONE)
	{
		if (MapName != NAME_None && MilestoneName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).AddMilestone(MilestoneName, TimeSeconds);
	}

	// Contains the MapName:SplineName
	FORCEINLINE bool HasMapSpline(FName MapName, FName SplineName) const
	{
		return MapsProgressTable.Contains(MapName) ? MapsProgressTable.FindChecked(MapName).HasSpline(SplineName)  : false;
	}

	FORCEINLINE bool HasMapSplineTimeFirstEnter(FName MapName, FName SplineName) const
	{
		return MapsProgressTable.Contains(MapName) ? MapsProgressTable.FindChecked(MapName).HasSplineTimeFirstEnter(SplineName) : false;
	}

	FORCEINLINE bool CanMapSplineCriticalFPSAreasBeSent(FName MapName, FName SplineName) const
	{
		return MapsProgressTable.Contains(MapName) ? MapsProgressTable.FindChecked(MapName).CanSplineCriticalFPSAreasBeSent(SplineName) : false;
	}

	FORCEINLINE void IncrementMapSplineEnterNum(FName MapName, FName SplineName)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).IncrementSplineEnterNum(SplineName);
	}

	FORCEINLINE void IncrementMapSplineCriticalFPSAreasSentRecordsNum(FName MapName, FName SplineName)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).IncrementSplineCriticalFPSAreasSentRecordsNum(SplineName);
	}

	FORCEINLINE void IncreaseMapSplineLostHP(FName MapName, FName SplineName, ESoDmgType DamageType, int32 LostHP)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).IncreaseSplineLostHP(SplineName, DamageType, LostHP);
	}

	FORCEINLINE void IncreaseMapSplineTimeSpent(FName MapName, FName SplineName, float DeltaSeconds)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).IncreaseSplineTimeSpent(SplineName, DeltaSeconds);
	}

	FORCEINLINE void AddMapSplineDeathEntry(FName MapName, FName SplineName, const FSoSplineDeathContext& DeathContext)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).AddSplineDeathEntry(SplineName, DeathContext);
	}

	// Updates the MapName:SplineName:FirstEnterTimeSeconds with FirstEnterTimeSeconds.
	// NOTE: Only updates the first enter time enter only if the first time is not valid/not set
	FORCEINLINE void UpdateMapSplineTimeFirstEnter(FName MapName, FName SplineName, float FirstEnterTimeSeconds)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).UpdateSplineTimeFirstEnter(SplineName, FirstEnterTimeSeconds);
	}

	FORCEINLINE void UpdateMapSplineAverageFPS(FName MapName, FName SplineName, int32 NewAverageFPS)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).UpdateSplineAverageFPS(SplineName, NewAverageFPS);
	}

	FORCEINLINE void UpdateMapSplineCriticalFPSAreas(FName MapName, FName SplineName, const FSoSplineCriticalFPSAreas& Areas)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).UpdateSplineCriticalFPSAreas(SplineName, Areas);
	}

	FORCEINLINE void SetMapSplineLength(FName MapName, FName SplineName, float Length)
	{
		if (MapName != NAME_None && SplineName != NAME_None)
			MapsProgressTable.FindOrAdd(MapName).SetSplineLength(SplineName, Length);
	}

	FORCEINLINE void ResetGameplayTotalVariables()
	{
		TotalDeathNum = 0;
		TotalDeathWithSoulKeeperNum = 0;
		TotalDeathWithCheckpointNum = 0;
		TotalLostHp = 0;
		TotalPlayTimeSeconds = 0.f;

		TotalLostHpByDamageTypeTable.Empty();
	}

	FORCEINLINE void EquipItem(FName ItemName, float TimeSeconds)
	{
		if (ItemName != NAME_None)
			ItemsStatsTable.FindOrAdd(ItemName).EquipItem(TimeSeconds);
	}

	FORCEINLINE void UnEquipItem(FName ItemName, float TimeSeconds)
	{
		if (ItemName != NAME_None)
			ItemsStatsTable.FindOrAdd(ItemName).UnEquipItem(TimeSeconds);
	}

	FORCEINLINE void SpellCasted(FName SpellName)
	{
		if (SpellName != NAME_None)
			SpellsStatsTable.FindOrAdd(SpellName).SpellCasted();
	}

	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Did we complete the map or what?
	UPROPERTY()
	ESoMapProgressState ProgressState = ESoMapProgressState::None;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalDeathNum = 0;

	// Deaths with soul keeper used
	// Mutual exclusive with TotalCheckpointDeathNum
	UPROPERTY(BlueprintReadOnly)
	int32 TotalDeathWithSoulKeeperNum = 0;

	// Deaths with checkpoint used
	// Mutual exclusive with TotalSoulKeeperDeathNum
	UPROPERTY(BlueprintReadOnly)
	int32 TotalDeathWithCheckpointNum = 0;

	// Total lost HP
	UPROPERTY(BlueprintReadOnly)
	int32 TotalLostHp = 0;

	// Total play time in seconds since game started.
	// Float here because we require more precision.
	UPROPERTY(BlueprintReadOnly)
	float TotalPlayTimeSeconds = 0.f;

	// UPROPERTY(BlueprintReadOnly)
	// float TotalPlayTimeSinceRestart = 0.f;

	// Average FPS over all all game
	UPROPERTY(BlueprintReadOnly)
	int32 TotalAverageFPS = 0;

	// Maps from damage type (ESoDamageType) to total lost HP by that damage.
	// Index/Key: ESoDamageType (casted to int)
	// Value: int32, total lost hp for the key damage type
	UPROPERTY(BlueprintReadOnly)
	TMap<ESoDmgType, int32> TotalLostHpByDamageTypeTable;
	// TArray<int32> TotalLostHpByDamageTypeTable;


	// Holds stats data about each map
	// Key: Map Name
	// Value: Stats about this map
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSoPlayerProgressMapStats> MapsProgressTable;

	// Holds stats about each item used by the player.
	// Key: Item Name
	// Value: Stats about this item
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSoPlayerProgressItemStats> ItemsStatsTable;

	// Holds stats about each spell used by the user
	// Key: Spell Name
	// Value: Stats about this spell
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSoPlayerProgressSpellStats> SpellsStatsTable;
};
