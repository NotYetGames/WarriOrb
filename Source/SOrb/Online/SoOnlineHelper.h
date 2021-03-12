// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/CoreOnline.h"

#if WARRIORB_WITH_ONLINE
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "OnlineSubsystemUtils/Public/OnlineSubsystemUtils.h"
#endif

#include "SoOnlineHelper.generated.h"


/**
 *
 */
UCLASS()
class SORB_API USoOnlineHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static TSharedPtr<const FUniqueNetId> GetUniqueNetIDFromObject(const UObject* WorldContextObject);


	static bool IsLocalPlayerSignedIn(class ULocalPlayer* LocalPlayer);

	UFUNCTION(BlueprintCallable)
	static void ShowExternalLoginUI(class USoGameInstance* SoGameInstance);

	/** signed in and got access to the controller associated with player controller 0 */
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static bool IsFirstLocalPlayerReadyToPlay(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void SpawnLocalPlayers(const UObject* WorldContextObject);

	UFUNCTION()
	static void DestroyLocalPlayers(ULocalPlayer* TheChosenOne);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static FString GetPlayerNickname(const UObject* WorldContextObject);



	static bool bWaitForFirstControllerInput;
};
