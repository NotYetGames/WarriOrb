// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Misc/SecureHash.h"

#include "SoPlayerProgressItemStats.generated.h"


// Holds the stats about the usage of an item
USTRUCT(BlueprintType)
struct FSoPlayerProgressItemUsageStats
{
	GENERATED_USTRUCT_BODY()
public:
	FSoPlayerProgressItemUsageStats()
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	FORCEINLINE bool operator==(const FSoPlayerProgressItemUsageStats& Other) const
	{
		return TimeEquippedSeconds == Other.TimeEquippedSeconds &&
			TimeUnEquippedSeconds == Other.TimeUnEquippedSeconds &&
			bSent == Other.bSent;
	}

	// Do we have a valid range?
	FORCEINLINE bool IsValidRange() const
	{
		return IsTimeEquippedValid() && IsTimeUnEquippedValid() && TimeEquippedSeconds <= TimeUnEquippedSeconds;
	}

	FORCEINLINE bool IsTimeEquippedValid() const { return TimeEquippedSeconds >= 0; }
	FORCEINLINE bool IsTimeUnEquippedValid() const { return TimeUnEquippedSeconds >= 0;  }

	FORCEINLINE void SetTimeEquippedSeconds(const float InTimeSeconds)
	{
		if (!IsTimeEquippedValid())
		{
			TimeEquippedSeconds = FMath::TruncToInt(InTimeSeconds);
			if (TimeEquippedSeconds < INDEX_NONE)
				TimeEquippedSeconds = INDEX_NONE;
		}
	}

	FORCEINLINE void SetTimeUnEquippedSeconds(const float InTimeSeconds)
	{
		if (!IsTimeUnEquippedValid())
		{
			TimeUnEquippedSeconds = FMath::TruncToInt(InTimeSeconds);
			if (TimeUnEquippedSeconds < INDEX_NONE)
				TimeUnEquippedSeconds = INDEX_NONE;
		}
	}

	FORCEINLINE bool CanBeSent() const
	{
		return bSent == false && IsValidRange();
	}
	FORCEINLINE void MarkAsSent()
	{
		if (IsValidRange())
			bSent = true;
	}

	FORCEINLINE int32 GetTimeUsedSeconds() const
	{
		return IsValidRange() ? TimeUnEquippedSeconds - TimeEquippedSeconds : 0;
	}

	void UpdateSHA1(FSHA1& HashState) const;
public:
	// Time at which this item was equipped in absolute seconds (from all sessions)
	UPROPERTY(BlueprintReadOnly)
	int32 TimeEquippedSeconds = INDEX_NONE;

	// Time at which this item was unequipped in absolute seconds (from all sessions)
	UPROPERTY(BlueprintReadOnly)
	int32 TimeUnEquippedSeconds = INDEX_NONE;

	// Was this interval sent to the analytics provider?
	UPROPERTY(BlueprintReadOnly)
	bool bSent = false;
};


// Holds the stats about an item
USTRUCT(BlueprintType)
struct FSoPlayerProgressItemStats
{
	GENERATED_USTRUCT_BODY()
public:
	FSoPlayerProgressItemStats()
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	FORCEINLINE void MarkAllItemsUsageAsSent()
	{
		for (auto& Elem : Usages)
			Elem.MarkAsSent();
	}

	// Equipped this item
	FORCEINLINE void EquipItem(const float TimeSeconds)
	{
		CurrentUsage = {};
		CurrentUsage.SetTimeEquippedSeconds(TimeSeconds);
	}

	// Unequipped this item
	FORCEINLINE void UnEquipItem(const float TimeSeconds)
	{
		CurrentUsage.SetTimeUnEquippedSeconds(TimeSeconds);

		// Update current usages
		if (CurrentUsage.IsValidRange())
			Usages.Add(CurrentUsage);

		CurrentUsage = {};
	}

	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Tracks the usage of items, all items here should have a valid range
	UPROPERTY(BlueprintReadOnly)
	TArray<FSoPlayerProgressItemUsageStats> Usages;

	// Holds the current usage buffer, this means that this has a valid equipped time but not a valid unequipped time.
	UPROPERTY(BlueprintReadOnly)
	FSoPlayerProgressItemUsageStats CurrentUsage;
};


// Holds the stats about a spell
USTRUCT(BlueprintType)
struct FSoPlayerProgressSpellStats
{
	GENERATED_USTRUCT_BODY()
public:
	FSoPlayerProgressSpellStats()
	{
		CheckAndFixIntegrity();
	}

	void CheckAndFixIntegrity();

	void SpellCasted() { CastNum++; }

	void UpdateSHA1(FSHA1& HashState) const;
public:
	// Number of times it was casted
	UPROPERTY(BlueprintReadOnly)
	int32 CastNum = 0;
};
