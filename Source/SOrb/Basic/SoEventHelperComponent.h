// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SoEventHandler.h"

#include "SoEventHelperComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoEventHelperSignature);


/**
 *  Helper class implementing SoEventHandler interface, and doing the subscriptions/unsubscriptions based on bools
 *  Subscribed events are handled via firing an event
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SORB_API USoEventHelperComponent : public UActorComponent, public ISoEventHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoEventHelperComponent();

	FORCEINLINE virtual void HandlePlayerRespawn_Implementation() override				{ OnPlayerRespawn.Broadcast();				}
	FORCEINLINE virtual void HandleSoPostLoad_Implementation() override					{ OnSoPostLoad.Broadcast();					}
	FORCEINLINE virtual void HandlePlayerRematerialize_Implementation() override		{ OnPlayerRematerialize.Broadcast();		}

	// maybe TODO
	FORCEINLINE virtual void HandleWhitelistedSplinesEntered_Implementation() override	{ OnWhitelistedSplinesEntered.Broadcast();	}
	FORCEINLINE virtual void HandleWhitelistedSplinesLeft_Implementation() override		{ OnWhitelistedSplinesLeft.Broadcast();		}


protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UPROPERTY(BlueprintAssignable, Category = EventHelper)
	FSoEventHelperSignature OnSoPostLoad;

	UPROPERTY(BlueprintAssignable, Category = EventHelper)
	FSoEventHelperSignature OnPlayerRematerialize;

	UPROPERTY(BlueprintAssignable, Category = EventHelper)
	FSoEventHelperSignature OnPlayerRespawn;

	UPROPERTY(BlueprintAssignable, Category = EventHelper)
	FSoEventHelperSignature OnWhitelistedSplinesEntered;

	UPROPERTY(BlueprintAssignable, Category = EventHelper)
	FSoEventHelperSignature OnWhitelistedSplinesLeft;

protected:

	/** ONLY MODIFY THESE IN ACTOR'S CONSTRUCTION SCRIPT - the should not change once BeginPlay() is called */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EventHelper)
	bool bHandleSoPostLoad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EventHelper)
	bool bHandlePlayerRematerialize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EventHelper)
	bool bHandlePlayerRespawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EventHelper)
	bool bHandleWhitelistedSplines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EventHelper)
	TArray<TAssetPtr<ASoSpline>> WhitelistedSplines;
};
