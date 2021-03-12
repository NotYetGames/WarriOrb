// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoOnlineHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

#if WARRIORB_WITH_ONLINE
#include "OnlineSubsystem.h"
#endif
#include "Basic/SoGameInstance.h"

bool USoOnlineHelper::bWaitForFirstControllerInput = false;


TSharedPtr<const FUniqueNetId> USoOnlineHelper::GetUniqueNetIDFromObject(const UObject* WorldContextObject)
{
	const APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject);
	if (!PlayerController)
		return nullptr;

	const APlayerState* PlayerState = PlayerController->PlayerState;
	if (!PlayerState)
		return nullptr;

	return PlayerState->UniqueId.GetUniqueNetId();
}


bool USoOnlineHelper::IsLocalPlayerSignedIn(ULocalPlayer* LocalPlayer)
{
#if WARRIORB_WITH_ONLINE
	if (LocalPlayer == nullptr)
	{
		return false;
	}

	const auto OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		const auto IdentityInterface = OnlineSub->GetIdentityInterface();
		if (IdentityInterface.IsValid())
		{
			auto UniqueId = LocalPlayer->GetUniqueNetIdFromCachedControllerId();
			if (UniqueId.IsValid())
			{
				const auto LoginStatus = IdentityInterface->GetLoginStatus(*UniqueId);
				if (LoginStatus == ELoginStatus::LoggedIn)
				{
					return true;
				}
			}
		}
	}
#endif
	return false;
}

bool USoOnlineHelper::IsFirstLocalPlayerReadyToPlay(const UObject* WorldContextObject)
{
	ULocalPlayer* LocalPlayer = USoStaticHelper::GetPlayerController(WorldContextObject)->GetLocalPlayer();
	return IsLocalPlayerSignedIn(LocalPlayer);
	// && LocalPlayer->GetUniqueNetIdFromCachedControllerId() == USoGameInstance::Get(WorldContextObject).GetCachedControllerNetID();
}


void USoOnlineHelper::ShowExternalLoginUI(USoGameInstance* SoGameInstance)
{
#if WARRIORB_WITH_ONLINE
	const auto ExternalUI = Online::GetExternalUIInterface();
	if (ExternalUI.IsValid())
	{
		ExternalUI->ShowLoginUI(0, false, false, FOnLoginUIClosedDelegate::CreateUObject(SoGameInstance, &USoGameInstance::HandleLoginUIClosed));
	}
#endif
}


void USoOnlineHelper::SpawnLocalPlayers(const UObject* WorldContextObject)
{
#if WARRIORB_WITH_ONLINE
	FString ErrorReason;
	// the first player is already created by default
	for (int i = 2; i <= MAX_LOCAL_PLAYERS; i++)
	{
		USoGameInstance::Get(WorldContextObject).CreateLocalPlayer(-1, ErrorReason, true);
	}

	bWaitForFirstControllerInput = true;
#endif
}


void USoOnlineHelper::DestroyLocalPlayers(ULocalPlayer* TheChosenOne)
{
	USoGameInstance& SoGameInstance = USoGameInstance::Get(TheChosenOne);

	const TArray<class ULocalPlayer*>& LocalPlayers = USoGameInstance::Get(TheChosenOne).GetLocalPlayers();

	for (int32 i = LocalPlayers.Num() -1; i>=0; --i)
	{
		if (LocalPlayers[i] != TheChosenOne)
			SoGameInstance.RemoveLocalPlayer(LocalPlayers[i]);
	}
}


FString USoOnlineHelper::GetPlayerNickname(const UObject* WorldContextObject)
{
#if WARRIORB_WITH_ONLINE
	if (const APlayerController* PlayerController = USoStaticHelper::GetPlayerController(WorldContextObject))
	{
		const auto OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			const auto IdentityInterface = OnlineSub->GetIdentityInterface();
			TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(PlayerController->GetLocalPlayer()->GetControllerId());
			if (UserId.IsValid())
				return IdentityInterface->GetPlayerNickname(*UserId);
		}
	}
#endif

	return FString("Invalid");
}