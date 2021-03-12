// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Object.h"

#include "Items/SoItemTypes.h"
#include "HAL/FileManager.h"
#include "SoWorldStateTypes.h"
#include "SoWorldStateTable.h"
#include "SoWorldStateEpisodeTypes.h"
#include "Online/SoOnlineCloud.h"

struct FSoStateMetadataSaveSlotEntry;

DECLARE_LOG_CATEGORY_EXTERN(LogSoWorldState, All, All);

enum class ESoSaveStatus : uint8
{
	Success = 0,

	InvalidSaveIndex,
	SaveFileMissing,
	SaveFileFailedParsing,
	SaveFailedToSave,
	SaveFailedToDelete,

	// Uses default file
	SaveFailedToLoad_UsingDefault,

	MetadataFileMissing,
	MetadataFileFailedParsing,
	MetadataFailedToSave,

	// WTH?
	LockedGameCompleted,

	UnknownError
};

/**
 * Singleton class storing the state values for save/load and level streaming
 * USoWorldState should offer all functionality, most likely there is no need to use this class directly
 */
class FSoWorldState
{
public:
	// Get singleton
	static FSoWorldState& Get();

	// Forbid copy and assign
	FSoWorldState(const FSoWorldState& Other) = delete;
	void operator=(const FSoWorldState& Other) = delete;

	//
	// Metadata and save slot files
	//

	FSoOnlineCloud& GetCloudSavesManager() { return CloudSaves; }

	FORCEINLINE void ReloadGameMetaData()
	{
		LoadGameMetadata();
	}


	// WorldState for Episode or chapter?
	bool IsForEpisode() const { return bForEpisodeState; }
	bool IsForChapter() const { return !bForEpisodeState; }
	void UseForEpisode(int32 SlotIndex, FName EpisodeName)
	{
		bForEpisodeState = true;
		SetSlotIndex(SlotIndex, false);
		Metadata.EpisodeName = EpisodeName;
		SaveGameMetadata();
	}
	void UseForChapter(int32 SlotIndex)
	{
		bForEpisodeState = false;
		SetSlotIndex(SlotIndex, false);
	}

	// Gets the current used save slot. This is updates if SaveGameForChapter/LoadGameForChapter is successful.
	int32 GetSlotIndex() const { return IsForChapter() ? Metadata.ChapterSlotIndex : Metadata.EpisodeSlotIndex; }
	void SetSlotIndex(int32 Index, bool bSaveMetadata = true)
	{
		if (IsForChapter())
			Metadata.ChapterSlotIndex = Index;
		else
			Metadata.EpisodeSlotIndex = Index;

		if (bSaveMetadata)
			SaveGameMetadata();
	}

	// Used for consoles
	int32 GetUserIndex() const { return UserIndex; }
	void SetUserIndex(int32 Index) { UserIndex = Index; }

	// Helper methods to save/load the current save slot index
	ESoSaveStatus SaveGame() { return SaveGameToSlot(GetSlotIndex()); }
	ESoSaveStatus LoadGame() { return LoadGameFromSlot(GetSlotIndex()); }

	// Exists?
	bool DoesSaveExist() const { return DoesSaveExistForSlot(GetSlotIndex()); }
	bool DoesSaveExistForSlot(int32 SlotIndex) const;

	// Paths
	FString GetSaveFilePath() const { return GetSaveFilePathFromSlot(GetSlotIndex()); }
	FString GetSaveFileName() const { return GetSaveFileNameFromSlot(GetSlotIndex()); }
	FString GetSaveFilePathFromSlot(int32 SlotIndex) const;
	FString GetSaveFileNameFromSlot(int32 SlotIndex) const;


	// Metadata EquippedItems
	const TArray<USoItemTemplate*>& GetMetadataEquippedItems() const { return GetStateTableMetadata().EquippedItems; }
	void SetMetadataEquippedItems(const TArray<USoItemTemplate*>& EquippedItems)
	{
		GetStateTableMetadata().EquippedItems = EquippedItems;
	}

	FName GetMapName() const { return GetStateTableMetadata().MapName; }
	bool IsGameCompleted() const
	{
		return IsStateTableAGameCompleted(GetStateTable());
	}

	static bool IsGameFinished(FName ProgressData)
	{
		return ProgressData == FinishedProgressSeparate || ProgressData == FinishedProgressSave;
	}

	// SpellsCapacity
	int32 GetInitialSpellsCapacity() const { return GetStateTableMetadata().InitialSpellsCapacity; }
	void SetInitialSpellsCapacity(int32 Value)
	{
		GetStateTableMetadata().InitialSpellsCapacity = Value;
	}

	// HP
	int32 GetInitialMaxHealth() const { return GetStateTableMetadata().InitialMaxHealth; }
	void SetInitialMaxHealth(int32 Value)
	{
		GetStateTableMetadata().InitialMaxHealth = Value;
	}

	// Progress Stats
	const FSoPlayerProgressStats& GetProgressStats() const { return GetStateTableMetadata().ProgressStats; }
	void SetProgressStats(const FSoPlayerProgressStats& InStats) { GetStateTableMetadata().ProgressStats = InStats; }


	// CheckpointLocationName
	bool IsCheckpointLocationDefault() const { return GetCheckpointLocation() == NAME_None; }
	FName GetCheckpointLocation() const { return GetStateTableMetadata().CheckpointLocationName; }
	void SetCheckpointLocation(FName CheckpointLocation);
	void SetDisplayedProgressName(FName DisplayedProgressName);

	void SetMapName(FName MapName);

	// Is slot at SlotIndex empty? Note this only checks the metadata
	bool IsEmpty() const { return GetStateTableMetadata().IsEmpty(); }

	// Clears the data from the current slot
	// NOTE: does not affect if this is for chapter or episode or the slot index or anything like this
	void ResetToDefault()
	{
		GetStateTable() = {};
		CloudSaves = {};
		ResetEpisodeAndChapterDataToDefault();
	}

	// Clears the data from the current slot and then sets the difficulty based on input
	// NOTE: does not affect if this is for chapter or episode or the slot index or anything like this
	void ResetToDefault(ESoDifficulty Difficulty)
	{
		GetStateTable() = {};
		GetStateTable().Metadata.Difficulty = Difficulty;
		GetStateTable().Metadata.DisplayedProgressName = FName("act1_start");
		CloudSaves = {};
		ResetEpisodeAndChapterDataToDefault();
	}

	ESoDifficulty GetGameDifficulty() const
	{
		return GetStateTable().Metadata.Difficulty;
	}

	void SetGameDifficulty(ESoDifficulty Difficulty)
	{
		GetStateTable().Metadata.Difficulty = Difficulty;
	}

	// Save game, updates the current slot index to be SlotIndex
	// Note: Modifies current slot index
	ESoSaveStatus SaveGameToSlot(int32 SlotIndex);

	/**
	 *  Deletes the game at SlotIndex
	 * 1. Saves the values for the SlotIndex and
	 *	Sets the metadata values in memory to empty
	 * 2. Deletes the save slot file and creates a backup file with all the data
	 */
	ESoSaveStatus DeleteGameInSlot(int32 SlotIndex);

	// Load from the save game. Updates the current slot index to be SlotIndex
	// NOTE: if this loads successfully the metadata current save slot index is updated.
	ESoSaveStatus LoadGameFromSlot(int32 SlotIndex);

	// For Delete, should we backup?
	void SetAutoBackupDeletedSaves(bool bBackup) { bAutoBackupDeletedSaves = bBackup; }
	void SetAutoBackupBeforeSave(bool bBackup) { bAutoBackupBeforeSave = bBackup; }

	//
	// Episodes file saves
	//

	FName GetEpisodeName() const { return Metadata.EpisodeName; }

	//
	// Helper static methods
	//

	// Was it a success or a failure, but we consider loading the default as success
	static FORCEINLINE bool IsSaveStatusSuccessful(ESoSaveStatus Status)
	{
		return Status == ESoSaveStatus::Success || Status == ESoSaveStatus::SaveFailedToLoad_UsingDefault
			|| Status == ESoSaveStatus::LockedGameCompleted;
	}

	static void LogSaveStatusIfError(const ESoSaveStatus Status, const FString& Context);

	// Gets all the save slots file paths.
	// returns:
	//		Key: SlotIndex
	//		Value: Absolute file system path
	TArray<int32> GetAllSavesSlotFilesPaths();

	// This save table is marked as a completed game
	static bool IsStateTableAGameCompleted(const FSoStateTable& StateTable);

	// Gets all the save slots from the file system
	bool GetAllSavesSlots(TMap<int32, FSoStateTable>& AllSlotsMetadata);

	//
	// Manipulate StateTable
	//

	// check StringSets variable
	void AddNameToSet(FName LevelName, FName Name);
	void RemoveNameFromSet(FName LevelName, FName Name);
	bool IsNameInSet(FName LevelName, FName Name) const;

	// reads value if there is any entry for the given name
	// if there is no value in the map reference is not modified - it is possible if the element was not saved before
	// bool and int is stored as int, (ValueName has to be unique inside level/actor/valuetype map)
	bool ReadIntValue(FName LevelName, FName EntryName, FName ValueName, int32& Value);
	void WriteIntValue(FName LevelName, FName EntryName, FName ValueName, int32 Value);

	bool ReadBoolValue(FName LevelName, FName EntryName, FName ValueName, bool& bValue);
	void WriteBoolValue(FName LevelName, FName EntryName, FName ValueName, bool bValue);

	bool ReadFloatValue(FName LevelName, FName EntryName, FName ValueName, float& Value);
	void WriteFloatValue(FName LevelName, FName EntryName, FName ValueName, float Value);

	bool ReadVectorValue(FName LevelName, FName EntryName, FName ValueName, FVector& Value);
	void WriteVectorValue(FName LevelName, FName EntryName, FName ValueName, const FVector& Value);

	// Get/Read/Write items to the StateTable
	TArray<FSoItem>& GetItemList(FName ListName);
	bool ReadItemList(FName ListName, TArray<FSoItem>& OutList) const;
	void WriteItemList(FName ListName, const TArray<FSoItem>& Items);

	void WriteEquippedSpellItemList(const TArray<FSoItem>& Items) { WriteItemList(EquippedSpellListName, Items); }
	bool ReadEquippedSpellItemList(TArray<FSoItem>& OutList) const { return ReadItemList(EquippedSpellListName, OutList); }

	void WriteEquippedItemList(const TArray<FSoItem>& Items) { WriteItemList(EquippedItemListName, Items); }
	bool ReadEquippedItemList(TArray<FSoItem>& OutList) const { return ReadItemList(EquippedItemListName, OutList); }

	void WriteInventoryItemList(const TArray<FSoItem>& Items) { WriteItemList(InventoryItemListName, Items); }
	bool ReadInventoryItemList(TArray<FSoItem>& OutList) const { return ReadItemList(InventoryItemListName, OutList); }

	// Get/Set Dialogue history
	void SetDlgHistory(const TMap<FGuid, FDlgHistory>& HistoryMap) { GetStateTable().DlgHistoryMap = HistoryMap; }
	const TMap<FGuid, FDlgHistory>& GetDlgHistory() const { return GetStateTable().DlgHistoryMap; }

	// Player related stuff:
	const FSoPlayerData& GetPlayerData() const { return GetStateTable().PlayerData; }
	void SetPlayerData(const FSoPlayerData& PlayerData) { GetStateTable().PlayerData = PlayerData; }


private:
	FSoWorldState() {};

	FSoStateTableMetadata& GetStateTableMetadata() const { return GetStateTable().Metadata; }
	FSoStateTable& GetStateTable() const;

	void ResetMapNameToDefault();
	void ResetEpisodeAndChapterDataToDefault();

	FString GetPrefixEpisodeFileName() const
	{
		return FString::Printf(TEXT("%s_%s"), *EpisodeBaseFileName, *Metadata.EpisodeName.ToString());
	}

	FORCEINLINE static bool CheckAndValidateStringSetInputs(FName LevelName, FName Name, const TCHAR* Context)
	{
		if (LevelName.IsNone())
		{
			UE_LOG(LogSoWorldState, Error, TEXT("Got LevelName = `None` and Name = `%s` with Context = `%s`. Ignoring"), *Name.ToString(), Context);
			return false;
		}

		if (Name.IsNone())
		{
			UE_LOG(LogSoWorldState, Error, TEXT("Got Name = `None` in LevelName = `%s` with Context = `%s`. Ignoring"), *LevelName.ToString(), Context);
			return false;
		}

		if (!Name.IsValidIndexFast())
		{
			UE_LOG(LogSoWorldState, Error, TEXT("Got Name.IsValidIndexFast() == false in LevelName = `%s` with Context = `%s`. Ignoring"), *LevelName.ToString(), Context);
			return false;
		}

		return true;
	}

	// Helper methods:
	static FSoStateEntry* GetEntryFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, bool bCreate = false);
	static int32* GetIntFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, FName ValueName, bool bCreate = false);
	static float* GetFloatFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, FName ValueName, bool bCreate = false);
	static FVector* GetVectorFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, FName ValueName, bool bCreate = false);

	// Validate SlotIndex is valid, ouput error message if it is not
	static bool VerifyValidSlotIndex(int32 SlotIndex, const TCHAR* ContextMessage);

	// Save the Metadata to file.
	ESoSaveStatus SaveGameMetadata();

	// Loads the Metadata from file
	ESoSaveStatus LoadGameMetadata();

	// Loads the state table from the file system
	ESoSaveStatus LoadGameStateTableForSlotIntoData(int32 SlotIndex, FSoStateTable& OutStateTable);

private:
	// Represents metadata for the save files
	FSoStateMetadata Metadata;

	// Our head is in the clouds
	FSoOnlineCloud CloudSaves;

	// For some platforms, master user index to identify the user doing the saving.
#if PLATFORM_XBOXONE
	int32 UserIndex = -1;
#else
	int32 UserIndex = 0;
#endif

	// Current World is for a chapter or episode?
	bool bForEpisodeState = false;

	// Was the metadata file loaded at least once?
	// Lazy set
	bool bMetadataLoadedOnce = false;

	// Was the data reset to default
	// Lazy set
	bool bInitializedOnce = false;

	// Should auto backup deleted saves
	bool bAutoBackupDeletedSaves = true;

	// Should we backup saves before saving.
	bool bAutoBackupBeforeSave = true;

	// Default slot index used if anything else fails.
	static constexpr int32 DefaultSlotIndex = 0;

	// The metadata file name
	static const FString MetadataFileName;

	// The chapter file name
	static const FString ChapterFileName;

	// The episodes file name
	static const FString EpisodeBaseFileName;
	static const FString EpisodesFileName;

	// Set in Gatekeeper_AfterBattle
	static const FName Chapter1CompletedName;
	static const FName Chapter2CompletedName;

	// set when the player chooses the ending
	static const FName FinishedProgressSave;
	static const FName FinishedProgressSeparate;

	// Item lists
	static const FName InventoryItemListName;
	static const FName EquippedItemListName;
	static const FName EquippedSpellListName;
};
