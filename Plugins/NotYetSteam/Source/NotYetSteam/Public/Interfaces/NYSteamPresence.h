// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"

class INYSteamSubsystem;


// https://partner.steamgames.com/doc/api/ISteamFriends#SetRichPresence
// FOnlineUserPresenceSteam
class NOTYETSTEAM_API FNYSteamPresence
{
public:
	FNYSteamPresence(INYSteamSubsystem* InSubsystem);
	virtual ~FNYSteamPresence() {}

	// Clears the rich presence
	virtual bool ClearRichPresence(const FUniqueNetId& PlayerId);

	// Helper that only sets the presence key for "status"
	// will show up in the 'view game info' dialog in the Steam friends list.
	// NOTE: you need to call clear rich presence before calling this yourself
	virtual bool SetRichPresenceStatus(const FUniqueNetId& PlayerId, const FString& Message);

	// Helper that allows you to set the "steam_display" key
	// Token names begin a # and can contain any alphanumeric character and underscores.
	// If steam_display is not set to a valid localization tag, then rich presence will not be displayed in the Steam client.
	virtual bool SetRichPresenceSteamDisplay(const FUniqueNetId& PlayerId, const FName Token, const TMap<FString, FText>& Substitutions);

protected:
	bool SetSteamRichPresence(const FString& Key, const FString& Value);

protected:
	// Reference to the owning subsystem
	INYSteamSubsystem* SteamSubsystem = nullptr;
};

typedef TSharedPtr<FNYSteamPresence, ESPMode::ThreadSafe> INYSteamPresencePtr;
