// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoDateTimeHelper.h"
#include "Engine/Engine.h"
#include "Settings/SoGameSettings.h"

#define LOCTEXT_NAMESPACE "DateTimeHelper"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoDateTimeHelper::GetTextFromMonth(int32 Month)
{
	const EMonthOfYear MonthType = static_cast<EMonthOfYear>(Month);
	switch (MonthType)
	{
		case EMonthOfYear::January:		return LOCTEXT("MonthJanuary", "January");
		case EMonthOfYear::February:	return LOCTEXT("MonthFebruary", "February");
		case EMonthOfYear::March:		return LOCTEXT("MonthMarch", "March");
		case EMonthOfYear::April:		return LOCTEXT("MonthApril", "April");
		case EMonthOfYear::May:			return LOCTEXT("MonthMay", "May");
		case EMonthOfYear::June:		return LOCTEXT("MonthJune", "June");
		case EMonthOfYear::July:		return LOCTEXT("MonthJuly", "July");
		case EMonthOfYear::August:		return LOCTEXT("MonthAugust", "August");
		case EMonthOfYear::September:	return LOCTEXT("MonthSeptember", "September");
		case EMonthOfYear::October:		return LOCTEXT("MonthOctober", "October");
		case EMonthOfYear::November:	return LOCTEXT("MonthNovember", "November");
		default:
		case EMonthOfYear::December:	return LOCTEXT("MonthDecember", "December");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoDateTimeHelper::GetTextFromSeconds(int32 Seconds, bool bShowEmptyHour)
{
	static FNumberFormattingOptions NumberFormattingOptions;
	NumberFormattingOptions.MinimumIntegralDigits = 2;

	int32 NormalizedHours = 0;
	int32 NormalizedMinutes = 0;
	int32 NormalizedSeconds = 0;
	NormalizeSeconds(Seconds, NormalizedHours, NormalizedMinutes, NormalizedSeconds);

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Hours"), FText::AsNumber(NormalizedHours, &NumberFormattingOptions));
	Arguments.Add(TEXT("Minutes"), FText::AsNumber(NormalizedMinutes, &NumberFormattingOptions));
	Arguments.Add(TEXT("Seconds"), FText::AsNumber(NormalizedSeconds, &NumberFormattingOptions));

	if (!bShowEmptyHour && NormalizedHours == 0)
	{
		return FText::Format(LOCTEXT("SecondsToTime", "{Minutes}:{Seconds}"), Arguments);
	}

	return FText::Format(LOCTEXT("SecondsToTimeMinusSeconds", "{Hours}:{Minutes}:{Seconds}"), Arguments);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::NormalizeTime(float Time)
{
	return Time * USoGameSettings::Get().GetGameSpeed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::DenormalizeTime(float Time)
{
	return Time / USoGameSettings::Get().GetGameSpeed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::GetWorldDeltaSeconds(const UObject* WorldContextObject)
{
	if (!GEngine)
		return 0.f;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetDeltaSeconds() : 0.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::GetWorldTimeSeconds(const UObject* WorldContextObject)
{
	if (!GEngine)
		return 0.f;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetTimeSeconds() : 0.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::GetWorldUnpausedTimeSeconds(const UObject* WorldContextObject)
{
	if (!GEngine)
		return 0.f;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetUnpausedTimeSeconds() : 0.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::GetWorldRealTimeSeconds(const UObject* WorldContextObject)
{
	if (!GEngine)
		return 0.f;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetRealTimeSeconds() : 0.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::GetWorldAudioTimeSeconds(const UObject* WorldContextObject)
{
	if (!GEngine)
		return 0.f;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetAudioTimeSeconds() : 0.f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::GetWorldTimeSinceSeconds(const UObject* WorldContextObject, float Time)
{
	return GetWorldTimeSeconds(WorldContextObject) - Time;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoDateTimeHelper::GetWorldRealTimeSinceSeconds(const UObject* WorldContextObject, float Time)
{
	return GetWorldRealTimeSeconds(WorldContextObject) - Time;
}

#undef LOCTEXT_NAMESPACE
