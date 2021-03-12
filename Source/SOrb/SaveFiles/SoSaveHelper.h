// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IDlgWriter;
class IDlgParser;

// See: https://docs.unrealengine.com/en-US/Gameplay/SaveGame/index.html
/**
 * Static functions handling save/load data to/from files
 * NOTE: All the file names do not have the file extension as it is added in a later step
 */
class FSoSaveHelper
{
public:
	static bool SaveToFile(IDlgWriter& Writer, const FString& FileNameNoExt, int32 UserIndex);
	static bool LoadFromFile(IDlgParser& Parser, const FString& FileNameNoExt, int32 UserIndex);

	static void BackupSaveFile(const FString& FileNameNoExt, const FString& NewCopyName, int32 UserIndex);
	static bool DeleteSaveFile(const FString& FileNameNoExt, int32 UserIndex);


	// Save gam file exists?
	static bool DoesSaveFileNameExist(const FString& FileNameNoExt, int32 UserIndex);
	static bool DoesSaveFileNameExistForSlot(const FString& FileNameNoExt, int32 SlotIndex, int32 UserIndex)
	{
		return DoesSaveFileNameExist(GetSaveFileNameFromSlot(FileNameNoExt, SlotIndex), UserIndex);
	}

	static FString GetSaveFilePathFromFileName(const FString& FileNameNoExt)
	{
		if (UseUnrealSaveSystem())
		{
			// Extension is added by unreal save system
			return FileNameNoExt;
		}

		// Text file
		return FString::Printf(TEXT("%s/%s.sav.txt"), *GetSaveGameDirectoryPath(), *FileNameNoExt);
	}

	// Gets the path for save file from the SaveSlotName Name and the provided SlotIndex;
	static FString GetSaveFilePathFromSlot(const FString& FileNameNoExt, int32 SlotIndex)
	{
		if (UseUnrealSaveSystem())
		{
			// Extension is added by unreal save system
			return FString::Printf(TEXT("%s_%d"), *FileNameNoExt, SlotIndex);
		}
		
		// Inside the Saves/SaveGames/<Name>_<Index>.sav.txt
		return FString::Printf(TEXT("%s/%s_%d.sav.txt"), *GetSaveGameDirectoryPath(), *FileNameNoExt, SlotIndex);
	}

	static FString GetSaveFileNameFromSlot(const FString& FileNameNoExt, int32 SlotIndex)
	{
		return FString::Printf(TEXT("%s_%d"), *FileNameNoExt, SlotIndex);
	}

	// Gets a valid (non existent) deleted save game file path for the Specified SlotIndex.
	static bool GetDeletedSaveFileNameFromSlot(const FString& SaveFileNameNoExt, int32 SlotIndex, int32 UserIndex, FString& OutDeletedFileNameNoExt)
	{
		return GetDeletedSaveFileNameFromFileName(
			FString::Printf(TEXT("DELETED_%s_%d"), *SaveFileNameNoExt, SlotIndex),
			UserIndex,
			OutDeletedFileNameNoExt
		);
	}
	static bool GetDeletedSaveFileNameFromFileName(const FString& SaveFileNameNoExt, int32 UserIndex, FString& OutDeletedFileNameNoExt);

	// Directory of where all the saves are
	static const FString& GetSaveGameDirectoryPath();

	// From WARRIORB_PASSWORD_SAVE_FILE
	static const FString& GetPasswordSaveFile();

	// Use the unreal binary save system or our own save system?
	static bool UseUnrealSaveSystem() { return WARRIORB_USE_UNREAL_SAVE_SYSTEM; }

protected:
	// Save string to file using the Unreal Save System
	static bool WriteUsingUnrealSaveSystem(const FString& FileName, int32 UserIndex, const FString& Data);
	static bool ReadUsingUnrealSaveSystem(const FString& FileName, int32 UserIndex, FString& OutData);
};
