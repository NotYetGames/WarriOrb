// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoOnlineCloud.h"

#if WARRIORB_WITH_STEAM
#include "INotYetSteamModule.h"
#endif // WARRIORB_WITH_STEAM
#include "Basic/Helpers/SoMathHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoOnlineCloud, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoOnlineCloud::Initiallize(TSharedPtr<const FUniqueNetId> InUserID)
{
	UE_LOG(LogSoOnlineCloud, Log, TEXT("Initiallize"));

	UserID = InUserID;
	bIsInitialized = false;

#if WARRIORB_WITH_STEAM
	// const INYSteamSubsystemPtr Steam = INotYetSteamModule::Get().GetSteamSubsystem();
	// if (!Steam.IsValid())
	// 	return ;

	// SteamUserCloud = Steam->GetUserCloud();
	// if (SteamUserCloud.IsValid())
	// {
	// 	UE_LOG(LogSoOnlineCloud, Log, TEXT("Initiallize: Steam UserCloud is available"));

	// 	SteamUserCloud->OnEnumerateUserFilesCompleteDelegates.AddRaw(this, &Self::HandleOnSteamEnumerateUserFilesComplete);
	// 	SteamUserCloud->EnumerateUserFiles(*UserID);
	// 	DumpCloudState();
	// 	DumpCloudFilesState();
	// }
	// else
	// {
	// 	UE_LOG(LogSoOnlineCloud, Warning, TEXT("Initiallize: SteamUserCloud is DISABLED"));
	// }
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoOnlineCloud::Shutdown()
{
#if WARRIORB_WITH_STEAM
	// SteamUserCloud.Reset();
	UserID.Reset();
#endif

	UE_LOG(LogSoOnlineCloud, Log, TEXT("Shutdown"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoOnlineCloud::IsSteamCloudEnabled() const
{
#if WARRIORB_WITH_STEAM
	// if (!SteamUserCloud.IsValid())
	// 	return false;
	//
	// return SteamUserCloud->IsCloudEnabledForAccount() && SteamUserCloud->IsCloudEnabledForApp();
	return false;
#else
	return false;
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoOnlineCloud::HandleOnSteamEnumerateUserFilesComplete(bool bWasSuccessful, const FUniqueNetId& HandleUserID)
{
#if WARRIORB_WITH_STEAM
	// if (HandleUserID != *UserID)
	// {
	// 	UE_LOG(LogSoOnlineCloud, Warning, TEXT("HandleOnSteamEnumerateUserFilesComplete: Not our user ID. Ignoring."));
	// 	return;
	// }
	// if (!bWasSuccessful)
	// {
	// 	UE_LOG(LogSoOnlineCloud, Error, TEXT("HandleOnSteamEnumerateUserFilesComplete: NOT successful :("));
	// 	return;
	// }
	// if (!IsSteamCloudEnabled())
	// {
	// 	UE_LOG(LogSoOnlineCloud, Warning, TEXT("HandleOnSteamEnumerateUserFilesComplete: Steam cloud not enabled. Ignoring."));
	// 	return;
	// }
	//
	// SteamUserCloud->GetUserFileList(*UserID, UserFiles);
	// DumpCloudState();
	// DumpCloudFilesState();
#endif // WARRIORB_WITH_STEAM

	bIsInitialized = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoOnlineCloud::DumpCloudState()
{
#if WARRIORB_WITH_STEAM
	// if (!SteamUserCloud.IsValid())
	// 	return;
	//
	// uint64 TotalBytes = 0, AvailableBytes = 0;
	// SteamUserCloud->GetQuota(TotalBytes, AvailableBytes);
	// UE_LOG(LogSoOnlineCloud, Verbose, TEXT("Steam Disk Quota: %f MB / %f MB"), USoMathHelper::BytesToMegaBytes(AvailableBytes), USoMathHelper::BytesToMegaBytes(TotalBytes));
	// UE_LOG(LogSoOnlineCloud, Verbose, TEXT("Game does %shave cloud storage enabled."), SteamUserCloud->IsCloudEnabledForApp() ? TEXT("") : TEXT("NOT "));
	// UE_LOG(LogSoOnlineCloud, Verbose, TEXT("User does %shave cloud storage enabled."), SteamUserCloud->IsCloudEnabledForAccount() ? TEXT("") : TEXT("NOT "));
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoOnlineCloud::DumpCloudFileState(const FString& FileName)
{
#if WARRIORB_WITH_STEAM
	// if (!SteamUserCloud.IsValid())
	// 	return;
	//
	// SteamUserCloud->DumpCloudFileState(*UserID, FileName);
#endif // WARRIORB_WITH_STEAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoOnlineCloud::DumpCloudFilesState()
{
#if WARRIORB_WITH_STEAM
	// if (!IsInitialized())
	// {
	// 	UE_LOG(LogSoOnlineCloud, Warning, TEXT("DumpCloudFilesState: Steam clouds are not initialized. Ignoring."));
	// 	return;
	// }
	//
	// for (const FCloudFileHeader& Header : UserFiles)
	// {
	// 	DumpCloudFileState(Header.FileName);
	// }

#endif // WARRIORB_WITH_STEAM
}
