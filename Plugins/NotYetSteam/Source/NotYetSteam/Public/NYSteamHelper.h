// Copyright 2019 Daniel Butum

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/CoreOnline.h"

#include "NYSteamUniqueNetId.h"

#include "NYSteamHelper.generated.h"

//
// NOTE: all functions without a player id/user id assumes the local current user (usually 0)
// And it assumes the running app
UCLASS()
class NOTYETSTEAM_API UNYSteamHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static bool Initialize(int32 AppId, bool bRequireRelaunch);

	FORCEINLINE static uint64 ConvertUniqueNetIdToUInt64(const FUniqueNetId& PlayerId)
	{
		return *reinterpret_cast<const uint64*>(PlayerId.GetBytes());
	}

	static bool IsCurrentUser(uint64 UserID);
	static bool IsCurrentUser(const FUniqueNetId& UserID);
	static bool IsCurrentGame(uint64 GameID);

	// Of the current running app
	static CSteamID GetCSteamID();
	static CGameID GetCGameID() { return CGameID(GetAppIDAsUInt32()); }

	static FNYSteamUniqueNetId GetSteamUserId() { return FNYSteamUniqueNetId(GetCSteamID()); }
	static FString GetAppIDAsString() { return FString::Printf(TEXT("%d"), GetAppIDAsUInt32()); }
	static uint32 GetAppIDAsUInt32();

	static FString GetDllAPI64Sha1Sum();

	// Assumes local User 0
	static FString GetCurrentGameLanguage();
};
