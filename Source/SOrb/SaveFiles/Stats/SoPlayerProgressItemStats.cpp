// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoPlayerProgressItemStats.h"
#include "Basic/Helpers/SoStringHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoPlayerProgressItemStats, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressItemUsageStats::CheckAndFixIntegrity()
{
	if (TimeEquippedSeconds < INDEX_NONE)
	{
		UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, ItemUsageStats.TimeEquippedSeconds < INDEX_NONE. Resseting both INDEX_NONE"));
		TimeEquippedSeconds = INDEX_NONE;
		TimeUnEquippedSeconds = INDEX_NONE;
	}
	if (TimeUnEquippedSeconds < INDEX_NONE)
	{
		UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, ItemUsageStats.TimeUnEquippedSeconds < INDEX_NONE. Resseting to INDEX_NONE"));
		TimeUnEquippedSeconds = INDEX_NONE;
	}

	if (IsTimeUnEquippedValid() && TimeEquippedSeconds > TimeUnEquippedSeconds)
	{
		UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, ItemUsageStats, TimeEquippedSeconds > TimeUnEquippedSeconds. Resseting both to INDEX_NONE"));
		TimeEquippedSeconds = INDEX_NONE;
		TimeUnEquippedSeconds = INDEX_NONE;
		bSent = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressItemUsageStats::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_Int32(HashState, TimeEquippedSeconds);
	USoStringHelper::UpdateSHA1_Int32(HashState, TimeUnEquippedSeconds);
	USoStringHelper::UpdateSHA1_Int32(HashState, bSent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressItemStats::CheckAndFixIntegrity()
{
	for (int32 Index = Usages.Num() - 1; Index >= 0; Index--)
	{
		Usages[Index].CheckAndFixIntegrity();
		if (!Usages[Index].IsValidRange())
		{
			UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, ItemStats.Usages[%d].IsValidRange() == false. Removing"), Index);
			Usages.RemoveAt(Index);
		}
	}

	// Should not be a valid range
	CurrentUsage.CheckAndFixIntegrity();
	if (CurrentUsage.IsValidRange())
	{
		UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, ItemStats.CurrentUsage.IsValidRange() == true. Reseeting to default"));
		CurrentUsage = {};
	}
	if (CurrentUsage.bSent == true)
	{
		UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, ItemStats.CurrentUsage.bSentAnalytics == true. Reseeting to false"));
		CurrentUsage.bSent = false;
	}
	if (CurrentUsage.IsTimeUnEquippedValid())
	{
		UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, ItemStats.IsTimeUnEquippedValid() == true. Reseeting to INDEX_NONE"));
		CurrentUsage.TimeUnEquippedSeconds = INDEX_NONE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressItemStats::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_Array(HashState, Usages);
	CurrentUsage.UpdateSHA1(HashState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressSpellStats::CheckAndFixIntegrity()
{
	if (CastNum < 0)
	{
		CastNum = 0;
		UE_LOG(LogSoPlayerProgressItemStats, Warning, TEXT("Failed integrity, SpellStats.CastNum < 0. Reseting to 0"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressSpellStats::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_Int32(HashState, CastNum);
}
