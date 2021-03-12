// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoLocationRegistry.generated.h"


class ASoMarker;

/**
 * Gather information about all the checkpoints in the game
 * Used in the child BP in it's Construct.
 * NOTE LocationName = NAME_None is the start one (default)
 */
UCLASS()
class SORB_API ASoLocationRegistry : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoLocationRegistry();

	// Get information from a LocationName.
	// These methods are safe, if the LocationName Marker does not exist it returns a sane default
	FSoSplinePoint GetSplineLocation(FName LocationName) const;
	float GetZLocation(FName LocationName) const;
	FVector GetForwardVector(FName LocationName) const;

	// Returns the Marker for the LocationName or nullptr if the marker is not registered.
	const ASoMarker* GetMarker(FName LocationName) const;

	// Does the Marker identified  by LocattionName exist in the registry?
	bool HasMarker(FName LocationName) const { return GetMarker(LocationName) != nullptr; }

	// Register the LocationName
	UFUNCTION(BlueprintCallable, Category = Initialization)
	void RegisterSplineLocation(FName Name, ASoMarker* Location);

	// Should the rotation of the marker be backwards
	bool ShouldMarkerLookBackwards(FName LocationName) const { return NegativeDirections.Contains(LocationName); }

	const TMap<FName, ASoMarker*>& GetAllCheckpoints() const { return SplineLocations; }

	// Gets the LocationRegistry Instance
	static ASoLocationRegistry* GetInstance(UObject* WorldContextObject);

	// NAME_None is always the default/start checkpoint
	static FName GetDefaultCheckpointLocationName() { return NAME_None; }

protected:
	// Key: LocationName
	// Value: The checkpoint marker
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TMap<FName, ASoMarker*> SplineLocations;

	/** if the named location is in this the default orientation will be along the negative spline direction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSet<FName> NegativeDirections;
};
