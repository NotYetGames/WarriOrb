// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "SplineLogic/SoSplinePointPtr.h"
#include "SoMarker.generated.h"


UCLASS()
class SORB_API ASoMarker : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoMarker();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void BeginPlay() override;

	// Initialize spline location from the spline pointer.
	void InitiliazeSplineLocation();

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	FSoSplinePoint GetSplineLocation() const { return SplineLocation; }

	UFUNCTION(BlueprintPure, Category = "SplinePoint")
	float GetDistanceOnSpline() const { return SplineLocation.GetDistance(); }

	// Gets the spline location pointer
	FSoSplinePointPtr GetSplineLocationPtr() const { return SplineLocationPtr; }

	static bool IsMarkerInRangeFromSplineLocation(const ASoMarker* Marker, const FSoSplinePoint& SplineLocation, float MinDistance, float MaxDistance);

protected:
	// EditAnywhere VisibleAnywhere
	/** Constructed in BeginPlay/ConstructScript from SplineLocationPtr  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoSplinePoint SplineLocation;

	/** used in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoSplinePointPtr SplineLocationPtr;
};
