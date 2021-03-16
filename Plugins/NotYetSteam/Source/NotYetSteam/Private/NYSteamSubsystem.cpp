// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NYSteamSubsystem.h"



#include "HAL/RunnableThread.h"
#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/SecureHash.h"

#include "Interfaces/NYSteamStatsAndAchievements.h"
#include "Interfaces/NYSteamPresence.h"
#include "Interfaces/NYSteamExternalUI.h"
#include "NYSteamAsyncTaskManager.h"
#include "NYSteamHelper.h"
#include "Misc/FileHelper.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <libloaderapi.h>
#include <minwindef.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY(LogNYSteamSubsystem)

bool FNYSteamSubsystem::Initialize()
{
	// Already initialized
	if (AsyncTaskManagerRunnable != nullptr)
	{
		return true;
	}

	UE_LOG(LogNYSteamSubsystem, Log, TEXT("Initialize"));

	UnrealSteamSubsystem = GetUnrealSteamSubsystem();
	if (!UnrealSteamSubsystem)
	{
		UE_LOG(LogNYSteamSubsystem, Error, TEXT("Initialize: failed to get Unreal Steam Subsystem"));
		return false;
	}

	UnrealSteamSubsystem->Init();

	// Try to init steam subsystem
	// if (!IsEnabled())
	// {
	// 	UE_LOG(LogNYSteamSubsystem, Warning, TEXT("Initialize: Steam is not Initialized !IsEnabled(). Trying to Init it ourselfs"));
	// 	if (UnrealSteamSubsystem && !UnrealSteamSubsystem->Init())
	// 	{
	// 		UE_LOG(LogNYSteamSubsystem, Error, TEXT("Initialize: Failed !UnrealSteamSubsystem->Init()"));
	// 		UnrealSteamSubsystem = nullptr;
	// 		return false;
	// 	}
	// }

	// Should be valid here
	// if (!IsEnabled())
	// {
	// 	UE_LOG(LogNYSteamSubsystem, Error, TEXT("Initialize: Steam is not Initialized after the second try"));
	// 	return false;
	// }

	// We can initialize here the thread and interfaces
	AsyncTaskManagerRunnable = new FNYSteamAsyncTaskManager(this);
	check(AsyncTaskManagerRunnable);
	const FString InstanceName = TEXT("NotYet");
	AsyncTaskThread = FRunnableThread::Create(AsyncTaskManagerRunnable, *FString::Printf(TEXT("NYSteamAsyncTaskThread %s"), *InstanceName), 128 * 1024, TPri_Normal);
	check(AsyncTaskThread);
	UE_LOG(LogNYSteamSubsystem, Verbose, TEXT("Created thread (ID:%d)."), AsyncTaskThread->GetThreadID());

	StatsAndAchievementsInterface = MakeShareable(new FNYSteamStatsAndAchievements(this));
	PresenceInterface = MakeShareable(new FNYSteamPresence(this));
	ExternalUIInterface = MakeShareable(new FNYSteamExternalUI(this));

	return true;
}

bool FNYSteamSubsystem::RestartAppIfNecessary(int32 AppId) const
{
#if WITH_EDITOR
	return false;
#endif

	// Check if we need to relaunch
	// This should have worked inside the Engine but epic programmers are super incompetent
	if (!GConfig)
		return false;

	// Compiled time so what we don't make  it too easy on crackers, at least 5 minutes of work ;)
	// GConfig->GetInt(TEXT("OnlineSubsystemSteam"), TEXT("SteamDevAppId"), RelaunchAppId, GEngineIni);
	// GConfig->GetBool(TEXT("OnlineSubsystemSteam"), TEXT("bRelaunchInSteam"), bRequireRelaunch, GEngineIni);
	if (AppId != 0 && SteamAPI_RestartAppIfNecessary(AppId))
	{
		if (FPlatformProperties::IsGameOnly() || FPlatformProperties::IsServerOnly())
		{
			UE_LOG(LogNYSteamSubsystem, Log, TEXT("Game restarting within Steam client, exiting"));
			FPlatformMisc::RequestExit(false);
			return true;
		}
	}

	return false;
}

FString FNYSteamSubsystem::GetDllAPI64Sha1Sum() const
{
	FString HashString;
	void* DllHandle = InternalGetDllAPI64Sha1Sum(HashString);
	FPlatformProcess::FreeDllHandle(DllHandle);
	return HashString;
}

void* FNYSteamSubsystem::InternalGetDllAPI64Sha1Sum(FString& HashString)
{
	void* DllHandle = nullptr;
#if PLATFORM_WINDOWS
	DllHandle = FPlatformProcess::GetDllHandle(TEXT("steam_api64"));
	HMODULE SteamDllHandle = (HMODULE)DllHandle;
	if (!SteamDllHandle)
	{
		return DllHandle;
	}

	// Get full path
	TCHAR Path[MAX_PATH];
	if (GetModuleFileName(SteamDllHandle, Path, sizeof(Path)) == 0)
	{
		return DllHandle;
	}

	// UE_LOG(LogNYSteamSubsystem, Log, TEXT("DLL Path: = %s"), Path);

	// FString PathStr(Path);
	// Load the data of the dll
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, Path))
	{
		return DllHandle;
	}

	// Calculate Raw SHA1 sum
	static constexpr uint32 HashLength = 20;
	uint8 HashArray[HashLength];
	FSHA1::HashBuffer(Data.GetData(), Data.Num(), HashArray);
	GetHashString(HashArray, HashLength, HashString);
#endif

	return DllHandle;
}

bool FNYSteamSubsystem::Shutdown()
{
	//if (IsEnabled())
	//{
	//	UnrealSteamSubsystem->Shutdown();
	//}
	UnrealSteamSubsystem = nullptr;

	if (StatsAndAchievementsInterface.IsValid())
	{
		StatsAndAchievementsInterface = nullptr;
	}
	if (PresenceInterface.IsValid())
	{
		PresenceInterface = nullptr;
	}
	if (ExternalUIInterface.IsValid())
	{
		ExternalUIInterface = nullptr;
	}

	if (AsyncTaskThread)
	{
		delete AsyncTaskThread;
		AsyncTaskThread = nullptr;
	}

	if (AsyncTaskManagerRunnable)
	{
		delete AsyncTaskManagerRunnable;
		AsyncTaskManagerRunnable = nullptr;
	}

	return true;
}

bool FNYSteamSubsystem::IsEnabled() const
{
	return UnrealSteamSubsystem && UnrealSteamSubsystem->IsEnabled();;
}

FString FNYSteamSubsystem::GetCurrentGameLanguage() const
{
	return UNYSteamHelper::GetCurrentGameLanguage();
}

FString FNYSteamSubsystem::GetAppIDAsString() const
{
	return UNYSteamHelper::GetAppIDAsString();
}

uint32 FNYSteamSubsystem::GetAppIDAsUInt32() const
{
	return UNYSteamHelper::GetAppIDAsUInt32();
}


// void FNYSteamSubsystem::QueueAsyncTask(FOnlineAsyncTask* AsyncTask)
// {
// 	check(AsyncTaskManagerRunnable);
// 	AsyncTaskManagerRunnable->AddToInQueue(AsyncTask);
// }

// void FNYSteamSubsystem::QueueAsyncOutgoingItem(FOnlineAsyncItem* AsyncItem)
// {
// 	check(AsyncTaskManagerRunnable);
// 	AsyncTaskManagerRunnable->AddToOutQueue(AsyncItem);
// }

bool FNYSteamSubsystem::Tick(float DeltaTime)
{
	if (!IsEnabled())
	{
		return true;
	}

	Initialize();
	if (AsyncTaskManagerRunnable)
	{
		AsyncTaskManagerRunnable->GameTick();
	}

	return true;
}

void FNYSteamSubsystem::GetHashString(uint8* HashArray, uint32 HashLength, FString& OutString)
{
	for (uint32 Idx = 0; Idx < HashLength; Idx++)
	{
		OutString += FString::Printf(TEXT("%02x"), HashArray[Idx]);
	}
}
