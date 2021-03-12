// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoPlayerProgressSplineStats.h"

#include "Basic/Helpers/SoStringHelper.h"
#include "DlgHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoPlayerProgressSplineStats, All, All);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplineDeathContext::CheckAndFixIntegrity()
{
	if (Distance < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.DeathData.Distance < 0. Resseting to 0"));
		Distance = 0;
	}
	if (TimeSeconds < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.DeathData.TimeSeconds < 0. Resseting to 0"));
		TimeSeconds = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplineCriticalFPSLocation::CheckAndFixIntegrity()
{
	if (AverageDistance < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, SplineCriticalFPS.Distance < 0. Resseting to 0"));
		AverageDistance = 0;
	}
	if (AverageCriticalFPS < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, SplineCriticalFPS.CriticalFPS < 0. Resseting to 0"));
		AverageCriticalFPS = 0;
	}
	if (TimeSeconds < 0.f)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, SplineCriticalFPS.TimeSeconds < 0. Resseting to 0"));
		TimeSeconds = 0.f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplineCriticalFPSAreas::CheckAndFixIntegrity()
{
	for (auto& Elem : Areas)
		Elem.CheckAndFixIntegrity();

	if (SentRecordsNum < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, SplineCriticalFPSAreas.SentRecordsNum < 0. Resseting to 0"));
		SentRecordsNum = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressSplineStats::CheckAndFixIntegrity()
{
	static constexpr int32 NumDamageTypes = static_cast<int32>(ESoDmgType::Max);
	for (int32 Index = 0; Index < NumDamageTypes; Index++)
	{
		const ESoDmgType Type = static_cast<ESoDmgType>(Index);
		if (!LostHpByDamageTypeTable.Contains(Type))
		{
			continue;
		}

		if (LostHpByDamageTypeTable[Type] < 0)
		{
			UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.LostHpByDamageTypeTable[%d] < 0. Resseting to 0"), Index);
			LostHpByDamageTypeTable[Type] = 0;
		}
	}

	if (TimeFirstEnterSeconds < INDEX_NONE)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.FirstEnterTimeSeconds < INDEX_NONE. Resseting to INDEX_NONE"));
		TimeFirstEnterSeconds = INDEX_NONE;
	}
	if (EnterNum < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.EnterNum < 0. Resseting to 0"));
		EnterNum = 0;
	}
	if (TimeSpentSeconds < 0.f)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.TimeSpentSeconds < 0. Resseting to 0"));
		TimeSpentSeconds = 0.f;
	}
	if (AverageFPS < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.AverageFPS < 0. Resseting to 0"));
		AverageFPS = 0;
	}
	if (LostHp < 0)
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, Spline.LostHp < 0. Resseting to 0"));
		LostHp = 0;
	}

	// Sent time first enter, but it is not valid, what
	if (bSentTimeFirstEnter && !IsTimeFirstEnterValid())
	{
		UE_LOG(LogSoPlayerProgressSplineStats, Warning, TEXT("Failed integrity, bSentTimeFirstEnter && !IsTimeFirstEnterValid()"));
		bSentTimeFirstEnter = false;
	}

	CriticalFPSAreas.CheckAndFixIntegrity();
	for (auto& Elem : DeathData)
		Elem.CheckAndFixIntegrity();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoPlayerProgressSplineStats::operator==(const FSoPlayerProgressSplineStats& Other) const
{
	return TimeFirstEnterSeconds == Other.TimeFirstEnterSeconds &&
		bSentTimeFirstEnter == Other.bSentTimeFirstEnter &&
		EnterNum == Other.EnterNum &&
		FMath::IsNearlyEqual(TimeSpentSeconds, Other.TimeSpentSeconds, KINDA_SMALL_NUMBER) &&
		AverageFPS == Other.AverageFPS &&
		DeathData == Other.DeathData &&
		LostHp == Other.LostHp &&
		FDlgHelper::IsMapEqual(LostHpByDamageTypeTable, Other.LostHpByDamageTypeTable);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressSplineStats::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_Int32(HashState, TimeFirstEnterSeconds);
	USoStringHelper::UpdateSHA1_Int32(HashState, bSentTimeFirstEnter);
	USoStringHelper::UpdateSHA1_Int32(HashState, EnterNum);
	USoStringHelper::UpdateSHA1_Float(HashState, TimeSpentSeconds);
	USoStringHelper::UpdateSHA1_Int32(HashState, AverageFPS);
	// NOT USED: CriticalFPSAreas
	USoStringHelper::UpdateSHA1_Int32(HashState, AverageFPS);
	USoStringHelper::UpdateSHA1_Array(HashState, DeathData);
	USoStringHelper::UpdateSHA1_Int32(HashState, LostHp);
	for (const auto& KeyValue : LostHpByDamageTypeTable)
	{
		USoStringHelper::UpdateSHA1_UInt8(HashState, static_cast<uint8>(KeyValue.Key));
		USoStringHelper::UpdateSHA1_Int32(HashState, KeyValue.Value);
	}
}
