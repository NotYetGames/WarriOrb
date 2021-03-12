// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoWorldStateTable.h"
#include "Misc/Guid.h"
#include "Misc/Base64.h"

#include "Basic/Helpers/SoStringHelper.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "Levels/SoLevelHelper.h"
#include "SoSaveHelper.h"
#include "DlgHelper.h"
#include "Items/ItemTemplates/SoItemTemplate.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoWorldStateTable, All, All);

const FString FSoStateTable::ChecksumSeparator(TEXT("||"));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoStateTableMetadata::CheckAndFixIntegrity()
{
	ProgressStats.CheckAndFixIntegrity();

	for (int32 Index = EquippedItems.Num() - 1; Index >= 0; Index--)
	{
		if (EquippedItems[Index] == nullptr || EquippedItems[Index]->IsPendingKill())
		{
			UE_LOG(LogSoWorldStateTable, Warning, TEXT("StateTableMetadata: Failed integrity, EnterEquippedItems[%d] is not Valid. Removing"), Index);
			EquippedItems.RemoveAtSwap(Index);
		}
	}

	if (!USoLevelHelper::IsValidMapName(MapName))
	{
		// TODO does this break anything?
		UE_LOG(LogSoWorldStateTable, Warning, TEXT("Failed integrity, MapName = `%s` is not valid. Using default"), *MapName.ToString());
		MapName = NAME_None;
	}

	if (InitialSpellsCapacity < 2)
	{
		InitialSpellsCapacity = 2;
	}

	// TODO the rest
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStateTableMetadata::GetSaveTimeAsDateTime(FDateTime& OutDateTime, bool bLocalTime) const
{
	const bool bParsed = FDateTime::ParseIso8601(*SaveTime, OutDateTime);

	// Convert to local time
	if (bParsed && bLocalTime)
		OutDateTime = USoDateTimeHelper::ConvertUTCTimeToLocalTime(OutDateTime);

	return bParsed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoStateTableMetadata::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_String(HashState, SaveTime);
	// NOT USED: ChecksumVersion
	// NOT USED: Checksum
	USoStringHelper::UpdateSHA1_String(HashState, GameBuildVersion);
	USoStringHelper::UpdateSHA1_String(HashState, GameBuildBranch);
	USoStringHelper::UpdateSHA1_String(HashState, GameBuildCommit);
	USoStringHelper::UpdateSHA1_String(HashState, GameBuildCommit);
	USoStringHelper::UpdateSHA1_UInt8(HashState, static_cast<uint8>(Difficulty));
	USoStringHelper::UpdateSHA1_Name(HashState, CheckpointLocationName);
	USoStringHelper::UpdateSHA1_Name(HashState, MapName);
	ProgressStats.UpdateSHA1(HashState);
	USoStringHelper::UpdateSHA1_ArrayOfUObjects(HashState, EquippedItems);
	USoStringHelper::UpdateSHA1_Int32(HashState, InitialMaxHealth);
	USoStringHelper::UpdateSHA1_Int32(HashState, InitialSpellsCapacity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoStateTable::CheckAndFixIntegrity()
{
	if (Version < FSoWorldStateVersion::RedesignedCombatSystem)
	{
		UE_LOG(LogSoWorldStateTable, Warning, TEXT("StateTable: Your save version is too old (before RedesignedCombatSystem), sorry about this :(. "));
		*this = {};
	}

	if (Version < FSoWorldStateVersion::Initial || Version > FSoWorldStateVersion::LatestVersion)
	{
		UE_LOG(LogSoWorldStateTable, Warning, TEXT("StateTable: Failed integrity, Version not in range [0, LatestVersion]. Resseting Version to LatestVersion"));
		Version = FSoWorldStateVersion::LatestVersion;
	}

	for (auto& Elem : ItemMap)
		Elem.Value.CheckAndFixIntegrity();

	Metadata.CheckAndFixIntegrity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStateTable::CheckAndFixIntegrityFromChecksum()
{
	// Check Integrity of playtime seconds
	const FString CurrentCheckSum = GetChecksum();
	if (Metadata.Checksum == CurrentCheckSum)
		return false;

	// Hash did not match, check playtime seconds
	UE_LOG(LogSoWorldStateTable, Error, TEXT("StateTable: Failed integrity. The save file did not match the checksum with the current value. Cheater?"));
	return true;

	// FString SavedSHA1Checksum;
	// int32 SavedChecksumTotalPlaytimeSeconds = 0;
	// if (DecodeChecksum(Metadata.Checksum, SavedSHA1Checksum, SavedChecksumTotalPlaytimeSeconds))
	// {
	// 	const int32 CurrentTotalPlayTimeSeconds = FMath::RoundToInt(Metadata.ProgressStats.TotalPlayTimeSeconds);
	// 	if (CurrentTotalPlayTimeSeconds < SavedChecksumTotalPlaytimeSeconds)
	// 	{
	// 		UE_LOG(
	// 			LogSoWorldStateTable,
	// 			Error,
	// 			TEXT("StateTable: Failed integrity. CurrentTotalPlayTimeSeconds(%d) < SavedChecksumTotalPlaytimeSeconds(%d). Resetting to checksum you filthy cheater!"),
	// 			CurrentTotalPlayTimeSeconds, SavedChecksumTotalPlaytimeSeconds
	// 		);
	// 		Metadata.ProgressStats.TotalPlayTimeSeconds = SavedChecksumTotalPlaytimeSeconds;
	// 	}
	// 	else if (CurrentTotalPlayTimeSeconds != SavedChecksumTotalPlaytimeSeconds)
	// 	{
	// 		// Something fucked up
	// 		bOutResetEverything = true;
	// 	}
	// }
	// else
	// {
	// 	// Maybe he cheated by omitting the checksum?
	// 	bOutResetEverything = true;
	// 	UE_LOG(LogSoWorldStateTable, Error, TEXT("StateTable: Failed integrity. Can't decode Checksum data!"));
	// }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStateTable::operator==(const FSoStateTable& Other) const
{
	return Version == Other.Version &&
		FDlgHelper::IsMapEqual(Entries, Other.Entries) &&
		FDlgHelper::IsMapEqual(StringSets, Other.StringSets) &&
		FDlgHelper::IsMapEqual(DlgHistoryMap, Other.DlgHistoryMap) &&
		FDlgHelper::IsMapEqual(ItemMap, Other.ItemMap) &&
		PlayerData == Other.PlayerData &&
		Metadata == Other.Metadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoStateTable::GetChecksum() const
{
	// Compute the  hash
	FSHA1 HashState;
	USoStringHelper::UpdateSHA1_Int32(HashState, Version);
	Metadata.UpdateSHA1(HashState);
	for (const auto& KeyValue : Entries)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
	for (const auto& KeyValue : StringSets)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
	for (const auto& KeyValue : DlgHistoryMap)
	{
		USoStringHelper::UpdateSHA1_Data(HashState, KeyValue.Key);
		for (int32 Index : KeyValue.Value.VisitedNodeIndices)
		{
			USoStringHelper::UpdateSHA1_Int32(HashState, Index);
		}
	}
	for (const auto& KeyValue : ItemMap)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
	PlayerData.UpdateSHA1(HashState);

	// Get the hash result
	HashState.Final();
	FSHAHash Hash;
	HashState.GetHash(Hash.Hash);

	// AES256(BASE64(SHA1Checksum + ChecksumSeparator + TotalPlaytimeSeconds))
	const FString FullString = Hash.ToString()
		+ ChecksumSeparator
		+ FString::FromInt(FMath::RoundToInt(Metadata.ProgressStats.TotalPlayTimeSeconds));

	// Convert to base64
	return USoStringHelper::AES256Encrypt(FBase64::Encode(FullString), FSoSaveHelper::GetPasswordSaveFile());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStateTable::DecodeChecksum(const FString& RawChecksum, FString& OutCheckSum, int32& OutTotalPlaytimeSeconds)
{
	// Decrypt AES256
	const FString DecryptedString = USoStringHelper::AES256Decrypt(RawChecksum, FSoSaveHelper::GetPasswordSaveFile());

	// Base64 decode
	FString FullString;
	if (!FBase64::Decode(DecryptedString, FullString))
		return false;

	// Split
	TArray<FString> ChecksumParts;
	if (FullString.ParseIntoArray(ChecksumParts, *ChecksumSeparator, false) == 0)
		return false;

	if (ChecksumParts.Num() < 2)
		return false;

	OutCheckSum = ChecksumParts[0];
	OutTotalPlaytimeSeconds = FCString::Atoi(*ChecksumParts[1]);
	return true;
}
