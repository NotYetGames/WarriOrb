// Copyright (c) Daniel Butum. All Rights Reserved.

#include "Interfaces/NYSteamStatsAndAchievements.h"

#include "Misc/ConfigCacheIni.h"
#include "NYSteamAPI_Internal.h"
#include "NYSteamHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogNYSteamAchievements, All, All)


FNYSteamAchievementsConfigReader::FNYSteamAchievementsConfigReader()
	: IniName(GEngineIni)
	, SectionName(TEXT("OnlineSubsystemSteam"))
{
}

// Returns empty string if couldn't read
FString FNYSteamAchievementsConfigReader::GetKey(const FString& KeyName)
{
	FString Result;
	if (!GConfig->GetString(*SectionName, *KeyName, Result, IniName))
	{
		return TEXT("");	// could just return Result, but being explicit is better
	}
	return Result;
}

bool FNYSteamAchievementsConfigReader::ReadAchievements(TArray<FNYUnrealSteamAchievementConfig> & OutArray)
{
	OutArray.Empty();
	int NumAchievements = 0;

	for (;; ++NumAchievements)
	{
		const FString Name = GetKey(FString::Printf(TEXT("Achievement_%d_Id"), NumAchievements));
		if (Name.IsEmpty())
		{
			break;
		}

		FNYUnrealSteamAchievementConfig NewAch{ Name };
		OutArray.Add(NewAch);
	}

	return NumAchievements > 0;
}



FNYSteamStatsAndAchievements::FNYSteamStatsAndAchievements(INYSteamSubsystem* InSubsystem)
	: SteamSubsystem(InSubsystem)
{
	check(SteamSubsystem);

	RequestCurrentStats();
}

void FNYSteamStatsAndAchievements::OnUserStatsReceived(FNYSteamUserStatsReceived_Result Result)
{
	if (Result.bIsCurrentUser)
	{
		bStatsReceivedValid = Result.bSuccess;
	}

	UserStatsReceivedEvent.Broadcast(Result);
}

void FNYSteamStatsAndAchievements::OnUserStatsStored(FNYSteamUserStatsStored_Result Result)
{
	UserStatsStoredEvent.Broadcast(Result);
}

void FNYSteamStatsAndAchievements::OnUserStatsUnloaded(FNYSteamUserStatsUnloaded_Result Result)
{
	UserStatsUnloadedEvent.Broadcast(Result);
}

void FNYSteamStatsAndAchievements::OnUserAchievementStored(FNYSteamUserAchievementStored_Result Result)
{
	UserAchievementStoredEvent.Broadcast(Result);
}

bool FNYSteamStatsAndAchievements::UnlockAchievement(FName AchievementName)
{
	const FString ThisContext = FString::Printf(
		TEXT("UnlockAchievement(AchievementName = %s)"),
		*AchievementName.ToString()
	);

	UE_LOG(LogNYSteamAchievements, Log, TEXT("%s"), *ThisContext);
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	const bool bStatus = UserStats->SetAchievement(TCHAR_TO_UTF8(*AchievementName.ToString()));

	// TODO: maybe trigger this for the next tick?
	return bStatus && StoreStats();
}

bool FNYSteamStatsAndAchievements::ResetAllAchievements()
{
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("ResetAllAchievements: bStatsReceivedValid = false"));
		return false;
	}

	return ResetAllStats(true);
}

bool FNYSteamStatsAndAchievements::ResetAchievement(FName AchievementName, bool bStoreStats)
{
	const FString ThisContext = FString::Printf(
		TEXT("ResetAchievement(Name = %s, bStoreStats = %d)"),
		*AchievementName.ToString(), bStoreStats
	);
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	if (UserStats->ClearAchievement(TCHAR_TO_UTF8(*AchievementName.ToString())))
	{
		if (bStoreStats)
		{
			StoreStats();
		}

		return true;
	}

	return false;
}

bool FNYSteamStatsAndAchievements::IsAchievementUnlocked(const FName AchievementName) const
{
	const FString ThisContext = FString::Printf(TEXT("IsAchievementUnlocked(Name = %s)"), *AchievementName.ToString());
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"),  *ThisContext);
		return false;
	}

	bool bUnlocked = false;
	if (UserStats->GetAchievement(TCHAR_TO_UTF8(*AchievementName.ToString()), &bUnlocked))
	{
		return bUnlocked;
	}

	return false;
}

bool FNYSteamStatsAndAchievements::GetAchievementUnlockTime(FName AchievementName, FDateTime& OutUnlockTime) const
{
	OutUnlockTime = {};
	const FString ThisContext = FString::Printf(TEXT("GetAchievementUnlockTime(AchievementName = %s)"), *AchievementName.ToString());
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	bool bUnlocked = false;
	uint32 UnlockUnixTime = 0;
	if (!UserStats->GetAchievementAndUnlockTime(TCHAR_TO_UTF8(*AchievementName.ToString()), &bUnlocked, &UnlockUnixTime))
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Failed call to GetAchievementAndUnlockTime"), *ThisContext);
		return false;
	}

	// Not unlocked can't have unix timestamp
	if (!bUnlocked)
	{
		return false;
	}

	OutUnlockTime = FDateTime::FromUnixTimestamp(UnlockUnixTime);
	return true;
}

bool FNYSteamStatsAndAchievements::GetAchievementTitle(FName AchievementName, FText& OutTitle) const
{
	FString Value;
	const bool bStatus = InternalGetAchievementDisplayAttribute(AchievementName, TEXT("name"), Value);
	OutTitle = FText::FromString(Value);
	return bStatus;
}

bool FNYSteamStatsAndAchievements::GetAchievementDescription(FName AchievementName, FText& OutDescription) const
{
	FString Value;
	const bool bStatus = InternalGetAchievementDisplayAttribute(AchievementName, TEXT("desc"), Value);
	OutDescription = FText::FromString(Value);
	return bStatus;
}

bool FNYSteamStatsAndAchievements::IsAchievementHidden(FName AchievementName) const
{
	FString Value;
	const bool bStatus = InternalGetAchievementDisplayAttribute(AchievementName, TEXT("hidden"), Value);
	return bStatus && Value == TEXT("1");
}

bool FNYSteamStatsAndAchievements::ShowAchievementProgress(FName AchievementName, int32 CurrentProgress, int32 MaxProgress) const
{
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("ShowAchievementProgress: Can't get ISteamUserStats"));
		return false;
	}

	return UserStats->IndicateAchievementProgress(TCHAR_TO_UTF8(*AchievementName.ToString()), CurrentProgress, MaxProgress);
}

bool FNYSteamStatsAndAchievements::RequestUserStats(const FUniqueNetId& PlayerId)
{
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("RequestUserStats: Can't get ISteamUserStats"));
		return false;
	}

	// TODO? FOnlineAsyncTaskSteamGetAchievements
	const FNYSteamUniqueNetId UserID{PlayerId};
	UserStats->RequestUserStats(UserID);

	return true;
}

bool FNYSteamStatsAndAchievements::RequestCurrentStats()
{
	if (bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Log, TEXT("RequestCurrentStats: Stats already received"))
		FNYSteamUserStatsReceived_Result Result;
		Result.bSuccess = true;
		UserStatsReceivedEvent.Broadcast(Result);
		return true;
	}

	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("RequestCurrentStats: Can't get ISteamUserStats"));
		return false;
	}

	return UserStats->RequestCurrentStats();
}

bool FNYSteamStatsAndAchievements::StoreStats()
{
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("StoreStats: Can't get ISteamUserStats"));
		return false;
	}

	return UserStats->StoreStats();
}

bool FNYSteamStatsAndAchievements::SetIntStat(FName StatName, int32 Data)
{
	const FString ThisContext = FString::Printf(TEXT("SetIntStat(StatName = %s, Data = %d)"), *StatName.ToString(), Data);
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	return UserStats->SetStat(TCHAR_TO_UTF8(*StatName.ToString()), Data);
}

bool FNYSteamStatsAndAchievements::SetFloatStat(FName StatName, float Data)
{
	const FString ThisContext = FString::Printf(TEXT("SetFloatStat(StatName = %s, Data = %f)"), *StatName.ToString(), Data);
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	return UserStats->SetStat(TCHAR_TO_UTF8(*StatName.ToString()), Data);
}

bool FNYSteamStatsAndAchievements::ResetAllStats(bool bAchievementsToo)
{
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("ResetAllStats: Can't get ISteamUserStats"));
		return false;
	}

	// NOTE: this calls StoreStats()
	return UserStats->ResetAllStats(bAchievementsToo);
}

bool FNYSteamStatsAndAchievements::GetIntStat(FName StatName, int32& OutData)
{
	const FString ThisContext = FString::Printf(TEXT("GetIntStat(StatName = %s)"), *StatName.ToString());
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	return UserStats->GetStat(TCHAR_TO_UTF8(*StatName.ToString()), &OutData);
}

bool FNYSteamStatsAndAchievements::GetFloatStat(FName StatName, float& OutData)
{
	const FString ThisContext = FString::Printf(TEXT("GetFloatStat(StatName = %s)"), *StatName.ToString());
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	return UserStats->GetStat(TCHAR_TO_UTF8(*StatName.ToString()), &OutData);
}

bool FNYSteamStatsAndAchievements::InternalGetAchievementDisplayAttribute(FName AchievementName, const FString& Key, FString& OutValue) const
{
	const FString ThisContext = FString::Printf(
		TEXT("InternalGetAchievementDisplayAttribute(Name = %s, Key = %s)"),
		*AchievementName.ToString(), *Key
	);
	if (!bStatsReceivedValid)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: bStatsReceivedValid = false"), *ThisContext);
		return false;
	}
	ISteamUserStats* UserStats = SteamUserStats();
	if (!UserStats)
	{
		UE_LOG(LogNYSteamAchievements, Error, TEXT("%s: Can't get ISteamUserStats"), *ThisContext);
		return false;
	}

	const char* Value = UserStats->GetAchievementDisplayAttribute(TCHAR_TO_UTF8(*AchievementName.ToString()), TCHAR_TO_UTF8(*Key));
	OutValue = UTF8_TO_TCHAR(Value);

	return false;
}
