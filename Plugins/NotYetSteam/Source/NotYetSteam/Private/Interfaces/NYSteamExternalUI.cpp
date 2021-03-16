// Copyright (c) Daniel Butum. All Rights Reserved.

#include "Interfaces/NYSteamExternalUI.h"
#include "INYSteamSubsystem.h"
#include "NYSteamAPI_Internal.h"


FNYSteamExternalUI::FNYSteamExternalUI(INYSteamSubsystem* InSubsystem)
	: SteamSubsystem(InSubsystem)
{
	check(SteamSubsystem);
}


bool FNYSteamExternalUI::ActivateGameOverlayToWebPage(const FString& URL)
{
	ISteamFriends* Friends = SteamFriends();
	if (!Friends)
	{
		return false;
	}

	if (URL.StartsWith(TEXT("https://")) || URL.StartsWith(TEXT("http://")))
	{
		Friends->ActivateGameOverlayToWebPage(TCHAR_TO_UTF8(*URL));
	}
	else
	{
		// Defaults to https
		Friends->ActivateGameOverlayToWebPage(TCHAR_TO_UTF8(*FString::Printf(TEXT("https://%s"), *URL)));
	}

	return true;
}

bool FNYSteamExternalUI::IsOverlayEnabled() const
{
	ISteamUtils* Utils = SteamUtils();
	if (!Utils)
	{
		return false;
	}

	return Utils->IsOverlayEnabled();
}

bool FNYSteamExternalUI::ShowFriendsUI()
{
	ISteamFriends* Friends = SteamFriends();
	if (!Friends)
	{
		return false;
	}

	Friends->ActivateGameOverlay("friends");
	return true;
}

bool FNYSteamExternalUI::ShowAchievementsUI()
{
	ISteamFriends* Friends = SteamFriends();
	if (!Friends)
	{
		return false;
	}

	Friends->ActivateGameOverlay("achievements");
	return true;
}

bool FNYSteamExternalUI::ActivateGameOverlayToStore(uint32 AppId, bool bAddToCart, bool bShowCart)
{
	if (AppId == 0)
	{
		return false;
	}
	ISteamFriends* Friends = SteamFriends();
	if (!Friends)
	{
		return false;
	}

	EOverlayToStoreFlag Flag = k_EOverlayToStoreFlag_None;
	if (bAddToCart)
	{
		Flag = bShowCart ? k_EOverlayToStoreFlag_AddToCartAndShow : k_EOverlayToStoreFlag_AddToCart;
	}

	Friends->ActivateGameOverlayToStore(AppId, Flag);
	return true;
}
