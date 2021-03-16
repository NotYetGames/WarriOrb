// Copyright (c) Daniel Butum. All Rights Reserved.

#include "Interfaces/NYSteamPresence.h"
#include "NYSteamAPI_Internal.h"
#include "NYSteamHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogNYSteamPresence, All, All)


FNYSteamPresence::FNYSteamPresence(INYSteamSubsystem* InSubsystem)
	: SteamSubsystem(InSubsystem)
{
	check(SteamSubsystem);
}

bool FNYSteamPresence::ClearRichPresence(const FUniqueNetId& PlayerId)
{
	if (!UNYSteamHelper::IsCurrentUser(PlayerId))
	{
		UE_LOG(LogNYSteamPresence, Warning, TEXT("ClearRichPresence: Not current steam user"));
		return false;
	}

	ISteamFriends* SteamFriendsPtr = SteamFriends();
	if (!SteamFriendsPtr)
	{
		UE_LOG(LogNYSteamPresence, Warning, TEXT("ClearRichPresence: Can't get ISteamFriends"));
		return false;
	}

	SteamFriendsPtr->ClearRichPresence();
	return true;
}

bool FNYSteamPresence::SetRichPresenceStatus(const FUniqueNetId& PlayerId, const FString& Message)
{
	static const FString StatusKey = TEXT("status");
	if (!UNYSteamHelper::IsCurrentUser(PlayerId))
	{
		UE_LOG(LogNYSteamPresence, Warning, TEXT("ClearPresence: Not current steam user"));
		return false;
	}

	if (!SetSteamRichPresence(StatusKey, Message))
	{
		return false;
	}

	return true;
}

bool FNYSteamPresence::SetRichPresenceSteamDisplay(const FUniqueNetId& PlayerId, const FName Token, const TMap<FString, FText>& Substitutions)
{
	// Docs: https://partner.steamgames.com/doc/api/ISteamFriends#richpresencelocalization
	static const FString SteamDisplayKey = TEXT("steam_display");
	if (!UNYSteamHelper::IsCurrentUser(PlayerId))
	{
		UE_LOG(LogNYSteamPresence, Warning, TEXT("SetRichPresenceSteamDisplay: Not current steam user"));
		return false;
	}

	const FString TokenString = Token.ToString();
	if (TokenString.Len() < 2)
	{
		UE_LOG(LogNYSteamPresence, Warning, TEXT("SetRichPresenceSteamDisplay: Token = %s is too short"), *TokenString);
		return false;
	}
	if (!TokenString.StartsWith(TEXT("#")))
	{
		UE_LOG(LogNYSteamPresence, Warning, TEXT("SetRichPresenceSteamDisplay: Token = %s is not a valid token. Valid tokens look like this: #token, #name"), *TokenString);
		return false;
	}

	// Set token
	if (!SetSteamRichPresence(SteamDisplayKey, TokenString))
	{
		return false;
	}

	// Use substitutions
	for (const auto& Pair : Substitutions)
	{
		SetSteamRichPresence(Pair.Key, Pair.Value.ToString());
	}

	return true;
}

bool FNYSteamPresence::SetSteamRichPresence(const FString& Key, const FString& Value)
{
	// Docs: https://partner.steamgames.com/doc/api/ISteamFriends#SetRichPresence
	ISteamFriends* SteamFriendsPtr = SteamFriends();
	if (!SteamFriendsPtr)
	{
		UE_LOG(LogNYSteamPresence, Warning, TEXT("SetSteamRichPresence: Can't get ISteamFriends"));
		return false;
	}

	if (!SteamFriendsPtr->SetRichPresence(TCHAR_TO_UTF8(*Key), TCHAR_TO_UTF8(*Value)))
	{
		if (Key.Len() == 0)
		{
			UE_LOG(
				LogNYSteamPresence,
				Warning,
				TEXT("SetSteamRichPresence: Cannot push rich presence for Key = %s, Value = %s. Because Key  empty"),
				*Key, *Value
			);
		}
		else if (Key.Len() >= k_cchMaxRichPresenceKeyLength)
		{
			UE_LOG(LogNYSteamPresence,
				Warning,
				TEXT("SetSteamRichPresence: Cannot push rich presence for Key = %s, Value = %s. Because Key is too long (%d)"),
				*Key, *Value, Key.Len()
			);
		}
		else if (Value.Len() >= k_cchMaxRichPresenceValueLength)
		{
			UE_LOG(LogNYSteamPresence,
				Warning,
				TEXT("SetSteamRichPresence: Cannot push rich presence for Key = %s, Value = %s. Because Value is too long (%d)"),
				*Key, *Value, Value.Len()
			);
		}
		else
		{
			UE_LOG(LogNYSteamPresence,
				Warning,
				TEXT("SetSteamRichPresence: Cannot push rich presence for Key = %s, Value = %s. Most likely because The user has reached the maximum amount of rich presence keys as defined by k_cchMaxRichPresenceKeys(%d). Call ClearPresence!!"),
				*Key, *Value, k_cchMaxRichPresenceKeys
			);
		}

		return false;
	}

	return true;
}
