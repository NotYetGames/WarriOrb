// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoWorldState.h"

#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "Misc/Paths.h"

#include "Levels/SoLevelHelper.h"
#include "Basic/SoConsoleCommands.h"
#include "SoSaveWriter.h"
#include "SoSaveParser.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Basic/SoGameSingleton.h"
#include "SoSaveHelper.h"

DEFINE_LOG_CATEGORY(LogSoWorldState);

// Some class constants

const FName FSoWorldState::FinishedProgressSave(TEXT("epilogue_save_creatures"));
const FName FSoWorldState::FinishedProgressSeparate(TEXT("epilogue_separate_worlds"));

const FString FSoWorldState::MetadataFileName(TEXT("METADATA"));
const FString FSoWorldState::EpisodeBaseFileName(TEXT("EPISODE"));
const FString FSoWorldState::EpisodesFileName(TEXT("EPISODES"));
const FString FSoWorldState::ChapterFileName(TEXT("SAVE"));
const FName FSoWorldState::Chapter1CompletedName(TEXT("Chapter1Completed"));
const FName FSoWorldState::Chapter2CompletedName(TEXT("Chapter2Completed"));

const FName FSoWorldState::InventoryItemListName(TEXT("Warriorb_ItemList"));
const FName FSoWorldState::EquippedItemListName(TEXT("Warriorb_Equipped"));
const FName FSoWorldState::EquippedSpellListName(TEXT("Warriorb_EquippedSpells"));


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoWorldState& FSoWorldState::Get()
{
	static FSoWorldState Instance;

	// For debug stuff
	//auto& StateTable = Instance.GetStateTable();

	// Lazy load the valid data the first time
	if (!Instance.bMetadataLoadedOnce)
	{
		if (Instance.LoadGameMetadata() != ESoSaveStatus::Success)
		{
			UE_LOG(LogSoWorldState, Error, TEXT("FSoWorldState::Get() FAILED to load the METADATA file"));
		}
	}
	if (!Instance.bInitializedOnce)
	{
		Instance.ResetToDefault();
		Instance.bInitializedOnce = true;
	}

	return Instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoWorldState::GetSaveFilePathFromSlot(int32 SlotIndex) const
{
	if (IsForChapter())
		return FSoSaveHelper::GetSaveFilePathFromSlot(ChapterFileName, SlotIndex);

	return FSoSaveHelper::GetSaveFilePathFromSlot(GetPrefixEpisodeFileName(), SlotIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoWorldState::GetSaveFileNameFromSlot(int32 SlotIndex) const
{
	if (IsForChapter())
		return FSoSaveHelper::GetSaveFileNameFromSlot(ChapterFileName, SlotIndex);

	return FSoSaveHelper::GetSaveFileNameFromSlot(GetPrefixEpisodeFileName(), SlotIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::DoesSaveExistForSlot(int32 SlotIndex) const
{
	return FSoSaveHelper::DoesSaveFileNameExist(GetSaveFileNameFromSlot(SlotIndex), UserIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::ResetEpisodeAndChapterDataToDefault()
{
	ResetMapNameToDefault();
	Metadata.CheckAndFixIntegrity();
	SaveGameMetadata();

	if (IsForEpisode())
	{
		FSoEpisodeMapParams EpisodeData;
		verify(USoLevelHelper::GetEpisodeData(GetEpisodeName(), EpisodeData));
		SetCheckpointLocation(EpisodeData.CheckpointLocation);
		WriteInventoryItemList(EpisodeData.EnterInventoryItems);
		WriteEquippedItemList(EpisodeData.EnterEquippedItems);
		WriteEquippedSpellItemList(EpisodeData.EnterEquippedSpells);
		SetInitialMaxHealth(EpisodeData.MaxHealth);
		SetInitialSpellsCapacity(EpisodeData.SpellsCapacity);
	}
	else
	{
		// Chapter
		SetInitialMaxHealth(30);
		SetInitialSpellsCapacity(2);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::ResetMapNameToDefault()
{
	if (IsForChapter())
	{
		// Chapter
		GetStateTableMetadata().MapName = USoLevelHelper::GetFirstChapterName();
	}
	else
	{
		// Episode
		GetStateTableMetadata().MapName = USoLevelHelper::GetFirstValidEpisodeName();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::SetCheckpointLocation(FName CheckpointLocation)
{
	GetStateTableMetadata().CheckpointLocationName = CheckpointLocation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::SetDisplayedProgressName(FName DisplayedProgressName)
{
	GetStateTableMetadata().DisplayedProgressName = DisplayedProgressName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::SetMapName(FName MapName)
{
	if (USoLevelHelper::IsValidMapName(MapName))
	{
		GetStateTableMetadata().MapName = MapName;
	}
	else
	{
		ResetMapNameToDefault();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSaveStatus FSoWorldState::SaveGameMetadata()
{
	UE_LOG(LogSoWorldState, Verbose, TEXT("SAVE Metadata"));

	// Can happen on editor Builds
#if WARRIORB_NON_EDITOR_TEST
	check(bMetadataLoadedOnce);
#endif

	Metadata.UpdateToLatestVersion();
	FSoSaveWriter SaveWriter;
	SaveWriter.Write(FSoStateMetadata::StaticStruct(), &Metadata);

	return FSoSaveHelper::SaveToFile(SaveWriter, MetadataFileName, UserIndex) ? ESoSaveStatus::Success : ESoSaveStatus::MetadataFailedToSave;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSaveStatus FSoWorldState::SaveGameToSlot(int32 SlotIndex)
{
	UE_LOG(LogSoWorldState, Verbose, TEXT("SAVE Save Slot at index = %d"), SlotIndex);
	if (!VerifyValidSlotIndex(SlotIndex, TEXT("SAVE save slot")))
		return ESoSaveStatus::InvalidSaveIndex;

	// TODO fix this
	//if (IsGameSlotACompletedGame(StateTable))
	//{
	//	UE_LOG(LogSoWorldState, Warning, TEXT("Can't save because this save slot = %d, is marked as a completed game (good job)."), SlotIndex);
	//	return ESoSaveStatus::LockedGameCompleted;
	//}

#if !PLATFORM_XBOXONE && !PLATFORM_SWITCH
	// Create backup of file
	if (bAutoBackupBeforeSave)
	{
		FSoSaveHelper::BackupSaveFile(GetSaveFileNameFromSlot(SlotIndex), TEXT("BEFORE_SAVE.backup"), UserIndex);
	}
#endif
	FSoStateTable& StateTable = GetStateTable();

	// Modify Current
	// Assume that once saved, this is the current index.
	StateTable.UpdateToLatestVersion();
	SetSlotIndex(SlotIndex);

	// Update Save time and game version
	StateTable.Metadata.SetSaveTimeToNow();
	StateTable.Metadata.GameBuildVersion = USoPlatformHelper::GetGameBuildVersion();
	StateTable.Metadata.GameBuildBranch = USoPlatformHelper::GetGameBuildBranch();
	StateTable.Metadata.GameBuildCommit = USoPlatformHelper::GetGameBuildCommit();

	// Fix integrity before
	StateTable.CheckAndFixIntegrity();

	// Calculate Checksum
	if (FSoConsoleCommands::IsSpeedRunCompetitionMode())
	{
		StateTable.Metadata.Checksum = StateTable.GetChecksum();
	}
	else
	{
		StateTable.Metadata.Checksum = TEXT("");
	}

	FSoSaveWriter SaveWriter;
	SaveWriter.Write(FSoStateTable::StaticStruct(), &StateTable);
	if (FSoSaveHelper::SaveToFile(SaveWriter, GetSaveFileNameFromSlot(SlotIndex), UserIndex))
	{
		// Validate if it saved correctly by loading again from the file
#if WARRIORB_NON_EDITOR_TEST
		FSoStateTable SlotLoadStateTable;
		if (LoadGameStateTableForSlotIntoData(SlotIndex, SlotLoadStateTable) == ESoSaveStatus::Success)
		{
			if (SlotLoadStateTable != StateTable)
			{
				checkNoEntry();
			}
		}
#endif

		return ESoSaveStatus::Success;
	}

	return ESoSaveStatus::SaveFailedToSave;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSaveStatus FSoWorldState::DeleteGameInSlot(int32 SlotIndex)
{
	UE_LOG(LogSoWorldState, Verbose, TEXT("DELETE Save Slot at index = %d"), SlotIndex);
	if (!VerifyValidSlotIndex(SlotIndex, TEXT("DELETE save slot")))
		return ESoSaveStatus::InvalidSaveIndex;

	const bool bFileExists = DoesSaveExistForSlot(SlotIndex);

	// Backup the save slot
	bool bStatus = true;
	if (bFileExists && bAutoBackupDeletedSaves)
	{
		FSoStateTable DeletedSave;

		// Step 1. Save the StateTable for the SlotIndex
		if (SlotIndex == GetSlotIndex())
		{
			// From Memory
			DeletedSave = GetStateTable();
		}
		else
		{
			// Load it from disk
			LoadGameStateTableForSlotIntoData(SlotIndex, DeletedSave);
		}

		// Backup
		FString DeletedFileNameNoExt;
		if (FSoSaveHelper::GetDeletedSaveFileNameFromFileName(GetSaveFileNameFromSlot(SlotIndex), UserIndex, DeletedFileNameNoExt))
		{
			FSoSaveWriter SaveWriter;
			SaveWriter.Write(FSoStateTable::StaticStruct(), &DeletedSave);
			bStatus &= FSoSaveHelper::SaveToFile(SaveWriter, DeletedFileNameNoExt, UserIndex);
		}
		UE_LOG(LogSoWorldState, Verbose, TEXT("Creating BACKUP for Deleted Save slot of index = %d to file = `%s`"), SlotIndex, *DeletedFileNameNoExt);
	}

	if (SlotIndex == GetSlotIndex())
		ResetToDefault();

	// Do not need to delete anything
	if (!bFileExists)
	{
		UE_LOG(LogSoWorldState, Verbose, TEXT("File missing for slot index = %d. Not deleting it *shrug*"), SlotIndex);
		return ESoSaveStatus::SaveFileMissing;
	}

	// Delete Save slot
	bStatus &= FSoSaveHelper::DeleteSaveFile(GetSaveFileNameFromSlot(SlotIndex), UserIndex);
	return bStatus ? ESoSaveStatus::Success : ESoSaveStatus::SaveFailedToDelete;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSaveStatus FSoWorldState::LoadGameMetadata()
{
	UE_LOG(LogSoWorldState, Verbose, TEXT("LOAD Metadata"));

	// probably xbox and the user is not signed in yet / atm
	if (UserIndex == -1)
	{
		return ESoSaveStatus::MetadataFileFailedParsing;
	}

	// Does not matter if the file does not exist, we tried
	bMetadataLoadedOnce = true;
	if (!FSoSaveHelper::DoesSaveFileNameExist(MetadataFileName, UserIndex))
	{
		// Most likely first time loading, set the save time to now
		UE_LOG(
			LogSoWorldState,
			Verbose,
			TEXT("Tried to LOAD Metadata. But file = `%s` does not exist. Creating it for the first ime"),
			*FSoSaveHelper::GetSaveFilePathFromFileName(MetadataFileName)
		);
		Metadata = {};
		Metadata.CheckAndFixIntegrity();
		return SaveGameMetadata();
	}

	FSoSaveParser Parser;
	if (FSoSaveHelper::LoadFromFile(Parser, MetadataFileName, UserIndex) && Parser.IsValidFile())
	{
		// Clear Current metadata
		Metadata = {};
		Parser.ReadAllProperty(FSoStateMetadata::StaticStruct(), &Metadata, nullptr);
		return Metadata.CheckAndFixIntegrity() ? SaveGameMetadata() : ESoSaveStatus::Success;
	}

	return ESoSaveStatus::MetadataFileFailedParsing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSaveStatus FSoWorldState::LoadGameFromSlot(int32 SlotIndex)
{
	UE_LOG(LogSoWorldState, Verbose, TEXT("LOAD Save Slot at index = %d"), SlotIndex);
	if (!VerifyValidSlotIndex(SlotIndex, TEXT("LOAD save slot")))
		return ESoSaveStatus::InvalidSaveIndex;

	ResetToDefault();
	if (!DoesSaveExistForSlot(SlotIndex))
	{
		UE_LOG(LogSoWorldState, Warning, TEXT("SlotIndex = %d save file does not exist, loading empty."), SlotIndex);
		SetSlotIndex(SlotIndex);
		return ESoSaveStatus::Success;
	}

	// Load save
	FSoStateTable& StateTable = GetStateTable();
	if (LoadGameStateTableForSlotIntoData(SlotIndex, StateTable) == ESoSaveStatus::Success)
	{
		SetSlotIndex(SlotIndex);
		return ESoSaveStatus::Success;
	}

	// Invalidate, Load default
	UE_LOG(LogSoWorldState, Warning, TEXT("Invalidating current slot index, setting to default SlotIndex = %d"), DefaultSlotIndex);
	SetSlotIndex(DefaultSlotIndex);
	ResetToDefault();

	// Don't care about failure/success, if it is empty we use it, if it is not we simply load it from there
	if (DoesSaveExistForSlot(SlotIndex))
		LoadGameStateTableForSlotIntoData(DefaultSlotIndex, StateTable);

	return ESoSaveStatus::SaveFailedToLoad_UsingDefault;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::LogSaveStatusIfError(const ESoSaveStatus Status, const FString& Context)
{
	if (IsSaveStatusSuccessful(Status))
		return;

	FString Message;
	switch (Status)
	{
	case ESoSaveStatus::InvalidSaveIndex:
		Message = TEXT("Save slot index is not in the valid range (most likely negative)");
		break;
	case ESoSaveStatus::SaveFileMissing:
		Message = TEXT("Save slot file does not exist (missing on the filesystem)");
		break;
	case ESoSaveStatus::SaveFileFailedParsing:
		Message = TEXT("Failed to parse the save slot file (most likely something is corrupt)");
		break;
	case ESoSaveStatus::SaveFailedToSave:
		Message = TEXT("SaveGameForChapter failed");
		break;
	case ESoSaveStatus::SaveFailedToDelete:
		Message = TEXT("DeleteGameInSlot failed");
		break;

	case ESoSaveStatus::MetadataFileMissing:
		Message = TEXT("Metadata file does not exist (missing on the filesystem)");
		break;
	case ESoSaveStatus::MetadataFileFailedParsing:
		Message = TEXT("Failed to parse metadata file (most likely something is corrupt)");
		break;
	case ESoSaveStatus::MetadataFailedToSave:
		Message = TEXT("SaveGameMetadata Failed");
		break;

	default:
		Message = FString::Printf(TEXT("Unhandled ESoSaveStatus = %d"), static_cast<int32>(Status));
		break;
	}

	UE_LOG(LogSoWorldState, Error, TEXT("Got ESoSaveStatus with a non success code. Message = `%s`, Context = `%s`"), *Message, *Context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<int32> FSoWorldState::GetAllSavesSlotFilesPaths()
{
	TArray<int32> SaveSlotsFilePaths;

	for (int32 i = 0; i < 8; ++i)
	{
		if (DoesSaveExistForSlot(i))
		{
			SaveSlotsFilePaths.Add(i);
		}
	}

	return SaveSlotsFilePaths;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::IsStateTableAGameCompleted(const FSoStateTable& StateTable)
{
	// TODO fix this for episodes
	return IsGameFinished(StateTable.Metadata.DisplayedProgressName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::GetAllSavesSlots(TMap<int32, FSoStateTable>& AllSlotsMetadata)
{
	bool bStatus = true;
	AllSlotsMetadata.Empty();

	// Load every save slot file
	TArray<int32> AllSaveSlotsFilePaths = GetAllSavesSlotFilesPaths();
	for (const int32 SaveSlotIndex : AllSaveSlotsFilePaths)
	{
		FSoStateTable SlotStateTable;
		if (LoadGameStateTableForSlotIntoData(SaveSlotIndex, SlotStateTable) == ESoSaveStatus::Success)
		{
			// NOTE: Duplicates should not exist here, filtered in GetAllSavesSlotFilesPaths
			AllSlotsMetadata.Add(SaveSlotIndex, SlotStateTable);
		}
		else
		{
			bStatus = false;
		}
	}

	return bStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::VerifyValidSlotIndex(int32 SlotIndex, const TCHAR* ContextMessage)
{
	if (SlotIndex < 0)
	{
		UE_LOG(LogSoWorldState, Warning, TEXT("Invalid Negative SlotIndex = %d. Context = %s"), ContextMessage);
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoSaveStatus FSoWorldState::LoadGameStateTableForSlotIntoData(int32 SlotIndex, FSoStateTable& OutStateTable)
{
	// Not on file system
	if (!DoesSaveExistForSlot(SlotIndex))
	{
		UE_LOG(LogSoWorldState, Warning, TEXT("Tried to LOAD Save Slot at Index = %d. But file = `%s` does not exist"), SlotIndex, *GetSaveFilePathFromSlot(SlotIndex));
		OutStateTable.CheckAndFixIntegrity();
		return ESoSaveStatus::SaveFileMissing;
	}

	// Load the save file
	FSoSaveParser Parser;
	if (!FSoSaveHelper::LoadFromFile(Parser, GetSaveFileNameFromSlot(SlotIndex), UserIndex) || !Parser.IsValidFile())
	{
		UE_LOG(LogSoWorldState, Warning, TEXT("Tried to PARSE Save Slot at Index = %d. But file = `%s`is not valid"), SlotIndex, *GetSaveFilePathFromSlot(SlotIndex));
		OutStateTable.CheckAndFixIntegrity();
		return ESoSaveStatus::SaveFileFailedParsing;
	}

	// TODO file can still be invalid
	Parser.ReadAllProperty(FSoStateTable::StaticStruct(), &OutStateTable, nullptr);
	OutStateTable.CheckAndFixIntegrity();

	// Speed run competition?
	if (FSoConsoleCommands::IsSpeedRunCompetitionMode())
	{
		// Most likely the checksum failed, reset everything
		if (OutStateTable.CheckAndFixIntegrityFromChecksum())
		{
			UE_LOG(LogSoWorldState, Warning, TEXT("CheckAndFixIntegrityFromChecksum is telling us to reset everything. Are you a cheater?"));
			OutStateTable = {};
		}
	}

	return ESoSaveStatus::Success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::AddNameToSet(FName LevelName, FName ValueName)
{
	if (!CheckAndValidateStringSetInputs(LevelName, ValueName, TEXT("AddNameToSet")))
		return;
	FSoStateTable& StateTable = GetStateTable();
	FSoStringSet* Set = StateTable.StringSets.Find(LevelName);
	if (Set == nullptr)
		Set = &StateTable.StringSets.Add(LevelName);

	Set->Strings.Add(ValueName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::RemoveNameFromSet(FName LevelName, FName ValueName)
{
	if (!CheckAndValidateStringSetInputs(LevelName, ValueName, TEXT("RemoveNameFromSet")))
		return;

	FSoStateTable& StateTable = GetStateTable();
	FSoStringSet* Set = StateTable.StringSets.Find(LevelName);
	if (Set == nullptr)
		return;

	Set->Strings.Remove(ValueName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::IsNameInSet(FName LevelName, FName ValueName) const
{
	CheckAndValidateStringSetInputs(LevelName, ValueName, TEXT("IsNameInSet. Not actually ignoring."));
	FSoStateTable& StateTable = GetStateTable();
	const FSoStringSet* Set = StateTable.StringSets.Find(LevelName);
	if (Set == nullptr)
		return false;

	return Set->Strings.Contains(ValueName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::ReadIntValue(FName LevelName, FName EntryName, FName ValueName, int32& Value)
{
	FSoStateTable& StateTable = GetStateTable();
	const int32* StoredValue = GetIntFromStateTable(StateTable, LevelName, EntryName, ValueName, false);
	if (StoredValue != nullptr)
	{
		Value = *StoredValue;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::WriteIntValue(FName LevelName, FName EntryName, FName ValueName, int32 Value)
{
	FSoStateTable& StateTable = GetStateTable();
	int32* StoredValue = GetIntFromStateTable(StateTable, LevelName, EntryName, ValueName, true);
	*StoredValue = Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::ReadBoolValue(FName LevelName, FName EntryName, FName ValueName, bool& bValue)
{
	int32 Value = 0;
	if (ReadIntValue(LevelName, EntryName, ValueName, Value))
	{
		bValue = Value == 0 ? false : true;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::WriteBoolValue(FName LevelName, FName EntryName, FName ValueName, bool bValue)
{
	FSoStateTable& StateTable = GetStateTable();
	int32* StoredValue = GetIntFromStateTable(StateTable, LevelName, EntryName, ValueName, true);
	*StoredValue = (bValue == true ? 1 : 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::ReadFloatValue(FName LevelName, FName EntryName, FName ValueName, float& Value)
{
	FSoStateTable& StateTable = GetStateTable();
	const float* StoredValue = GetFloatFromStateTable(StateTable, LevelName, EntryName, ValueName, false);
	if (StoredValue != nullptr)
	{
		Value = *StoredValue;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::WriteFloatValue(FName LevelName, FName EntryName, FName ValueName, float Value)
{
	FSoStateTable& StateTable = GetStateTable();
	float* StoredValue = GetFloatFromStateTable(StateTable, LevelName, EntryName, ValueName, true);
	*StoredValue = Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::ReadVectorValue(FName LevelName, FName EntryName, FName ValueName, FVector& Value)
{
	FSoStateTable& StateTable = GetStateTable();
	const FVector* StoredValue = GetVectorFromStateTable(StateTable, LevelName, EntryName, ValueName, false);
	if (StoredValue != nullptr)
	{
		Value = *StoredValue;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::WriteVectorValue(FName LevelName, FName EntryName, FName ValueName, const FVector& Value)
{
	FSoStateTable& StateTable = GetStateTable();
	FVector* StoredValue = GetVectorFromStateTable(StateTable, LevelName, EntryName, ValueName, true);
	*StoredValue = Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<FSoItem>& FSoWorldState::GetItemList(FName ListName)
{
	FSoStateTable& StateTable = GetStateTable();
	return StateTable.ItemMap.FindOrAdd(ListName).Items;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoWorldState::ReadItemList(FName ListName, TArray<FSoItem>& OutList) const
{
	// TODO special case for episode?
	FSoStateTable& StateTable = GetStateTable();
	// Difference from GetItemList: This does not create the ListName if it does not exist
	const FSoItemArray* Array = StateTable.ItemMap.Find(ListName);
	if (Array == nullptr)
		return false;

	OutList = Array->Items;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoWorldState::WriteItemList(FName ListName, const TArray<FSoItem>& Items)
{
	FSoStateTable& StateTable = GetStateTable();
	FSoItemArray* Array = StateTable.ItemMap.Find(ListName);
	if (Array == nullptr)
		StateTable.ItemMap.Add(ListName, FSoItemArray{ Items });
	else
		Array->Items = Items;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32* FSoWorldState::GetIntFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, FName ValueName, bool bCreate)
{
	FSoStateEntry* Entry = GetEntryFromStateTable(Table, LevelName, EntryName, bCreate);
	if (Entry != nullptr)
	{
		int32* Ptr = Entry->Ints.Find(ValueName);
		if (Ptr == nullptr && bCreate)
			Ptr = &Entry->Ints.Add(ValueName, 0);
		return Ptr;
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float* FSoWorldState::GetFloatFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, FName ValueName, bool bCreate)
{
	FSoStateEntry* Entry = GetEntryFromStateTable(Table, LevelName, EntryName, bCreate);
	if (Entry != nullptr)
	{
		float* Ptr = Entry->Floats.Find(ValueName);
		if (Ptr == nullptr && bCreate)
			Ptr = &Entry->Floats.Add(ValueName, 0.0f);
		return Ptr;
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector* FSoWorldState::GetVectorFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, FName ValueName, bool bCreate)
{
	FSoStateEntry* Entry = GetEntryFromStateTable(Table, LevelName, EntryName, bCreate);
	if (Entry != nullptr)
	{
		FVector* Ptr = Entry->Vectors.Find(ValueName);
		if (Ptr == nullptr && bCreate)
			Ptr = &Entry->Vectors.Add(ValueName, FVector(0,0,0));
		return Ptr;
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoStateEntry* FSoWorldState::GetEntryFromStateTable(FSoStateTable& Table, FName LevelName, FName EntryName, bool bCreate)
{
	FSoStateEntries* EntryMap = Table.Entries.Find(LevelName);

	if (EntryMap == nullptr)
	{
		// Create Level name entry inside table
		if (bCreate)
			EntryMap = &(Table.Entries.Add(LevelName));
		else
			return nullptr;
	}

	FSoStateEntry* Entry = EntryMap->Entries.Find(EntryName);
	if (Entry == nullptr && bCreate)
		Entry = &EntryMap->Entries.Add(EntryName);

	return Entry;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoStateTable& FSoWorldState::GetStateTable() const
{
	// Use this as we must keep references to some actors
	return USoGameSingleton::GetStateTable();
}
