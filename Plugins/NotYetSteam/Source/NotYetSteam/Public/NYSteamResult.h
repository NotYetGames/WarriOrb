// Copyright 2019 Daniel Butum

#pragma once

#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"
#include "NYSteamAPI_Internal.h"

#include "NYSteamResult.generated.h"


// https://partner.steamgames.com/doc/api/ISteamUserStats#UserStatsStored_t
// NOTE: always current user
USTRUCT()
struct NOTYETSTEAM_API FNYSteamUserStatsStored_Result
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	bool bSuccess = false;
};


// https://partner.steamgames.com/doc/api/ISteamUserStats#UserStatsUnloaded_t
USTRUCT()
struct NOTYETSTEAM_API FNYSteamUserStatsUnloaded_Result
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	bool bSuccess = false;

	UPROPERTY()
	bool bIsCurrentUser = false;

	uint64 UserID = 0;
};


// https://partner.steamgames.com/doc/api/ISteamUserStats#UserStatsReceived_t
USTRUCT()
struct NOTYETSTEAM_API FNYSteamUserStatsReceived_Result
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	bool bSuccess = false;

	UPROPERTY()
	bool bIsCurrentUser = false;

	uint64 UserID = 0;
};

// https://partner.steamgames.com/doc/api/ISteamUserStats#UserAchievementStored_t
// NOTE: always current user
USTRUCT()
struct NOTYETSTEAM_API FNYSteamUserAchievementStored_Result
{
	GENERATED_USTRUCT_BODY()

public:
	bool IsUnlocked() const
	{
		return CurrentProgress == 0 && MaxProgress == 0;
	}

public:
	UPROPERTY()
	bool bSuccess = false;

	UPROPERTY()
	FName AchievementName;

	UPROPERTY()
	uint32 CurrentProgress = 0;

	UPROPERTY()
	uint32 MaxProgress = 0;
};

struct NOTYETSTEAM_API FNYSteamResultHelper
{
	// Takes a Steam EResult value, and converts it into a string (with extra debug info)
	static FString ResultToString(EResult Result);

	static bool IsResultSuccess(EResult Result)
	{
		return Result == k_EResultOK;
	}
};
