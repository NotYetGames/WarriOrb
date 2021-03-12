// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/SecureHash.h"

#include "Items/SoItem.h"

#include "SoPlayerProgressSplineStats.generated.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Some player stat stuff
USTRUCT(BlueprintType)
struct FSoSplineDeathContext
{
	GENERATED_USTRUCT_BODY()
public:
	FSoSplineDeathContext()
	{
		CheckAndFixIntegrity();
	}
	FSoSplineDeathContext(const float InDistance, const float InTime, const bool bInSoulKeeperActive) :
		Distance(FMath::TruncToInt(InDistance)), TimeSeconds(FMath::TruncToInt(InTime)), bSoulKeeperActive(bInSoulKeeperActive)
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	FORCEINLINE bool operator==(const FSoSplineDeathContext& Other) const
	{
		return Distance == Other.Distance &&
			TimeSeconds == Other.TimeSeconds &&
			bSoulKeeperActive == Other.bSoulKeeperActive;
	}

public:
	// Distance of death along the spline.
	UPROPERTY(BlueprintReadOnly)
	int32 Distance = 0;

	// Absolute time of death (to all sessions)
	UPROPERTY(BlueprintReadOnly)
	int32 TimeSeconds = 0;

	// Is the soul keeper used?
	UPROPERTY(BlueprintReadOnly)
	bool bSoulKeeperActive = false;
};


// Holds info about an FPS occurrence
USTRUCT(BlueprintType)
struct FSoSplineCriticalFPSLocation
{
	GENERATED_USTRUCT_BODY()
public:
	FSoSplineCriticalFPSLocation()
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	// Is this similar to the other? At least ThresholdSimilar units apart
	FORCEINLINE bool IsSimilarTo(const FSoSplineCriticalFPSLocation& Other) const
	{
		return FMath::Abs(AverageDistance - Other.AverageDistance) < ThresholdDistanceSimilar;
	}

	// Combine with the other FPS critical
	// Assumes this.IsSimilarTo(Other)
	FORCEINLINE void MergeWith(const FSoSplineCriticalFPSLocation& Other)
	{
		TimeSeconds += Other.TimeSeconds;
		UpdateAverageDistance(Other.AverageDistance);
		UpdateCriticalAverageFPS(Other.AverageCriticalFPS);
	}

	FORCEINLINE bool IsValid() const
	{
		return AverageDistance >= 0 && AverageCriticalFPS > 0 && TimeSeconds > 1.f;
	}

	FORCEINLINE void UpdateAverageDistance(const float NewDistance)
	{
		if (NewDistance < 0.f)
			return;

		if (AverageDistance > 0)
			AverageDistance = FMath::RoundToInt((AverageDistance + NewDistance) / 2.f);
		else
			AverageDistance = FMath::TruncToInt(NewDistance);
	}

	FORCEINLINE void UpdateCriticalAverageFPS(const float NewAverageFPS)
	{
		if (NewAverageFPS < 0.f)
			return;

		if (AverageCriticalFPS > 0)
			AverageCriticalFPS = FMath::RoundToInt((AverageCriticalFPS + NewAverageFPS) / 2.f);
		else
			AverageCriticalFPS = FMath::TruncToInt(NewAverageFPS);
	}

public:
	// Average Distance of critical FPS occurrences
	UPROPERTY(BlueprintReadOnly)
	int32 AverageDistance = 0;

	// Critical FPS found on this spline
	UPROPERTY(BlueprintReadOnly)
	int32 AverageCriticalFPS = 0;

	// For how much time this critical FPS occur
	UPROPERTY(BlueprintReadOnly)
	float TimeSeconds = 0.f;

	// The threshold so that we do not collect multiple spline in the same are
	static constexpr int32 ThresholdDistanceSimilar = 1000;
};


// Holds the Array of areas
USTRUCT(BlueprintType)
struct FSoSplineCriticalFPSAreas
{
	GENERATED_USTRUCT_BODY()
public:
	FSoSplineCriticalFPSAreas()
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	// Merge other areas into this one
	FORCEINLINE void MergeWith(const FSoSplineCriticalFPSAreas& Other)
	{
		if (!Other.HasAreas())
			return;

		if (Areas.Num() == 0)
		{
			// Copy
			Areas = Other.Areas;
		}
		else
		{
			// Merge in
			for (const FSoSplineCriticalFPSLocation& OtherArea : Other.Areas)
				AddCriticalFPS(OtherArea);
		}
	}

	void AddCriticalFPS(const FSoSplineCriticalFPSLocation& CriticalFPSEntry)
	{
		// TODO maybe add/search on sorted array, binary search
		if (!CriticalFPSEntry.IsValid())
			return;

		// Is similar to anything else?
		for (auto& Elem : Areas)
			if (Elem.IsSimilarTo(CriticalFPSEntry))
			{
				// Combine with the first occurrence
				Elem.MergeWith(CriticalFPSEntry);
				return;
			}

		// Can't fit anymore :shrug
		if (Areas.Num() + 1 > MaxCriticalFPSAreas)
			return;

		// Can add
		Areas.Add(CriticalFPSEntry);
	}

	FORCEINLINE bool HasAreas() const { return Areas.Num() > 0; }
	FORCEINLINE void IncrementSentRecordsNum()
	{
		if (HasAreas())
			SentRecordsNum++;
	}

	FORCEINLINE bool CanBeSent() const
	{
		// MaxCriticalFPSAreas is our limit
		return HasAreas() && SentRecordsNum <= MaxCriticalFPSAreas;
	}

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<FSoSplineCriticalFPSLocation> Areas;

	// How many times  did we send this data to the analytics server
	UPROPERTY(BlueprintReadOnly)
	int32 SentRecordsNum = 0;

	// Maximum amount of max critical FPS areas, depends on the SplineLength if set
	int32 MaxCriticalFPSAreas = 10;
};


// Holds the player progress about some spline
USTRUCT(BlueprintType)
struct FSoPlayerProgressSplineStats
{
	GENERATED_USTRUCT_BODY()
public:
	FSoPlayerProgressSplineStats()
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();
	bool operator==(const FSoPlayerProgressSplineStats& Other) const;

	FORCEINLINE bool IsTimeFirstEnterValid() const { return TimeFirstEnterSeconds >= 0; }
	FORCEINLINE void SetTimeFirstEnterSeconds(const float Seconds)
	{
		// Update if invalid
		if (!IsTimeFirstEnterValid())
		{
			bSetFirstTimeEnterInCurrentSession = true;
			TimeFirstEnterSeconds = FMath::TruncToInt(Seconds);
			if (TimeFirstEnterSeconds < INDEX_NONE)
				TimeFirstEnterSeconds = INDEX_NONE;
		}
	}
	FORCEINLINE void MarkTimeFirstEnterAsSent()
	{
		if (IsTimeFirstEnterValid())
			bSentTimeFirstEnter = true;
	}

	FORCEINLINE void IncrementEnterNum() { EnterNum++; }
	FORCEINLINE void IncreaseTimeSpent(float DeltaSeconds) { TimeSpentSeconds += FMath::Abs(DeltaSeconds); }

	FORCEINLINE void IncreaseLostHP(ESoDmgType DamageType, int32 LostHP)
	{
		if (LostHP < 0)
			return;

		LostHp += LostHP;
		if (!LostHpByDamageTypeTable.Contains(DamageType))
			LostHpByDamageTypeTable.Add(DamageType, 0);

		LostHpByDamageTypeTable[DamageType] += LostHP;
	}

	FORCEINLINE void AddDeathEntry(const FSoSplineDeathContext& DeathContext)
	{
		DeathData.Add(DeathContext);
	}

	// Useful to not spam the analytics server.
	FORCEINLINE bool HasStatsForAtLeast(float AccumulateThreshold = 1.0f) const
	{
		return EnterNum > 0 && TimeSpentSeconds > AccumulateThreshold;
	}

	FORCEINLINE void UpdateAverageFPS(int32 NewAverageFPS)
	{
		if (NewAverageFPS < 0)
			return;

		if (AverageFPS > 0)
			AverageFPS = FMath::RoundToInt((AverageFPS + NewAverageFPS) / 2.f);
		else
			AverageFPS = NewAverageFPS;
	}

	FORCEINLINE void UpdateCriticalFPSAreas(const FSoSplineCriticalFPSAreas& Areas)
	{
		CriticalFPSAreas.MergeWith(Areas);
	}

	FORCEINLINE void IncrementCriticalFPSAreasSentRecordsNum()
	{
		CriticalFPSAreas.IncrementSentRecordsNum();
	}

	FORCEINLINE void SetSplineLength(float InSplineLength)
	{
		SplineLength = FMath::TruncToInt(InSplineLength);
		// At least two entries for this
		CriticalFPSAreas.MaxCriticalFPSAreas = FMath::Max(2, SplineLength / FSoSplineCriticalFPSLocation::ThresholdDistanceSimilar);
	}

	FORCEINLINE bool HasSplineLength() const { return SplineLength > 0; }
	FORCEINLINE bool CanCriticalFPSAreasBeSent() const { return CriticalFPSAreas.CanBeSent(); }

	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Time at which this spline was first entered at. In Absolute seconds (from all seconds) in the current session.
	// Int here because we do not require that much precision
	UPROPERTY(BlueprintReadOnly)
	int32 TimeFirstEnterSeconds = INDEX_NONE;

	// Was the time first enter for this spline already sent?
	UPROPERTY(BlueprintReadOnly)
	bool bSentTimeFirstEnter = false;

	// The number of times player entered this spline
	UPROPERTY(BlueprintReadOnly)
	int32 EnterNum = 0;

	// Amount of time the player spent on the spline
	UPROPERTY(BlueprintReadOnly)
	float TimeSpentSeconds = 0.f;

	// Average FPS on this spline.
	UPROPERTY(BlueprintReadOnly)
	int32 AverageFPS = 0;

	// Holds the info about the critical FPS areas
	UPROPERTY(BlueprintReadOnly)
	FSoSplineCriticalFPSAreas CriticalFPSAreas;

	// Data about deaths on this spline
	UPROPERTY(BlueprintReadOnly)
	TArray<FSoSplineDeathContext> DeathData;

	// Data about HP lost on this spline
	UPROPERTY(BlueprintReadOnly)
	int32 LostHp = 0;

	// Data about HP lost on this spline by damage type
	// Index/Key: ESoDamageType (casted to int)
	// Value: int32, total lost hp for the key damage type
	UPROPERTY(BlueprintReadOnly)
	TMap<ESoDmgType, int32> LostHpByDamageTypeTable;

	// Useful to know in which session the FirstEnterTimeSeconds was set.
	// TODO remove this and resend events not in current session but that have bSentTimeFirstEnter = false when UI is done.
	bool bSetFirstTimeEnterInCurrentSession = false;

protected:
	// The length of this spline.
	int32 SplineLength = 0;
};
