// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoSaveHelper.h"

#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "SaveGameSystem.h"
#include "PlatformFeatures.h"

#include "IO/IDlgWriter.h"
#include "IO/IDlgParser.h"
#include "SoWorldState.h"
#include "Kismet/GameplayStatics.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSaveHelper::SaveToFile(IDlgWriter& Writer, const FString& FileNameNoExt, int32 UserIndex)
{
#if PLATFORM_XBOXONE
	if (UserIndex == -1)
		return false;
#endif

	// Binary files
	if (UseUnrealSaveSystem())
	{
		return WriteUsingUnrealSaveSystem(GetSaveFilePathFromFileName(FileNameNoExt), UserIndex, Writer.GetAsString());
	}

	// Text files
	return Writer.ExportToFile(GetSaveFilePathFromFileName(FileNameNoExt));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSaveHelper::LoadFromFile(IDlgParser& Parser, const FString& FileNameNoExt, int32 UserIndex)
{
#if PLATFORM_XBOXONE
	if (UserIndex == -1)
		return false;
#endif

	// Binary files
	if (UseUnrealSaveSystem())
	{
		FString Data;
		if (ReadUsingUnrealSaveSystem(GetSaveFilePathFromFileName(FileNameNoExt), UserIndex,Data))
		{
			Parser.InitializeParserFromString(Data);
			return true;
		}
		return false;
	}

	// Text files
	Parser.InitializeParser(GetSaveFilePathFromFileName(FileNameNoExt));
	return Parser.IsValidFile();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSaveHelper::BackupSaveFile(const FString& FileNameNoExt, const FString& NewCopyName, int32 UserIndex)
{
	const FString FileToCopy = GetSaveFilePathFromFileName(FileNameNoExt);
	const FString BackupFile = GetSaveFilePathFromFileName(NewCopyName);

	UE_LOG(
		LogSoWorldState,
		Verbose,
		TEXT("Creating BACKUP for Save file: Copying file from = `%s` to = `%s`"),
		*FileToCopy, *BackupFile
	);
	if (UseUnrealSaveSystem())
	{
		// Binary files
		FString DataToCopy;
		if (ReadUsingUnrealSaveSystem(FileToCopy, UserIndex, DataToCopy))
		{
			WriteUsingUnrealSaveSystem(BackupFile, UserIndex, DataToCopy);
		}
	}
	else
	{
		// Text files
		// Delete it first as we can't copy if already exists
		IFileManager::Get().Copy(*BackupFile, *FileToCopy, true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSaveHelper::DeleteSaveFile(const FString& FileNameNoExt, int32 UserIndex)
{
	// Binary files
	if (UseUnrealSaveSystem())
	{
		ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
		if (SaveSystem && FileNameNoExt.Len() > 0)
		{
			return SaveSystem->DeleteGame(false, *GetSaveFilePathFromFileName(FileNameNoExt), UserIndex);
		}
		return false;
	}

	// Text files
	return IFileManager::Get().Delete(*GetSaveFilePathFromFileName(FileNameNoExt), true, false, false);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSaveHelper::WriteUsingUnrealSaveSystem(const FString& FileName, int32 UserIndex, const FString& Data)
{
	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	// If we have a system and an object to save and a save name...
	if (SaveSystem && FileName.Len() > 0 && Data.Len() > 0)
	{
		FString NonConstData = Data;
		TArray<uint8> ObjectBytes;
		FMemoryWriter MemoryWriter(ObjectBytes, true);
		MemoryWriter << NonConstData;

		return SaveSystem->SaveGame(false, *FileName, UserIndex, ObjectBytes);
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSaveHelper::ReadUsingUnrealSaveSystem(const FString& FileName, int32 UserIndex, FString& OutData)
{
	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	// If we have a save system and a valid name..
	if (SaveSystem && (FileName.Len() > 0))
	{
		// Load raw data from slot
		TArray<uint8> ObjectBytes;
		bool bSuccess = SaveSystem->LoadGame(false, *FileName, UserIndex, ObjectBytes);
		if (bSuccess)
		{
			FMemoryReader MemoryReader(ObjectBytes, true);
			MemoryReader << OutData;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSaveHelper::DoesSaveFileNameExist(const FString& FileNameNoExt, int32 UserIndex)
{
	// Binary files
	if (UseUnrealSaveSystem())
	{
		if (ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem())
		{
			return SaveSystem->DoesSaveGameExist(*GetSaveFilePathFromFileName(FileNameNoExt), UserIndex);
		}
		return false;
	}

	// Text files
	return IFileManager::Get().FileExists(*GetSaveFilePathFromFileName(FileNameNoExt));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSaveHelper::GetDeletedSaveFileNameFromFileName(const FString& SaveFileNameNoExt, int32 UserIndex, FString& OutDeletedFileNameNoExt)
{
	static constexpr int32 MaxIteration = 1000;
	bool bFoundValid = false;
	int32 Iteration = -1;
	do
	{
		// Add indices to the end of BasePath until we find a valid one
		Iteration++;
		OutDeletedFileNameNoExt = FString::Printf(TEXT("%s-backup-%d"), *SaveFileNameNoExt, Iteration);
		if (!DoesSaveFileNameExist(OutDeletedFileNameNoExt, UserIndex))
			bFoundValid = true;

		// until the file does not exists OR until something is very wrong.
	} while (!bFoundValid && Iteration < MaxIteration);

	return bFoundValid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString& FSoSaveHelper::GetSaveGameDirectoryPath()
{
	static FString Path = FString::Printf(TEXT("%sSaveGames"), *FPaths::ProjectSavedDir());
	return Path;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FString& FSoSaveHelper::GetPasswordSaveFile()
{
#ifndef WARRIORB_PASSWORD_SAVE_FILE
#error "WARRIORB_PASSWORD_SAVE_FILE is not set in the SOrb.Build.cs file"
#endif // !WARRIORB_PASSWORD_SAVE_FILE
	static const FString Password(TEXT(WARRIORB_PASSWORD_SAVE_FILE));
	return Password;
}
