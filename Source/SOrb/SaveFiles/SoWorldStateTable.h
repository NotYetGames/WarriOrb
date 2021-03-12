// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Misc/SecureHash.h"

#include "SoWorldStateTypes.h"
#include "Stats/SoPlayerProgressStats.h"
#include "DlgMemory.h"
#include "Basic/SoDifficulty.h"

#include "SoWorldStateTable.generated.h"


// Holds metadata info for each save slot entry
USTRUCT()
struct FSoStateTableMetadata
{
	GENERATED_USTRUCT_BODY()
public:
	FSoStateTableMetadata() {}
	FSoStateTableMetadata(const TArray<USoItemTemplate*>& InEquippedItems) :
		EquippedItems(InEquippedItems) {}

	// Is this Empty? (aka default)
	FORCEINLINE bool IsEmpty() const
	{
		return SaveTime.IsEmpty() &&
			CheckpointLocationName == NAME_None &&
			EquippedItems.Num() == 0 &&
			ProgressStats.IsEmpty();
	}

	void CheckAndFixIntegrity();

	// Update SaveTime to reflect the current time.
	FORCEINLINE void SetSaveTimeToNow()
	{
		// UTC time so that we can convert back easily to local time if the user changes time zone
		// See time format https://en.wikipedia.org/wiki/ISO_8601
		SaveTime = *FDateTime::UtcNow().ToIso8601();
	}

	bool GetSaveTimeAsDateTime(FDateTime& OutDateTime, bool bLocalTime) const;

	FORCEINLINE bool operator==(const FSoStateTableMetadata& Other) const
	{
		return SaveTime == Other.SaveTime &&
			CheckpointLocationName == Other.CheckpointLocationName &&
			MapName == Other.MapName &&
			ProgressStats == Other.ProgressStats &&
			EquippedItems == Other.EquippedItems;
	}

	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Time this save was made at, UTC time
	UPROPERTY()
	FString SaveTime;

	// Checksum of all the data in the files besides this value
	UPROPERTY()
	int32 ChecksumVersion = 0;
	UPROPERTY()
	FString Checksum;

	// In what version of the game was this saved
	UPROPERTY()
	FString GameBuildVersion;

	// Build branch
	UPROPERTY()
	FString GameBuildBranch;

	// Git commit number
	UPROPERTY()
	FString GameBuildCommit;

	UPROPERTY()
	ESoDifficulty Difficulty = ESoDifficulty::Intended;

	// Name of the checkpoint location this save is at
	// Also saved in the PlayerData, but we want to query the location from the save slots
	UPROPERTY()
	FName CheckpointLocationName;

	UPROPERTY()
	FName DisplayedProgressName;

	// Current Map name (Act/Episode), in internal form, use conversion to friendly if you want it in that way
	UPROPERTY()
	FName MapName;

	// Progress stats of the game (for all the sessions)
	UPROPERTY()
	FSoPlayerProgressStats ProgressStats;

	// Hold all the currently equipped items
	// Useful if we want to preview the character in the save slots UI.
	UPROPERTY()
	TArray<USoItemTemplate*> EquippedItems;

	UPROPERTY()
	int32 InitialMaxHealth = 30.f;

	UPROPERTY()
	int32 InitialSpellsCapacity = 2.f;
};


// Holds the current state information aka a save
USTRUCT()
struct FSoStateTable
{
	GENERATED_USTRUCT_BODY()
public:
	void CheckAndFixIntegrity();

	// Returns true if we must reset everything
	bool CheckAndFixIntegrityFromChecksum();

	FORCEINLINE void UpdateToLatestVersion()
	{
		Version = FSoWorldStateVersion::LatestVersion;
	}

	bool operator==(const FSoStateTable& Other) const;

	FORCEINLINE bool operator!=(const FSoStateTable& Other) const
	{
		return !(*this == Other);
	}

	FString GetChecksum() const;

	static bool DecodeChecksum(const FString& RawChecksum, FString& OutCheckSum, int32& OutTotalPlaytimeSeconds);
public:
	// Version of the save
	UPROPERTY(VisibleAnywhere)
	int32 Version = FSoWorldStateVersion::LatestVersion;

	// Metadata for this save slot
	UPROPERTY(VisibleAnywhere)
	FSoStateTableMetadata Metadata;

	// Key: LevelName
	// Value: Actors in Level
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FSoStateEntries> Entries;

	// Key: LevelName
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FSoStringSet> StringSets;

	// Key: Dialogue Unique identifier
	UPROPERTY(VisibleAnywhere)
	TMap<FGuid, FDlgHistory> DlgHistoryMap;

	// Holds all the item sets
	// Key: ItemSetKey
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FSoItemArray> ItemMap;

	UPROPERTY(VisibleAnywhere)
	FSoPlayerData PlayerData;

	static const FString ChecksumSeparator;
};
