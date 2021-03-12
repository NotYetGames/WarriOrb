// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/SecureHash.h"

#include "SoPlayerProgressSplineStats.h"

#include "SoPlayerProgressMapStats.generated.h"


// Data about each milestone
USTRUCT(BlueprintType)
struct FSoPlayerProgressMilestoneStats
{
	GENERATED_USTRUCT_BODY()
public:
	FSoPlayerProgressMilestoneStats()
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	FORCEINLINE bool operator==(const FSoPlayerProgressMilestoneStats& Other) const
	{
		return TimeSeconds == Other.TimeSeconds;
	}

	FORCEINLINE bool HasTime() { return TimeSeconds > INDEX_NONE; }
	FORCEINLINE void SetTimeSeconds(const float InTimeSeconds)
	{
		TimeSeconds = FMath::TruncToInt(InTimeSeconds);
		if (TimeSeconds < INDEX_NONE)
			TimeSeconds = INDEX_NONE;
	}

	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Time at which this milestone was recorded. In absolute seconds (from all sessions)
	// If TimeSeconds = INDEX_NONE it means we do not care about the time
	UPROPERTY(BlueprintReadOnly)
	int32 TimeSeconds = INDEX_NONE;

	// TODO add bSent for later when we the user can opt out of sending analytics
};


// Contains stats about a map
// Can be used for an Act Map or an Episode Map
USTRUCT(BlueprintType)
struct FSoPlayerProgressMapStats
{
	GENERATED_USTRUCT_BODY()
public:
	FSoPlayerProgressMapStats()
	{
		CheckAndFixIntegrity();
	}

	FORCEINLINE void CheckAndFixIntegrity()
	{
		for (auto& Elem : Splines)
			Elem.Value.CheckAndFixIntegrity();

		for (auto& Elem : MilestonesCompleted)
			Elem.Value.CheckAndFixIntegrity();
	}

	bool operator==(const FSoPlayerProgressMapStats& Other) const;

	FORCEINLINE void AddMilestone(FName MilestoneName, const float TimeSeconds = INDEX_NONE)
	{
		FSoPlayerProgressMilestoneStats MilestoneStats;
		MilestoneStats.SetTimeSeconds(TimeSeconds);
		MilestonesCompleted.Add(MilestoneName, MilestoneStats);
	}
	FORCEINLINE bool HasMilestone(FName MilestoneName) const { return MilestonesCompleted.Contains(MilestoneName); }

	FORCEINLINE bool HasSpline(FName SplineName) const { return Splines.Contains(SplineName); }
	FORCEINLINE bool HasSplineTimeFirstEnter(FName SplineName) const
	{
		return HasSpline(SplineName) ? Splines.FindChecked(SplineName).IsTimeFirstEnterValid() : false;
	}
	FORCEINLINE bool CanSplineCriticalFPSAreasBeSent(FName SplineName) const
	{
		return HasSpline(SplineName) ? Splines.FindChecked(SplineName).CanCriticalFPSAreasBeSent() : false;
	}

	// Only updates if the FirstEnterTimeSeconds is invalid
	FORCEINLINE void UpdateSplineTimeFirstEnter(FName SplineName, float FirstEnterTimeSeconds)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).SetTimeFirstEnterSeconds(FirstEnterTimeSeconds);
	}

	FORCEINLINE void UpdateSplineCriticalFPSAreas(FName SplineName, const FSoSplineCriticalFPSAreas& Areas)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).UpdateCriticalFPSAreas(Areas);
	}

	FORCEINLINE void UpdateSplineAverageFPS(FName SplineName, int32 NewAverageFPS)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).UpdateAverageFPS(NewAverageFPS);
	}

	FORCEINLINE void SetSplineLength(FName SplineName, float Length)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).SetSplineLength(Length);
	}

	FORCEINLINE void IncrementSplineCriticalFPSAreasSentRecordsNum(const FName SplineName)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).IncrementCriticalFPSAreasSentRecordsNum();
	}

	FORCEINLINE void IncrementSplineEnterNum(FName SplineName)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).IncrementEnterNum();
	}

	FORCEINLINE void IncreaseSplineLostHP(FName SplineName, ESoDmgType DamageType, int32 LostHP)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).IncreaseLostHP(DamageType, LostHP);
	}

	FORCEINLINE void IncreaseSplineTimeSpent(FName SplineName, float DeltaSeconds)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).IncreaseTimeSpent(DeltaSeconds);
	}

	FORCEINLINE void AddSplineDeathEntry(FName SplineName, const FSoSplineDeathContext& DeathContext)
	{
		if (SplineName != NAME_None)
			Splines.FindOrAdd(SplineName).AddDeathEntry(DeathContext);
	}

	// Gets all the splines first time enter value that need to be sent.
	// All splines that had their FirstEnterTimeSeconds set in the current session and bSentTimeFirstEnter = false
	//
	// Returns:
	//	Map:
	//		Key: SplineName
	//		Value: FirstEnterTimeSeconds
	FORCEINLINE TMap<FName, int32> GetAllSplinesFirstTimeEnterForSending() const
	{
		TMap<FName, int32> FilteredSplines;
		for (const auto& Elem : Splines)
			if (Elem.Value.IsTimeFirstEnterValid() && Elem.Value.bSentTimeFirstEnter == false && Elem.Value.bSetFirstTimeEnterInCurrentSession == true)
				FilteredSplines.Add(Elem.Key, Elem.Value.TimeFirstEnterSeconds);

		return FilteredSplines;
	}

	FORCEINLINE void MarkAllSplinesFirstTimeEnterAsSent()
	{
		for (auto& Elem : Splines)
			Elem.Value.MarkTimeFirstEnterAsSent();
	}

	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Holds info about each spline name in this map
	// Key: Spline Name
	// Value: Stats about this spline
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSoPlayerProgressSplineStats> Splines;

	// All the completed milestones
	UPROPERTY(BlueprintReadOnly)
	TMap<FName, FSoPlayerProgressMilestoneStats> MilestonesCompleted;
};
