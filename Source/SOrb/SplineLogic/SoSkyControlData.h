// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "SoSkyControlData.generated.h"

struct FSoSplinePoint;
class ASoPlayerSpline;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoSkyControlPoint
{
	GENERATED_USTRUCT_BODY()
public:
	FSoSkyControlPoint() {};

	FSoSkyControlPoint(FName InPreset, float InDistance) : Preset(InPreset), Distance(InDistance) {};

	UPROPERTY(EditAnywhere)
	FName Preset = FName("Default");

	UPROPERTY(EditAnywhere)
	float Distance = 0.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoSkyControlValue
{
	GENERATED_USTRUCT_BODY()

public:
	FSoSkyControlValue();
	FSoSkyControlValue(const FSoSkyControlPoint& Only);
	FSoSkyControlValue(const FSoSkyControlPoint& First, const FSoSkyControlPoint& Second, float InSecondWeight);

public:

	UPROPERTY(BlueprintReadOnly)
	FName FirstPreset;

	UPROPERTY(BlueprintReadOnly)
	FName SecondPreset;

	UPROPERTY(BlueprintReadOnly)
	float SecondWeight = 0.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoSkyControlPoints
{
	GENERATED_USTRUCT_BODY()

public:

	FSoSkyControlValue GetInterpolated(const FSoSplinePoint& Location) const;

	void ClearAndCreateDefaults(ASoPlayerSpline* Spline);
	void ValidateStartAndEnd(ASoPlayerSpline* Spline);

	void CreateNewPoint(float Distance);

	int32 GetClosestIndex(float Distance) const;
	int32 GetPrevIndex(float Distance) const;
	int32 GetNextIndex(float Distance) const;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSoSkyControlPoint> ControlPoints;
};
