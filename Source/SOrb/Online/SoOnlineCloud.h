// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"

#if WARRIORB_WITH_ONLINE
#include "OnlineSubsystemTypes.h"
#endif // WARRIORB_WITH_ONLINE

#include "SoOnlineCloud.generated.h"


USTRUCT()
struct FSoOnlineCloud
{
	GENERATED_USTRUCT_BODY()
	typedef FSoOnlineCloud Self;

public:
	FSoOnlineCloud() {}

	void Initiallize(TSharedPtr<const FUniqueNetId> InUserID);
	void Shutdown();

	bool IsInitialized() const { return bIsInitialized;  }
	bool IsSteamCloudEnabled() const;

	void DumpCloudState();
	void DumpCloudFileState(const FString& FileName);
	void DumpCloudFilesState();

protected:
	void HandleOnSteamEnumerateUserFilesComplete(bool bWasSuccessful, const FUniqueNetId& HandleUserID);

protected:
#if WARRIORB_WITH_STEAM
	// TSharedPtr<UObject> SteamUserCloud = nullptr;
#endif

	// Enumerate files finished at least once
	bool bIsInitialized = false;

	// Online user id
	TSharedPtr<const FUniqueNetId> UserID = nullptr;

#if WARRIORB_WITH_ONLINE
	// All the cloud user files metadata
	TArray<FCloudFileHeader> UserFiles;
#endif // WARRIORB_WITH_ONLINE
};
