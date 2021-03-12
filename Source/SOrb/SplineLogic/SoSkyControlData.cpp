// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoSkyControlData.h"
#include "Components/SplineComponent.h"
#include "Objects/SoSky.h"
#include "SoPlayerSpline.h"
#include "Basic/SoGameSingleton.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSkyControlValue::FSoSkyControlValue(const FSoSkyControlPoint& First, const FSoSkyControlPoint& Second, float InSecondWeight)
{
	FirstPreset = First.Preset;
	SecondPreset = Second.Preset;
	SecondWeight = InSecondWeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSkyControlValue::FSoSkyControlValue()
{
	FirstPreset = ASoSky::DefaultPresetName;
	SecondPreset = ASoSky::DefaultPresetName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSkyControlValue::FSoSkyControlValue(const FSoSkyControlPoint& Only)
{
	FirstPreset = Only.Preset;
	SecondPreset = ASoSky::DefaultPresetName;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSkyControlValue FSoSkyControlPoints::GetInterpolated(const FSoSplinePoint& Location) const
{
	FSoSkyControlValue Value;

	const float Distance = Location.GetDistance();
	int32 Index = 0;

	for (; Index + 1 < ControlPoints.Num() && ControlPoints[Index + 1].Distance < Distance; ++Index);

	if (Index < ControlPoints.Num())
	{
		if (Index == ControlPoints.Num() - 1)
			Value = FSoSkyControlValue{ ControlPoints[Index] };
		else
		{
			const float FirstDistance = ControlPoints[Index].Distance;
			const float SecondDistance = ControlPoints[Index + 1].Distance;

			if (fabs(SecondDistance - FirstDistance) < KINDA_SMALL_NUMBER)
				Value = FSoSkyControlValue{ ControlPoints[Index] };
			else
			{
				const float SecondWeight = (Distance - FirstDistance) / (SecondDistance - FirstDistance);
				Value = FSoSkyControlValue{ ControlPoints[Index], ControlPoints[Index + 1], SecondWeight };
			}
		}
	}
	return Value;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSkyControlPoints::ClearAndCreateDefaults(ASoPlayerSpline* Spline)
{
	if (Spline == nullptr)
		return;

	ControlPoints.Empty();
	const FString& SplineName = Spline->GetSplineName();

	FName StartSkyPreset = USoGameSingleton::GetDefaultSkyPresetFromSplineName(SplineName, true);
	FName EndSkyPreset = USoGameSingleton::GetDefaultSkyPresetFromSplineName(SplineName, false);

	if (StartSkyPreset == NAME_None)
		StartSkyPreset = ASoSky::DefaultPresetName;
	if (EndSkyPreset == NAME_None)
		EndSkyPreset = ASoSky::DefaultPresetName;

	ControlPoints.Add({ StartSkyPreset, 0.0f });
	ControlPoints.Add({ EndSkyPreset, Spline->GetSplineComponent()->GetSplineLength() });
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSkyControlPoints::ValidateStartAndEnd(ASoPlayerSpline* Spline)
{
	if (ControlPoints.Num() < 2)
		ClearAndCreateDefaults(Spline);
	else
	{
		ControlPoints[0].Distance = 0;
		ControlPoints[ControlPoints.Num() - 1].Distance = Spline->GetSplineComponent()->GetSplineLength();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSkyControlPoints::CreateNewPoint(float Distance)
{
	int32 Index = 0;
	while (Index < ControlPoints.Num() && ControlPoints[Index].Distance < Distance)
		++Index;

	FSoSkyControlPoint Point;
	Point.Distance = Distance;
	if (Index > 0)
		Point.Preset = ControlPoints[Index - 1].Preset;
	else
		if (Index < ControlPoints.Num())
			Point.Preset = ControlPoints[Index].Preset;

	ControlPoints.Insert(Point, Index);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoSkyControlPoints::GetClosestIndex(float Distance) const
{
	if (ControlPoints.Num() == 0)
		return -1;

	int32 Index = 0;
	while (Index < ControlPoints.Num() && ControlPoints[Index].Distance < Distance)
		++Index;

	const int32 PreIndex = FMath::Max(0, Index - 1);
	const int32 PostIndex = FMath::Min(Index, ControlPoints.Num() - 1);

	if (fabs(ControlPoints[PreIndex].Distance - Distance) < fabs(ControlPoints[PostIndex].Distance - Distance))
		return PreIndex;

	return PostIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoSkyControlPoints::GetPrevIndex(float Distance) const
{
	int32 Index = 0;
	while (Index < ControlPoints.Num() && ControlPoints[Index].Distance < Distance)
		++Index;

	return FMath::Max(Index - 1, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoSkyControlPoints::GetNextIndex(float Distance) const
{
	int32 Index = 0;
	while (Index < ControlPoints.Num() && ControlPoints[Index].Distance < Distance)
		++Index;

	return FMath::Min(Index, ControlPoints.Num() - 1);
}
