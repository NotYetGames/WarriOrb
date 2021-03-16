
// Copyright 2019 Daniel Butum

#include "NYSteamHelper.h"

#include "NYSteamAPI_Internal.h"
#include "INotYetSteamModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogNYSteamHelper, All, All)

bool UNYSteamHelper::Initialize(int32 AppId, bool bRequireRelaunch)
{
	INYSteamSubsystemPtr Steam = INotYetSteamModule::Get().GetSteamSubsystem();
	if (!Steam.IsValid())
	{
		UE_LOG(LogNYSteamHelper, Error, TEXT("Initialize: INYSteamSubsystemPtr is not valid"));
		return false;
	}

	if (bRequireRelaunch && Steam->RestartAppIfNecessary(AppId))
	{
		return false;
	}

	return Steam->Initialize();

	// if (!SteamAPI_Init())
	// {
	// 	UE_LOG(LogNYSteamHelper, Warning, TEXT("Steam failed to initialize (SteamAPI_Init() failed)"));
	// 	return false;
	// }

	// ISteamController* Controller = SteamController();
	// if (!Controller)
	// {
	// 	UE_LOG(LogNYSteamHelper, Warning, TEXT("Failed to get SteamController()"));
	// 	return false;
	// }

	// // TODO: why it crashes here?
	// // Tried:
	// // 1. Removing the steam dll from the engine
	// // 2. Replacing the dll from the engine with the updated one
	// // 3. Renaming our dll to notyet_steam.dll and using that
	// // Apparently UE does not like we have our own steam library version :(
	// if (!SteamController()->Init())
	// {
	// 	UE_LOG(LogNYSteamHelper, Warning, TEXT("Failed SteamController()->Init()"));
	// 	return false;
	// }

	// UE_LOG(LogNYSteamHelper, Log, TEXT("Initialized Steam SDK"));
}

bool UNYSteamHelper::IsCurrentUser(uint64 UserID)
{
	if (!SteamUser())
	{
		return false;
	}

	return GetCSteamID() == UserID;
}

bool UNYSteamHelper::IsCurrentUser(const FUniqueNetId& UserID)
{
	if (!SteamUser())
	{
		return false;
	}

	// NOTE: we do not need this as this assumes we can't ever be in offline mode
	//if (!User->BLoggedOn())
	//{
	//	return false;
	//}

	const CSteamID SteamId = ConvertUniqueNetIdToUInt64(UserID);
	return GetCSteamID() == SteamId;
}

bool UNYSteamHelper::IsCurrentGame(uint64 GameID)
{
	if (!SteamUser())
	{
		return false;
	}

	return GameID == GetCGameID().ToUint64();
}

CSteamID UNYSteamHelper::GetCSteamID()
{
	ISteamUser* User = SteamUser();
	if (!User)
	{
		return k_steamIDNil;
	}

	return User->GetSteamID();
}

uint32 UNYSteamHelper::GetAppIDAsUInt32()
{
	ISteamUtils* Utils = SteamUtils();
	if (!Utils)
	{
		return 0;
	}

	return Utils->GetAppID();
}

FString UNYSteamHelper::GetDllAPI64Sha1Sum()
{
	INYSteamSubsystemPtr Steam = INotYetSteamModule::Get().GetSteamSubsystem();
	if (!Steam.IsValid())
	{
		return TEXT("");
	}

	return Steam->GetDllAPI64Sha1Sum();
}

FString UNYSteamHelper::GetCurrentGameLanguage()
{
	ISteamApps* Apps = SteamApps();
	if (!Apps)
	{
		return TEXT("");
	}

	return FString(ANSI_TO_TCHAR(Apps->GetCurrentGameLanguage()));
}
