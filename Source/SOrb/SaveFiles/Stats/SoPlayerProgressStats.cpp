// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoPlayerProgressStats.h"

#include "Basic/Helpers/SoStringHelper.h"
#include "Levels/SoLevelHelper.h"
#include "DlgHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoPlayerProgressStats, All, All);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressStats::CheckAndFixIntegrity()
{
	static constexpr int32 NumDamageTypes = static_cast<int32>(ESoDmgType::Max);
	for (int32 Index = 0; Index < NumDamageTypes; Index++)
	{
		const ESoDmgType Type = static_cast<ESoDmgType>(Index);
		if (!TotalLostHpByDamageTypeTable.Contains(Type))
		{
			continue;
		}

		if (TotalLostHpByDamageTypeTable[Type] < 0)
		{
			UE_LOG(LogSoPlayerProgressStats, Warning, TEXT("Failed integrity, TotalLostHpByDamageTypeTable[%d] < 0. Resseting to 0"), Index);
			TotalLostHpByDamageTypeTable[Type] = 0;
		}
	}

	if (TotalDeathNum < 0)
	{
		UE_LOG(LogSoPlayerProgressStats, Warning, TEXT("Failed integrity, TotalDeathNum < 0. Resseting to 0"));
		TotalDeathNum = 0;
	}
	if (TotalDeathWithSoulKeeperNum < 0)
	{
		UE_LOG(LogSoPlayerProgressStats, Warning, TEXT("Failed integrity, TotalDeathWithSoulKeeperNum < 0. Resseting to 0"));
		TotalDeathWithSoulKeeperNum = 0;
	}
	if (TotalDeathWithCheckpointNum < 0)
	{
		UE_LOG(LogSoPlayerProgressStats, Warning, TEXT("Failed integrity, TotalDeathWithCheckpointNum < 0. Resseting to 0"));
		TotalDeathWithCheckpointNum = 0;
	}
	if (TotalLostHp < 0)
	{
		UE_LOG(LogSoPlayerProgressStats, Warning, TEXT("Failed integrity, TotalLostHp < 0. Resseting to 0"));
		TotalLostHp = 0;
	}
	if (TotalPlayTimeSeconds < 0.f)
	{
		UE_LOG(LogSoPlayerProgressStats, Warning, TEXT("Failed integrity, PlayTimeSeconds < 0. Resseting to 0"));
		TotalPlayTimeSeconds = 0.f;
	}

	{
		TArray<FName> KeysToRemove;
		for (auto& Elem : MapsProgressTable)
		{
			const FName MapName = Elem.Key;
			if (!USoLevelHelper::IsValidMapName(MapName))
			{
				UE_LOG(LogSoPlayerProgressStats, Warning, TEXT("Failed integrity, MapName = `%s` is not valid. Removing"), *MapName.ToString());
				KeysToRemove.Add(MapName);
				continue;
			}

			Elem.Value.CheckAndFixIntegrity();
		}

		for (FName Key : KeysToRemove)
			MapsProgressTable.Remove(Key);
	}
	{
		for (auto& Elem : ItemsStatsTable)
			Elem.Value.CheckAndFixIntegrity();
	}

	{
		for (auto& Elem : SpellsStatsTable)
			Elem.Value.CheckAndFixIntegrity();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoPlayerProgressStats::operator==(const FSoPlayerProgressStats& Other) const
{
	return TotalDeathNum == Other.TotalDeathNum &&
		TotalDeathWithSoulKeeperNum == Other.TotalDeathWithSoulKeeperNum &&
		TotalDeathWithCheckpointNum == Other.TotalDeathWithCheckpointNum &&
		TotalLostHp == Other.TotalLostHp &&
		FDlgHelper::IsMapEqual(TotalLostHpByDamageTypeTable, Other.TotalLostHpByDamageTypeTable) &&
		FMath::IsNearlyEqual(TotalPlayTimeSeconds, Other.TotalPlayTimeSeconds, KINDA_SMALL_NUMBER) &&
		FDlgHelper::IsMapEqual(MapsProgressTable, Other.MapsProgressTable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressStats::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_UInt8(HashState, static_cast<uint8>(ProgressState));
	USoStringHelper::UpdateSHA1_Int32(HashState, TotalDeathNum);
	USoStringHelper::UpdateSHA1_Int32(HashState, TotalDeathWithSoulKeeperNum);
	USoStringHelper::UpdateSHA1_Int32(HashState, TotalDeathWithCheckpointNum);
	USoStringHelper::UpdateSHA1_Int32(HashState, TotalLostHp);
	USoStringHelper::UpdateSHA1_Float(HashState, TotalPlayTimeSeconds);
	USoStringHelper::UpdateSHA1_Int32(HashState, TotalAverageFPS);
	for (const auto& KeyValue : TotalLostHpByDamageTypeTable)
	{
		USoStringHelper::UpdateSHA1_UInt8(HashState, static_cast<uint8>(KeyValue.Key));
		USoStringHelper::UpdateSHA1_Int32(HashState, KeyValue.Value);
	}
	for (const auto& KeyValue : MapsProgressTable)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
	for (const auto& KeyValue : ItemsStatsTable)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
	for (const auto& KeyValue : SpellsStatsTable)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
}
