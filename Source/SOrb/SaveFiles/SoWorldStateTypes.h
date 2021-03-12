// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Misc/SecureHash.h"

#include "Items/SoItemTypes.h"
#include "Items/SoItem.h"

#include "SoWorldStateTypes.generated.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Storage structs
// Are created so that we can save to text file as we can't directly save TMap/TArray, etc

// Keep track of version in save files
struct FSoWorldStateVersion
{
	enum Type
	{
		Initial = 0,
		MovedMetadataToSaveFiles,
		RedesignedCombatSystem,
		AddedEpisode,
		ModifiedALotOfStuffBreakingChanges,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
};


// Each actor can have his own FSoStateEntry to store int/float/fvector values
USTRUCT()
struct FSoStateEntry
{
	GENERATED_USTRUCT_BODY()
public:
	bool operator==(const FSoStateEntry& Other) const;
	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Key: VariableName
	UPROPERTY(VisibleAnywhere)
	TMap<FName, int32> Ints;

	// Key: VariableName
	UPROPERTY(VisibleAnywhere)
	TMap<FName, float> Floats;

	// Key: VariableName
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FVector> Vectors;
};


USTRUCT()
struct FSoStateEntries
{
	GENERATED_USTRUCT_BODY()
public:
	bool operator==(const FSoStateEntries& Other) const;
	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Key: ActorName
	// Valee: Variables for this actor
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FSoStateEntry> Entries;
};

USTRUCT()
struct FSoStringSet
{
	GENERATED_USTRUCT_BODY()
public:
	bool operator==(const FSoStringSet& Other) const;
	void UpdateSHA1(FSHA1& HashState) const;

public:
	// each level has a list of strings here
	// actors can add/remove strings or check their existence
	UPROPERTY(VisibleAnywhere, Meta = (DlgLinePerItem))
	TSet<FName> Strings;
};


USTRUCT()
struct FSoNameSet
{
	GENERATED_USTRUCT_BODY()
public:
	bool operator==(const FSoNameSet& Other) const;
	void UpdateSHA1(FSHA1& HashState) const;

public:
	UPROPERTY(VisibleAnywhere, Meta = (DlgLinePerItem))
	TSet<FName> Names;
};


USTRUCT()
struct FSoItemArray
{
	GENERATED_USTRUCT_BODY()
public:
	FORCEINLINE void CheckAndFixIntegrity()
	{
		// TODO maybe
	}

	FORCEINLINE bool operator==(const FSoItemArray& Other) const
	{
		return Items == Other.Items;
	}
	void UpdateSHA1(FSHA1& HashState) const;

public:
	UPROPERTY()
	TArray<FSoItem> Items;
};


/** Player specific stuffs: */
USTRUCT()
struct FSoPlayerData
{
	GENERATED_USTRUCT_BODY()
public:
	bool operator==(const FSoPlayerData& Other) const;
	void UpdateSHA1(FSHA1& HashState) const;

public:
	// Key: VariableName
	UPROPERTY()
	TMap<FName, int32> Ints;

	// Key: VariableName
	UPROPERTY()
	TMap<FName, float> Floats;

	// All bool variables are in this set, the bool variables not in this set are considered false by default
	UPROPERTY()
	TSet<FName> TrueBools;

	// Key: VariableName
	UPROPERTY()
	TMap<FName, FName> Names;
};


// Holds the metadata for the game, like the current SlotIndex
//
// Chapter saves:
// - SlotIndex - represents the current  slot index for chapter the save
//
// Episodes saves:
//- SlotIndex - represents the current episodfe
//-
USTRUCT()
struct FSoStateMetadata
{
	GENERATED_USTRUCT_BODY()
public:
	bool CheckAndFixIntegrity();

	FORCEINLINE void UpdateToLatestVersion()
	{
		Version = FSoWorldStateVersion::LatestVersion;
	}

public:
	// Version of the save
	UPROPERTY()
	int32 Version = FSoWorldStateVersion::LatestVersion;

	// Last chapter save slot index used. This is the one that will be loaded on game restart by default.
	UPROPERTY()
	int32 ChapterSlotIndex = 0;

	// Tells us about the last used episode name from the EpisodesArray
	UPROPERTY()
	FName EpisodeName = NAME_None;

	// Last episode save slot index used.
	// TODO use it
	UPROPERTY()
	int32 EpisodeSlotIndex = 0;
};
