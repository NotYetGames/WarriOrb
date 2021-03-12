// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoCharShadowData.h"
#include "Basic/Helpers/SoMathHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoCharShadowKeyNode& FSoCharShadowKeyNode::GetInvalidKey()
{
	static FSoCharShadowKeyNode Invalid = FSoCharShadowKeyNode{ 0.0f };
	return Invalid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCharShadowKeyNode::CopyValuesFrom(const FSoCharShadowKeyNode& Ref)
{
	Elevation = Ref.Elevation;
	Azimuth = Ref.Azimuth;
	Strength = Ref.Strength;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoCharShadowNodeList::CreateNewKey(float Distance, int32 KeyIndex)
{
	Keys.Push(GetInterpolated(Distance, KeyIndex));
	Keys.Sort([](const FSoCharShadowKeyNode& First, const FSoCharShadowKeyNode& Second)
	{
		return First.Distance < Second.Distance;
	});

	return GetClosestIndex(Distance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoCharShadowNodeList::GetClosestIndex(float Distance) const
{
	if (Keys.Num() == 0)
		return -1;

	int32 Index = 0;
	while (Index < Keys.Num() && Keys[Index].Distance < Distance)
		++Index;

	const int32 PreIndex = FMath::Max(0, Index - 1);
	const int32 PostIndex = FMath::Min(Index, Keys.Num() - 1);

	if (fabs(Keys[PreIndex].Distance - Distance) < fabs(Keys[PostIndex].Distance - Distance))
		return PreIndex;

	return PostIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoCharShadowNodeList::GetPrevIndex(float Distance) const
{
	int32 Index = 0;
	while (Index < Keys.Num() && Keys[Index].Distance < Distance)
		++Index;

	return FMath::Max(Index - 1, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoCharShadowNodeList::GetNextIndex(float Distance) const
{
	int32 Index = 0;
	while (Index < Keys.Num() && Keys[Index].Distance < Distance)
		++Index;

	return FMath::Min(Index, Keys.Num() - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCharShadowKeyNode FSoCharShadowNodeList::GetInterpolated(const float DistanceOnSpline, int32& InOutIndex) const
{
	FSoCharShadowKeyNode Interpolated;
	Interpolated.Strength = 0.0f;
	Interpolated.Distance = DistanceOnSpline;

	if (Keys.Num() > 0)
	{
		InOutIndex = FMath::Clamp(InOutIndex, 0, Keys.Num() - 1);

		while (DistanceOnSpline < Keys[InOutIndex].Distance && InOutIndex > 0)
			--InOutIndex;
		while (InOutIndex < Keys.Num() - 1 && DistanceOnSpline > Keys[InOutIndex + 1].Distance)
			++InOutIndex;

		if (InOutIndex == 0 && Keys[0].Distance > DistanceOnSpline)
			Interpolated.CopyValuesFrom(Keys[0]);
		else if (InOutIndex == Keys.Num() - 1)
			Interpolated.CopyValuesFrom(Keys[InOutIndex]);
		else
		{
			const float FirstDistance = Keys[InOutIndex].Distance;
			const float SecondDistance = Keys[InOutIndex + 1].Distance;
			const float OtherWeight = (DistanceOnSpline - FirstDistance) / (SecondDistance - FirstDistance);

			Interpolated.Azimuth = USoMathHelper::InterpolateSmoothStep(Keys[InOutIndex].Azimuth,
																		Keys[InOutIndex].Azimuth + USoMathHelper::ShortAngleDist(Keys[InOutIndex].Azimuth, Keys[InOutIndex + 1].Azimuth),
																		OtherWeight);

			Interpolated.Elevation = USoMathHelper::InterpolateSmoothStep(Keys[InOutIndex].Elevation, Keys[InOutIndex + 1].Elevation, OtherWeight);
			Interpolated.Strength = USoMathHelper::InterpolateSmoothStep(Keys[InOutIndex].Strength, Keys[InOutIndex + 1].Strength, OtherWeight);
		}
	}

	return Interpolated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCharShadowNodeList::ReScale(float SplineLength)
{
	const float LastDistance = Keys.Top().Distance;
	if (fabs(SplineLength - LastDistance) > KINDA_SMALL_NUMBER)
	{
		const float ScaleFactor = SplineLength / LastDistance;
		for (int i = 0; i < Keys.Num(); ++i)
			Keys[i].Distance *= ScaleFactor;
	}
}
