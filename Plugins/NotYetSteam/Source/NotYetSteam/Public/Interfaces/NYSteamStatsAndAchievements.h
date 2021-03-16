// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineStats.h"
#include "UObject/CoreOnline.h"

#include "Interfaces/OnlineAchievementsInterface.h"
#include "NYSteamResult.h"

class INYSteamSubsystem;

// https://partner.steamgames.com/doc/api/ISteamUserStats#UserAchievementStored_t

// Called when the latest stats and achievements for a specific user (including the local user) have been received from the server.
DECLARE_MULTICAST_DELEGATE_OneParam(FNYOnUserStatsReceivedEvent, FNYSteamUserStatsReceived_Result);

// Result of a request to store the user stats.
DECLARE_MULTICAST_DELEGATE_OneParam(FNYOnUserStatsStoredEvent, FNYSteamUserStatsStored_Result);

// Callback indicating that a user's stats have been unloaded.
DECLARE_MULTICAST_DELEGATE_OneParam(FNYOnUserStatsUnloadedEvent, FNYSteamUserStatsUnloaded_Result);

// Result of a request to store the achievements on the server, or an "indicate progress" call.
// If both m_nCurProgress and m_nMaxProgress are zero, that means the achievement has been fully unlocked.
DECLARE_MULTICAST_DELEGATE_OneParam(FNYOnUserAchievementStoredEvent, FNYSteamUserAchievementStored_Result);


/** Structure describing a Steam achievement */
struct FNYUnrealSteamAchievementConfig
{
public:
	FNYUnrealSteamAchievementConfig(const FString& InRawString) : Name(*InRawString) {}

	/** Returns debugging string to print out achievement info */
	FString ToDebugString() const
	{
		return FString::Printf(TEXT("Achievement:{%s}"), *Name.ToString());
	}

public:
	// The Name of the achievement
	FName Name;
};

/**
 * A helper class for configuring achievements in ini
 */
struct FNYSteamAchievementsConfigReader
{

public:
	/**
	 * Create a config using the default values:
	 * IniName - GEngineIni
	 */
	FNYSteamAchievementsConfigReader();

	/** Returns empty string if couldn't read */
	FString GetKey(const FString& KeyName);
	bool ReadAchievements(TArray<FNYUnrealSteamAchievementConfig>& OutArray);

public:
	/** Ini file name to find the config values */
	FString IniName;

	/** Section name for Steam */
	FString SectionName;
};

//
// Also handles Steam stats
// NOTE: all methods that do not have the PlayerId parameter assumes they call the current user
class NOTYETSTEAM_API FNYSteamStatsAndAchievements
{
	typedef FNYSteamStatsAndAchievements ThisClass;

public:
	FNYSteamStatsAndAchievements(INYSteamSubsystem* InSubsystem);
	virtual ~FNYSteamStatsAndAchievements() {}


	// What we identify the current user by
	// virtual void SetCurrentUserID(const FUniqueNetId& PlayerId);

	//
	// Achievements
	//

	virtual bool UnlockAchievement(FName AchievementName);

	// NOTE: Dangerous, use with care
	virtual bool ResetAllAchievements();
	virtual bool ResetAchievement(FName AchievementName, bool bStoreStats = true);

	// Tells if an Achievement was unlocked
	virtual bool IsAchievementUnlocked(FName AchievementName) const;

	// Gets the time the achievement was unlocked, if it was not unlocked then this returns false
	virtual bool GetAchievementUnlockTime(FName AchievementName, FDateTime& OutUnlockTime) const;
	virtual bool GetAchievementTitle(FName AchievementName, FText& OutTitle) const;
	virtual bool GetAchievementDescription(FName AchievementName, FText& OutDescription) const;
	virtual bool IsAchievementHidden(FName AchievementName) const;

	// Gets all the achievements read from the config file
	virtual bool GetAllAchievementNamesFromUnrealConfig(TArray<FName>& OutArray) const
	{
		OutArray = {};
		for (const auto& Config : UnrealConfigAchievements)
		{
			OutArray.Add(Config.Name);
		}

		return OutArray.Num() > 0;
	}

	// Shows the user a pop-up notification with the current progress of an achievement.
	virtual bool ShowAchievementProgress(FName AchievementName, int32 CurrentProgress, int32 MaxProgress) const;


	//
	// Stats
	//

	// Set to true after either RequestUserStats or RequestCurrentStats
	virtual bool HasValidStatsReceived() const { return bStatsReceivedValid; }

	// Async request
	virtual bool RequestUserStats(const FUniqueNetId& PlayerId);
	virtual bool RequestCurrentStats();



	// Store stats, this function is async
	// https://partner.steamgames.com/doc/api/ISteamUserStats#StoreStats
	virtual bool StoreStats();

	// https://partner.steamgames.com/doc/api/ISteamUserStats#SetStat
	virtual bool SetIntStat(FName StatName, int32 Data);
	virtual bool SetFloatStat(FName StatName, float Data);
	virtual bool GetIntStat(FName StatName, int32& OutData);
	virtual bool GetFloatStat(FName StatName, float& OutData);

	// NOTE: Dangerous, you should resync your stats after this
	virtual bool ResetAllStats(bool bAchievementsToo = false);

	//
	// Events
	//

	virtual FNYOnUserStatsReceivedEvent& OnUserStatsReceived() { return UserStatsReceivedEvent; }
	virtual FNYOnUserStatsStoredEvent& OnUserStatsStored() { return UserStatsStoredEvent; };
	virtual FNYOnUserStatsUnloadedEvent& OnUserStatsUnloaded() { return UserStatsUnloadedEvent; };
	virtual FNYOnUserAchievementStoredEvent& OnUserAchievementStored() { return UserAchievementStoredEvent; };

	// Callback handles
	void OnUserStatsReceived(FNYSteamUserStatsReceived_Result Result);
	void OnUserStatsStored(FNYSteamUserStatsStored_Result Result);
	void OnUserStatsUnloaded(FNYSteamUserStatsUnloaded_Result Result);
	void OnUserAchievementStored(FNYSteamUserAchievementStored_Result Result);

protected:
	// Calls SteamUserStats()->GetAchievementDisplayAttribute
	// See https://partner.steamgames.com/doc/api/ISteamUserStats#GetAchievementDisplayAttribute
	bool InternalGetAchievementDisplayAttribute(FName AchievementName, const FString& Key, FString& OutValue) const;

protected:
	/** Reference to the owning subsystem */
	INYSteamSubsystem* SteamSubsystem = nullptr;

	// Achievements configured in the config (not player-specific)
	TArray<FNYUnrealSteamAchievementConfig> UnrealConfigAchievements;

	//
	// bool bRequestedStats = false;

	// Did we receive user stats and achievements from steam?
	// NOTE: for multiple players we need multiple of these
	bool bStatsReceivedValid = false;

	//
	// Events
	//

	FNYOnUserStatsReceivedEvent UserStatsReceivedEvent;
	FNYOnUserStatsStoredEvent UserStatsStoredEvent;
	FNYOnUserStatsUnloadedEvent UserStatsUnloadedEvent;
	FNYOnUserAchievementStoredEvent UserAchievementStoredEvent;
};

typedef TSharedPtr<FNYSteamStatsAndAchievements, ESPMode::ThreadSafe> INYSteamStatsAndAchievementsPtr;
