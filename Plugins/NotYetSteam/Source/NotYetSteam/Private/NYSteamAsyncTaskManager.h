// Copyright (c) Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"

#include "NYSteamAPI_Internal.h"

class FNYSteamSubsystem;

class FNYSteamAsyncTaskManager : public FOnlineAsyncTaskManager
{
	typedef FNYSteamAsyncTaskManager ThisClass;
public:
	FNYSteamAsyncTaskManager(FNYSteamSubsystem* InSteamSubsystem) :
		OnUserStatsStoredCallback(this, &ThisClass::OnUserStatsStored),
		OnUserStatsReceivedCallback(this, &ThisClass::OnUserStatsReceived),
		OnUserStatsUnloadedCallback(this, &ThisClass::OnUserStatsUnloaded),
		OnUserAchievementStoredCallback(this, &ThisClass::OnUserAchievementStored),
		SteamSubsystem(InSteamSubsystem)
	{}

	~FNYSteamAsyncTaskManager()
	{
	}

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;

protected:

	//
	// Create Steam callback
	//

	// Result of a request to store the user stats.
	STEAM_CALLBACK(ThisClass, OnUserStatsStored, UserStatsStored_t, OnUserStatsStoredCallback);

	// Called when the latest stats and achievements for a specific user (including the local user) have been received from the server.
	STEAM_CALLBACK(ThisClass, OnUserStatsReceived, UserStatsReceived_t, OnUserStatsReceivedCallback);

	// Callback indicating that a user's stats have been unloaded.
	STEAM_CALLBACK(ThisClass, OnUserStatsUnloaded, UserStatsUnloaded_t, OnUserStatsUnloadedCallback);

	// Result of a request to store the achievements on the server, or an "indicate progress" call.
	STEAM_CALLBACK(ThisClass, OnUserAchievementStored, UserAchievementStored_t, OnUserAchievementStoredCallback);

	// Cached reference to the main online subsystem
	FNYSteamSubsystem* SteamSubsystem = nullptr;
};
