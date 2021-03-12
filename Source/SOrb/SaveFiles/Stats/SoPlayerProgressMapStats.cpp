// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoPlayerProgressMapStats.h"

#include "Basic/Helpers/SoStringHelper.h"
#include "DlgHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoPlayerProgressMapStats, All, All);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressMilestoneStats::CheckAndFixIntegrity()
{
	if (TimeSeconds < INDEX_NONE)
	{
		UE_LOG(LogSoPlayerProgressMapStats, Warning, TEXT("Failed integrity, Milestone.TimeSeconds < INDEX_NONE. Resseting to INDEX_NONE"));
		TimeSeconds = INDEX_NONE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressMilestoneStats::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_Int32(HashState, TimeSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoPlayerProgressMapStats::operator==(const FSoPlayerProgressMapStats& Other) const
{
	return FDlgHelper::IsMapEqual(Splines, Other.Splines) &&
		FDlgHelper::IsMapEqual(MilestonesCompleted, Other.MilestonesCompleted);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoPlayerProgressMapStats::UpdateSHA1(FSHA1& HashState) const
{
	for (const auto& KeyValue : Splines)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
	for (const auto& KeyValue : MilestonesCompleted)
	{
		USoStringHelper::UpdateSHA1_Name(HashState, KeyValue.Key);
		KeyValue.Value.UpdateSHA1(HashState);
	}
}
