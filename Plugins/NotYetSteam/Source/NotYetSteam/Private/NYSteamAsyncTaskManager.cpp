// Copyright (c) Daniel Butum. All Rights Reserved.

#include "NYSteamAsyncTaskManager.h"
#include "NYSteamSubsystem.h"
#include "NYSteamHelper.h"
#include "NYSteamResult.h"
#include "NYSteamAsyncEvents.h"

void FNYSteamAsyncTaskManager::OnlineTick()
{
	// NOTE: this is done by the FOnlineAsyncTaskManagerSteam::OnlineTick()
	check(SteamSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId);

	// if (SteamSubsystem->IsSteamClientAvailable())
	// {
	// 	SteamAPI_RunCallbacks();
	// }

	// if (SteamSubsystem->IsSteamServerAvailable())
	// {
	// 	SteamGameServer_RunCallbacks();
	// }
}


void FNYSteamAsyncTaskManager::OnUserStatsStored(UserStatsStored_t* CallbackData)
{
	// NOTE: not on the game thread
	// Wrong game?
	if (!UNYSteamHelper::IsCurrentGame(CallbackData->m_nGameID))
	{
		UE_LOG(LogNYSteamSubsystem, Warning, TEXT("OnUserStatsStored: but for wrong game. Ignoring"));
		return;
	}
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("OnUserStatsStored"));

	// Validate
	const FNYSteamUniqueNetId UserID = UNYSteamHelper::GetSteamUserId();
	if (!FNYSteamResultHelper::IsResultSuccess(CallbackData->m_eResult))
	{
		UE_LOG(
			LogNYSteamSubsystem,
			Error,
			TEXT("OnUserStatsStored: Error = %s"),
			*FNYSteamResultHelper::ResultToString(CallbackData->m_eResult)
		);
		if (CallbackData->m_eResult == k_EResultFail)
		{
			UE_LOG(
				LogNYSteamSubsystem,
				Error,
				TEXT("Invalid stats data set, stats have been reverted to state prior to last write.")
			);
		}
		if (CallbackData->m_eResult == k_EResultInvalidParam)
		{
			UE_LOG(
				LogNYSteamSubsystem,
				Error,
				TEXT("One or more stats we set broke a constraint. Invalid stats data set, stats have been reverted to state prior to last write.")
			);
		}
	}

	// Fire event
	auto* NewEvent = new FNYSteamAsyncStatsStoredEvent(SteamSubsystem, UserID, *CallbackData);
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("%s"), *NewEvent->ToString());
	AddToOutQueue(NewEvent);
}

void FNYSteamAsyncTaskManager::OnUserStatsReceived(UserStatsReceived_t* CallbackData)
{
	// NOTE: not on the game thread
	// Wrong game?
	if (!UNYSteamHelper::IsCurrentGame(CallbackData->m_nGameID))
	{
		UE_LOG(LogNYSteamSubsystem, Warning, TEXT("OnUserStatsReceived: but for wrong game. Ignoring"));
		return;
	}
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("OnUserStatsReceived"));

	// Validate
	const FNYSteamUniqueNetId UserID{CallbackData->m_steamIDUser};
	if (!FNYSteamResultHelper::IsResultSuccess(CallbackData->m_eResult))
	{
		UE_LOG(
			LogNYSteamSubsystem,
			Error,
			TEXT("Failed to obtain steam user stats, user: %s error: %s"),
			*UserID.ToDebugString(), *FNYSteamResultHelper::ResultToString(CallbackData->m_eResult)
		);
		if (CallbackData->m_eResult == k_EResultFail)
		{
			UE_LOG(
				LogNYSteamSubsystem,
				Error,
				TEXT("OnUserStatsReceived: Failed to obtain steam user stats, user: %s has no stats entries"),
				*UserID.ToDebugString()
			);
		}
	}

	// Fire Event
	auto* NewEvent = new FNYSteamAsyncStatsReceivedEvent(SteamSubsystem, UserID, *CallbackData);
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("%s"), *NewEvent->ToString());
	AddToOutQueue(NewEvent);
}

void FNYSteamAsyncTaskManager::OnUserStatsUnloaded(UserStatsUnloaded_t* CallbackData)
{
	// NOTE: not on the game thread
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("OnUserStatsUnloaded"));

	// Fire Event
	const FNYSteamUniqueNetId UserID{CallbackData->m_steamIDUser};
	auto* NewEvent = new FNYSteamAsyncStatsUnloadedEvent(SteamSubsystem, UserID, *CallbackData);
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("%s"), *NewEvent->ToString());
	AddToOutQueue(NewEvent);
}

void FNYSteamAsyncTaskManager::OnUserAchievementStored(UserAchievementStored_t* CallbackData)
{
	// NOTE: not on the game thread
	// Wrong game?
	if (!UNYSteamHelper::IsCurrentGame(CallbackData->m_nGameID))
	{
		UE_LOG(LogNYSteamSubsystem, Warning, TEXT("OnUserAchievementStored: but for wrong game. Ignoring"));
		return;
	}
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("OnUserAchievementStored"));

	// Fire Event
	auto* NewEvent = new FNYSteamAsyncAchievementStoredEvent(SteamSubsystem, *CallbackData);
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("%s"), *NewEvent->ToString());
	AddToOutQueue(NewEvent);

}
