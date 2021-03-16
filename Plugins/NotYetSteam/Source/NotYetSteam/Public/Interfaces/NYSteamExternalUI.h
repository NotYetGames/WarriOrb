// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class INYSteamSubsystem;


// Own wrapper around FOnlineExternalUISteam
// https://partner.steamgames.com/doc/api/ISteamFriends#ActivateGameOverlay
class NOTYETSTEAM_API FNYSteamExternalUI
{
public:
	FNYSteamExternalUI(INYSteamSubsystem* InSubsystem);
	virtual ~FNYSteamExternalUI() {}

	virtual bool IsOverlayEnabled() const;
	virtual bool ShowAchievementsUI();
	virtual bool ShowFriendsUI();

	virtual bool ActivateGameOverlayToWebPage(const FString& URL);
	virtual bool ActivateGameOverlayToStore(uint32 AppId, bool bAddToCart = false, bool bShowCart = false);

private:
	/** Reference to the owning subsystem */
	INYSteamSubsystem* SteamSubsystem = nullptr;
};

typedef TSharedPtr<FNYSteamExternalUI, ESPMode::ThreadSafe> INYSteamExternalUIPtr;
