// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoSplinePoint.h"
#include "SoSplinePointPtr.generated.h"


/**
 *  Safe way to store a spline location from another level
 *  All the splines are in the persistent level, this struct
 *  is mostly used to store init spline locations in editor
 */
USTRUCT(BlueprintType)
struct FSoSplinePointPtr
{
	GENERATED_USTRUCT_BODY()
public:

	FSoSplinePointPtr();

	FSoSplinePointPtr(const FSoSplinePoint& SplinePoint);

	// Extract the Splin point from the Spline pointer
	FSoSplinePoint Extract() const;

	void SetDistanceAlongSpline(float Distance);
	float GetDistanceAlongSpline() const { return DistanceAlongSpline; }

	// Is The Spline pointer valid?
	bool IsValid() const;
protected:
	UPROPERTY(EditAnywhere)
	float DistanceAlongSpline = 0.f;

	/** safe spline reference, works between different levels */
	UPROPERTY(EditAnywhere)
	TAssetPtr<ASoSpline> SplinePtr;
};
