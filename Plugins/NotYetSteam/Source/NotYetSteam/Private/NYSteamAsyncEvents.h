// Copyright (c) Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"
#include "Templates/SharedPointer.h"
#include "OnlineAsyncTaskManager.h"
#include "NYSteamSubsystem.h"
#include "NYSteamUniqueNetId.h"
#include "NYSteamResult.h"
#include "Interfaces/NYSteamStatsAndAchievements.h"

#include "NYSteamAPI_Internal.h"

// For https://partner.steamgames.com/doc/api/ISteamUserStats#UserStatsStored_t
class FNYSteamAsyncStatsStoredEvent : public FOnlineAsyncEvent<FNYSteamSubsystem>
{
	typedef FOnlineAsyncEvent Super;

private:
	const FNYSteamUniqueNetId UserID;
	UserStatsStored_t CallbackResult;

	/** Hidden on purpose */
	FNYSteamAsyncStatsStoredEvent() :
		Super(nullptr),
		UserID(0),
		CallbackResult{0}
	{
	}

public:
	FNYSteamAsyncStatsStoredEvent(FNYSteamSubsystem* InSubsystem, const FNYSteamUniqueNetId& InUserId, const UserStatsStored_t& InResult) :
		Super(InSubsystem),
		UserID(InUserId),
		CallbackResult(InResult)
	{
	}

	virtual ~FNYSteamAsyncStatsStoredEvent() {}

	FString ToString() const override
	{
		return FString::Printf(
			TEXT("FNYSteamAsyncStatsStoredEvent bWasSuccessful: %d User: %s Result: %s"),
			FNYSteamResultHelper::IsResultSuccess(CallbackResult.m_eResult),
			*UserID.ToDebugString(), *FNYSteamResultHelper::ResultToString(CallbackResult.m_eResult)
		);
	}

	void Finalize() override
	{
		Super::Finalize();
	}
	void TriggerDelegates() override
	{
		Super::TriggerDelegates();

		FNYSteamUserStatsStored_Result Result;
		Result.bSuccess = FNYSteamResultHelper::IsResultSuccess(CallbackResult.m_eResult);

		INYSteamStatsAndAchievementsPtr StatsAndAchievements =
			StaticCastSharedPtr<FNYSteamStatsAndAchievements>(Subsystem->GetStatsAndAchievements());
		StatsAndAchievements->OnUserStatsStored(Result);
	}
};

// For https://partner.steamgames.com/doc/api/ISteamUserStats#UserStatsReceived_t
class FNYSteamAsyncStatsReceivedEvent : public FOnlineAsyncEvent<FNYSteamSubsystem>
{
	typedef FOnlineAsyncEvent Super;

private:
	const FNYSteamUniqueNetId UserID;
	UserStatsReceived_t CallbackResult;

	/** Hidden on purpose */
	FNYSteamAsyncStatsReceivedEvent() :
		UserID(0),
		CallbackResult{0}
	{}

public:
	FNYSteamAsyncStatsReceivedEvent(FNYSteamSubsystem* InSubsystem, const FNYSteamUniqueNetId& InUserId, const UserStatsReceived_t& InResult) :
		FOnlineAsyncEvent(InSubsystem),
		UserID(InUserId),
		CallbackResult(InResult)
	{
	}
	virtual ~FNYSteamAsyncStatsReceivedEvent() {}

	/**
	 *	Get a human readable description of task
	 */
	FString ToString() const override
	{
		return FString::Printf(
			TEXT("FNYSteamAsyncStatsReceivedEvent bWasSuccessful: %d User: %s Result: %s"),
			FNYSteamResultHelper::IsResultSuccess(CallbackResult.m_eResult),
			*UserID.ToDebugString(), *FNYSteamResultHelper::ResultToString(CallbackResult.m_eResult)
		);
	}

	void Finalize() override
	{
		Super::Finalize();
	}
	void TriggerDelegates() override
	{
		Super::TriggerDelegates();

		FNYSteamUserStatsReceived_Result Result;
		Result.UserID = CallbackResult.m_steamIDUser.ConvertToUint64();
		Result.bSuccess = FNYSteamResultHelper::IsResultSuccess(CallbackResult.m_eResult);
		Result.bIsCurrentUser = UNYSteamHelper::IsCurrentUser(Result.UserID);

		INYSteamStatsAndAchievementsPtr StatsAndAchievements =
			StaticCastSharedPtr<FNYSteamStatsAndAchievements>(Subsystem->GetStatsAndAchievements());
		StatsAndAchievements->OnUserStatsReceived(Result);
	}
};


// For https://partner.steamgames.com/doc/api/ISteamUserStats#UserStatsUnloaded_t
class FNYSteamAsyncStatsUnloadedEvent : public FOnlineAsyncEvent<FNYSteamSubsystem>
{
	typedef FOnlineAsyncEvent Super;

private:
	const FNYSteamUniqueNetId UserID;
	UserStatsUnloaded_t CallbackResult;

	/** Hidden on purpose */
	FNYSteamAsyncStatsUnloadedEvent() :
		Super(nullptr),
		UserID(0)
	{
		CallbackResult.m_steamIDUser.SetFromUint64(0);
	}

public:
	FNYSteamAsyncStatsUnloadedEvent(FNYSteamSubsystem* InSubsystem, const FNYSteamUniqueNetId& InUserId, const UserStatsUnloaded_t& InResult) :
		Super(InSubsystem),
		UserID(InUserId),
		CallbackResult(InResult)
	{
	}
	virtual ~FNYSteamAsyncStatsUnloadedEvent() {}

	FString ToString() const override
	{
		return FString::Printf(
			TEXT("FNYSteamAsyncStatsUnloadedEvent User: %s"),
			*UserID.ToDebugString()
		);
	}

	void Finalize() override
	{
		Super::Finalize();
	}
	void TriggerDelegates() override
	{
		Super::TriggerDelegates();

		FNYSteamUserStatsUnloaded_Result Result;
		Result.bSuccess = true;
		Result.UserID = CallbackResult.m_steamIDUser.ConvertToUint64();
		Result.bIsCurrentUser = UNYSteamHelper::IsCurrentUser(Result.UserID);

		INYSteamStatsAndAchievementsPtr StatsAndAchievements =
			StaticCastSharedPtr<FNYSteamStatsAndAchievements>(Subsystem->GetStatsAndAchievements());
		StatsAndAchievements->OnUserStatsUnloaded(Result);
	}
};


// For https://partner.steamgames.com/doc/api/ISteamUserStats#UserAchievementStored_t
class FNYSteamAsyncAchievementStoredEvent : public FOnlineAsyncEvent<FNYSteamSubsystem>
{
	typedef FOnlineAsyncEvent Super;

private:
	UserAchievementStored_t CallbackResult;
	FNYSteamUserAchievementStored_Result Result;

	/** Hidden on purpose */
	FNYSteamAsyncAchievementStoredEvent() :
		Super(nullptr)
	{
	}

public:
	FNYSteamAsyncAchievementStoredEvent(FNYSteamSubsystem* InSubsystem, const UserAchievementStored_t& InResult) :
		Super(InSubsystem),
		CallbackResult(InResult)
	{
		Result.bSuccess = true;
		Result.AchievementName = *FString(UTF8_TO_TCHAR(CallbackResult.m_rgchAchievementName));
		Result.CurrentProgress = CallbackResult.m_nCurProgress;
		Result.MaxProgress = CallbackResult.m_nMaxProgress;
	}

	virtual ~FNYSteamAsyncAchievementStoredEvent() {}

	FString ToString() const override
	{
		return FString::Printf(
			TEXT("FNYSteamAsyncAchievementStoredEvent AchievementName = %s, CurrentProgress = %d, MaxProgress = %d"),
			*Result.AchievementName.ToString(), Result.CurrentProgress, Result.MaxProgress
		);
	}

	void Finalize() override
	{
		Super::Finalize();
	}
	void TriggerDelegates() override
	{
		Super::TriggerDelegates();

		INYSteamStatsAndAchievementsPtr StatsAndAchievements =
			StaticCastSharedPtr<FNYSteamStatsAndAchievements>(Subsystem->GetStatsAndAchievements());
		StatsAndAchievements->OnUserAchievementStored(Result);
	}
};
