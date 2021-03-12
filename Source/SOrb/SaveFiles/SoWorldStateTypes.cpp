// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoWorldStateTypes.h"

#include "Basic/Helpers/SoStringHelper.h"
#include "DlgHelper.h"
#include "Levels/SoLevelHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoWorldStateTypes, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStateEntry::operator==(const FSoStateEntry& Other) const
{
	return FDlgHelper::IsMapEqual(Ints, Other.Ints) &&
		FDlgHelper::IsMapEqual(Floats, Other.Floats) &&
		FDlgHelper::IsMapEqual(Vectors, Other.Vectors);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoStateEntry::UpdateSHA1(FSHA1& HashState) const
{
	for (const auto& KeyValue : Ints)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		USoStringHelper::UpdateSHA1_Int32(HashState, KeyValue.Value);
	}
	for (const auto& KeyValue : Floats)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		USoStringHelper::UpdateSHA1_Float(HashState, KeyValue.Value);
	}
	for (const auto& KeyValue : Vectors)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		USoStringHelper::UpdateSHA1_Data(HashState, KeyValue.Value);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStateEntries::operator==(const FSoStateEntries& Other) const
{
	return FDlgHelper::IsMapEqual(Entries, Other.Entries);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoStateEntries::UpdateSHA1(FSHA1& HashState) const
{
	for (const auto& KeyValue : Entries)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStringSet::operator==(const FSoStringSet& Other) const
{
	return FDlgHelper::IsSetEqual(Strings, Other.Strings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoStringSet::UpdateSHA1(FSHA1& HashState) const
{
	for (FName Name : Strings)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, Name);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoNameSet::operator==(const FSoNameSet& Other) const
{
	return FDlgHelper::IsSetEqual(Names, Other.Names);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoNameSet::UpdateSHA1(FSHA1& HashState) const
{
	for (FName Name : Names)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, Name);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoPlayerData::operator==(const FSoPlayerData& Other) const
{
	return FDlgHelper::IsSetEqual(TrueBools, Other.TrueBools) &&
		FDlgHelper::IsMapEqual(Ints, Other.Ints) &&
		FDlgHelper::IsMapEqual(Floats, Other.Floats) &&
		FDlgHelper::IsMapEqual(Names, Other.Names);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoItemArray::UpdateSHA1(FSHA1& HashState) const
{
	for (const auto& Elem : Items)
	{
		Elem.UpdateSHA1(HashState);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerData::UpdateSHA1(FSHA1& HashState) const
{
	for (const auto& KeyValue : Ints)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		USoStringHelper::UpdateSHA1_Int32(HashState, KeyValue.Value);
	}
	for (const auto& KeyValue : Floats)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		USoStringHelper::UpdateSHA1_Float(HashState, KeyValue.Value);
	}
	for (FName Name : TrueBools)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, Name);
	}
	for (const auto& KeyValue : Names)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Value);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoStateMetadata::CheckAndFixIntegrity()
{
	// Invalid ranges
	bool bFixedSomething = false;
	if (Version < FSoWorldStateVersion::Initial || Version > FSoWorldStateVersion::LatestVersion)
	{
		UE_LOG(LogSoWorldStateTypes, Error, TEXT("Metadata: Failed integrity, Version not in range [0, LatestVersion]. Resseting Version to LatestVersion"));
		Version = FSoWorldStateVersion::LatestVersion;
		bFixedSomething = true;
	}

	if (ChapterSlotIndex < 0)
	{
		UE_LOG(LogSoWorldStateTypes, Error, TEXT("Metadata: Failed integrity, ChapterSlotIndex < 0. Resseting to 0"));
		ChapterSlotIndex = 0;
		bFixedSomething = true;
	}

	if (EpisodeSlotIndex < 0)
	{
		UE_LOG(LogSoWorldStateTypes, Error, TEXT("Metadata: Failed integrity, EpisodeSlotIndex < 0. Resseting to 0"));
		EpisodeSlotIndex = 0;
		bFixedSomething = true;
	}

	if (!USoLevelHelper::IsValidEpisodeName(EpisodeName))
	{
		const FName FirstValidEpisodeName = USoLevelHelper::GetFirstValidEpisodeName();
		UE_LOG(
			LogSoWorldStateTypes,
			Error,
			TEXT("Metadata: Failed integrity, EpisodeName = %s is invalid. Resseting to default = %s"),
			*EpisodeName.ToString(), *FirstValidEpisodeName.ToString()
		);
		EpisodeName = FirstValidEpisodeName;
		bFixedSomething = true;
	}

	return bFixedSomething;
}
