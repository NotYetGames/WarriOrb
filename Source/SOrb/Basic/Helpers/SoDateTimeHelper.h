// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/App.h"

#include "SoDateTimeHelper.generated.h"

// Helper class for Date and time related things
UCLASS()
class SORB_API USoDateTimeHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Converts UtcDateTime from UTC time (UTC+0) to local time.
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static FORCEINLINE FDateTime ConvertUTCTimeToLocalTime(FDateTime UTCDateTime)
	{
		// Shift to the local time.
		UTCDateTime += GetTimeZoneAsTimespan();
		return UTCDateTime;
	}

	// Converts LocalDateTime from the local time zone to the UTC+0 time
	// NOTE: This assumes that LocalDateTime is the same local time as this machine.
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static FORCEINLINE FDateTime ConvertLocalTimeToUTCTime(FDateTime LocalDateTime)
	{
		// Unshift to UTC+0 time
		LocalDateTime -= GetTimeZoneAsTimespan();
		return LocalDateTime;
	}

	/** Gets the formated Text from the month number, where January is the first month, Month = 1  */
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static FText GetTextFromMonth(int32 Month);

	/**
	 * Gets the current time zone as a FTimespan.
	 * NOTE: this should have pretty good precision up to seconds.
	 * The hours should be the timezone difference we want.
	 * Should be:
	 * + for TimeZone > 0
	 * - for TimeZone < 0
	 */
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static FORCEINLINE FTimespan GetTimeZoneAsTimespan()
	{
		return FDateTime::Now() - FDateTime::UtcNow();
	}

	// Shortcut for the GetTimeZoneAsTimespan
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static FORCEINLINE int32 GetTimeZoneAsInt()
	{
		return GetTimeZoneAsTimespan().GetHours();
	}

	// Does as it says
	static FORCEINLINE float SecondsToMinutes(float Seconds) { return Seconds / 60.f; }
	static FORCEINLINE float SecondsToMinutes(int32 Seconds) { return static_cast<float>(Seconds) / 60.f; }

	/**
	 * Converts an X amount of seconds into hours, minutes and seconds.
	 * Normalizes the seconds so that they can be put on a clock.
	 * OutHours - [0, infinity)
	 * OutSeconds - [0, 60)
	 * OutMinutes - [0, 60)
	 */
	static FORCEINLINE void NormalizeSeconds(int32 Seconds, int32& OutHours, int32& OutMinutes, int32& OutSeconds)
	{
		OutSeconds = Seconds % 60;
		// Only interested in the minutes of hours, not the total absolute minutes value
		OutMinutes = (Seconds % 3600) / 60;
		OutHours = Seconds / 3600;
	}

	// BP version
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static void NormalizeSecondsBP(int32 Seconds, int32& OutHours, int32& OutMinutes, int32& OutSeconds)
	{
		NormalizeSeconds(Seconds, OutHours, OutMinutes, OutSeconds);
	}

	/**
	 * Format Seconds to to a human readable format: Hours:Minutes[:Seconds]
	 */
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static FText GetTextFromSeconds(int32 Seconds, bool bShowEmptyHour = true);

	// Helper for GetTextFromIntSeconds, truncates seconds towards 0
	UFUNCTION(BlueprintPure, Category = ">DateTime")
	static FORCEINLINE FText GetTextFromFloatSeconds(const float Seconds, const bool bShowEmptyHour = true)
	{
		return GetTextFromSeconds(FMath::TruncToInt(Seconds), bShowEmptyHour);
	}

	// This is just GetAverageMS() / 1000.f
	UFUNCTION(BlueprintPure, Category = ">Stats")
	static float GetAverageFPS()
	{
		extern ENGINE_API float GAverageFPS;
		return GAverageFPS;
	}

	UFUNCTION(BlueprintPure, Category = ">Stats")
	static float GetAverageMS()
	{
		extern ENGINE_API float GAverageMS;
		return GAverageMS;
	}

	// Gets the Time but normalized to the current game speed (time dilation)
	UFUNCTION(BlueprintPure, Category = ">Time")
	static float NormalizeTime(float Time);

	UFUNCTION(BlueprintPure, Category = ">Time")
	static float DenormalizeTime(float Time);

	// Gets app time delta in seconds (this is real time)
	// IS stopped when game pauses
	// NOT dilated/clamped
	UFUNCTION(BlueprintPure, Category = ">Time")
	static float GetDeltaSeconds() { return static_cast<float>(GetDeltaSecondsDouble()); }
	static double GetDeltaSecondsDouble() { return FApp::GetDeltaTime(); }

	// Returns the world frame delta time in seconds
	// IS adjusted by time dilation
	UFUNCTION(BlueprintPure, Category = ">Time", meta = (WorldContext="WorldContextObject"))
	static float GetWorldDeltaSeconds(const UObject* WorldContextObject);

	// Returns time in seconds since world was brought up for play
	// IS stopped when game pauses
	// IS dilated/clamped
	UFUNCTION(BlueprintPure, Category=">Time", meta=(WorldContext="WorldContextObject"))
	static float GetWorldTimeSeconds(const UObject* WorldContextObject);

	// Returns time in seconds since world was brought up for play
	// NOT stopped when game pauses
	// IS dilated/clamped
	UFUNCTION(BlueprintPure, Category=">Time", meta=(WorldContext="WorldContextObject"))
	static float GetWorldUnpausedTimeSeconds(const UObject* WorldContextObject);

	// Returns time in seconds since world was brought up for play
	// NOT stopped when game pauses
	// NOT dilated/clamped
	UFUNCTION(BlueprintPure, Category=">Time", meta=(WorldContext="WorldContextObject"))
	static float GetWorldRealTimeSeconds(const UObject* WorldContextObject);

	// Returns time in seconds since world was brought up for play
	// IS stopped when game pauses
	// NOT dilated/clamped
	UFUNCTION(BlueprintPure, Category=">Time", meta=(WorldContext="WorldContextObject"))
	static float GetWorldAudioTimeSeconds(const UObject* WorldContextObject);

	// Helper for getting the time since a certain time.
	// This is just GetWorldTimeSeconds() - Time.
	// Time should be:
	// - IS stopped when game pauses
	// - IS dilated/clamped
	UFUNCTION(BlueprintPure, Category=">Time", meta=(WorldContext="WorldContextObject"))
	static float GetWorldTimeSinceSeconds(const UObject* WorldContextObject, float Time);


	// Helper for getting the time since a certain time.
	// This is just GetWorldRealTimeSeconds() - Time.
	// Time should be:
	// - NOT stopped when game pauses
	// - NOT dilated/clamped
	UFUNCTION(BlueprintPure, Category=">Time", meta=(WorldContext="WorldContextObject"))
	static float GetWorldRealTimeSinceSeconds(const UObject* WorldContextObject, float Time);
};
