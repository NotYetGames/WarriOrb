// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoEventHandler.generated.h"

class ASoSpline;

/**
 *  Helper functions because UInterface can't have static functions
 *  UObjects passed as parameter must implement the ISoEventHandler interface!
 */
UCLASS()
class SORB_API USoEventHandlerHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Subscribes to post load and calls HandleSoPostLoad() on the subscribed actor */
	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void SubscribeToSoPostLoad(UObject* WorldContextObject);

	/** Unsubscribes from post load, should be called when the actor is removed from level / destroyed */
	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void UnsubscribeFromSoPostLoad(UObject* WorldContextObject);


	/** Registers caller to the list so HandlePlayerRematerialize() will be called on it. Is not called on subscribe */
	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void SubscribeToPlayerRematerialize(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void UnsubscribeFromPlayerRematerialize(UObject* WorldContextObject);


	/** Registers caller to the list so HandlePlayerRematerialize() will be called on it. Is not called on subscribe */
	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void SubscribeToPlayerRespawn(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void UnsubscribeFromPlayerRespawn(UObject* WorldContextObject);


	/**
	 *  Registers USoEventHandler implementers and notifies them based on area checks on player spline change
	 *  If the character is in whitelisted area right now the HandleWhitelistedSplinesEntered() is called immediately
	 */
	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void SubscribeWhitelistedSplines(UObject* WorldContextObject, const TArray<TAssetPtr<ASoSpline>>& WhitelistedSplines);

	/** Expected not to be called during spline change callback */
	UFUNCTION(BlueprintCallable, Category = SoPostLoad, meta = (WorldContext = "WorldContextObject"))
	static void UnsubscribeWhitelistedSplines(UObject* WorldContextObject);
};


/** Just unreal interface thing */
UINTERFACE()
class SORB_API USoEventHandler : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


class SORB_API ISoEventHandler
{
	GENERATED_IINTERFACE_BODY()

	/** Called on each actor after a save file is loaded */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SoPostLoadHandler)
	void HandleSoPostLoad();

	/**
	 *  Called if registered in case of the following events:
	 *		- Player died and respawned near Checkpoint/Soulkeeper
	 *		- Player interacted with a Soulkeeper and regained HP (the world has to be reset in this case, same way as if he would have died)
	 *		- Player performed a BreakIntoPeaces/FallIntoDeath HitReact and respawned near a rematerialize point
	 *			(Some traps must be reset in this case, or they are ruined / could be skipped)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SoPostLoadHandler)
	void HandlePlayerRematerialize();

	/**
	 *  Called if registered in case of the following events:
	 *		- Player died and respawned near Checkpoint/Soulkeeper
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SoPostLoadHandler)
	void HandlePlayerRespawn();



	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SoPostLoadHandler)
	void HandleWhitelistedSplinesEntered();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = SoPostLoadHandler)
	void HandleWhitelistedSplinesLeft();
};
