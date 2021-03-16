// Copyright (c) Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"

#include "INYSteamSubsystem.h"

class FOnlineAsyncTask;
class FOnlineAsyncItem;
class FNYSteamAsyncTaskManager;
class FRunnableThread;

DECLARE_LOG_CATEGORY_EXTERN(LogNYSteamSubsystem, All, All)

class FNYSteamSubsystem : public INYSteamSubsystem, public FTickerObjectBase
{
	typedef FNYSteamSubsystem Self;
public:
	FNYSteamSubsystem() {}
	virtual ~FNYSteamSubsystem() {}

	// INYSteamSubsystem interface
	bool Initialize() override;
	bool Shutdown() override;
	bool IsEnabled() const override;
	bool RestartAppIfNecessary(int32 AppId) const override;
	FString GetDllAPI64Sha1Sum() const override;

	INYSteamStatsAndAchievementsPtr GetStatsAndAchievements() const override { return StatsAndAchievementsInterface;  }
	INYSteamPresencePtr GetPresence() const override { return PresenceInterface; }
	INYSteamExternalUIPtr GetExternalUI() const override { return ExternalUIInterface; }

	FString GetCurrentGameLanguage() const override;
	FString GetAppIDAsString() const override;
	uint32 GetAppIDAsUInt32() const override;

protected:
 	bool Tick(float DeltaTime) override;

	static void* InternalGetDllAPI64Sha1Sum(FString& HashString);
	static void GetHashString(uint8* HashArray, uint32 HashLength, FString& OutString);

	//
	//	Add an async task onto the task queue for processing
	// @param AsyncTask - new heap allocated task to process on the async task thread
	//
	// void QueueAsyncTask(FOnlineAsyncTask* AsyncTask);

	//
	//	Add an async task onto the outgoing task queue for processing
	// @param AsyncItem - new heap allocated task to process on the async task thread
	//
	// void QueueAsyncOutgoingItem(FOnlineAsyncItem* AsyncItem);

protected:
	// Task and thread reference
	FNYSteamAsyncTaskManager* AsyncTaskManagerRunnable = nullptr;
	FRunnableThread* AsyncTaskThread = nullptr;

	// Unreal version
	IOnlineSubsystem* UnrealSteamSubsystem = nullptr;

	// Interfaces
	INYSteamStatsAndAchievementsPtr StatsAndAchievementsInterface = nullptr;
	INYSteamPresencePtr PresenceInterface = nullptr;
	INYSteamExternalUIPtr ExternalUIInterface = nullptr;
};
