// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoCameraData.h"

#include "Basic/Helpers/SoMathHelper.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoCamNodeList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_LOG_CATEGORY_STATIC(LogSoCameraData, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
T Interpolate(const T& First, const T& Second, const float SecondWeight, ESoCamInterpolationMethod Method)
{
	// https://codeplea.com/simple-interpolation
	switch (Method)
	{
	case ESoCamInterpolationMethod::ECIM_Cosine:
		return USoMathHelper::InterpolateCosine(First, Second, SecondWeight);

	case ESoCamInterpolationMethod::ECIM_SmoothStep:
		return USoMathHelper::InterpolateSmoothStep(First, Second, SecondWeight);

	case ESoCamInterpolationMethod::ECIM_Acceleration:
		return USoMathHelper::InterpolateAcceleration(First, Second, SecondWeight);

	case ESoCamInterpolationMethod::ECIM_Deceleration:
		return USoMathHelper::InterpolateDeceleration(First, Second, SecondWeight);

	case ESoCamInterpolationMethod::ECIM_Linear:
	default:
		return First + SecondWeight * (Second - First);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCamKeyNode::FSoCamKeyNode()
{
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCamKeyNode::FSoCamKeyNode(const FSoSplinePoint& InSplinePoint)
{
	Distance = InSplinePoint.GetDistance();
	SplinePoint = InSplinePoint;
	Position = InSplinePoint;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCamKeyNode::FSoCamKeyNode(float InArmLength, float InDeltaYaw, float InDeltaPitch, float InMaxHorizontalOffset, FVector InDeltaOffset) :
	ArmLength(InArmLength),
	DeltaYaw(InDeltaYaw),
	DeltaPitch(InDeltaPitch),
	MaxHorizontalOffset(InMaxHorizontalOffset),
	DeltaOffset(InDeltaOffset)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCamNodeList::CreateNewCamKey(const FSoSplinePoint& SplinePoint, int32 CamKeyIndex)
{
	CamKeys.Push(GetInterpolatedCamData(SplinePoint.GetDistance(), CamKeyIndex, true));

	CamKeys.Sort([](const FSoCamKeyNode& First, const FSoCamKeyNode& Second)
	{
		return First.SplinePoint.GetDistance() < Second.SplinePoint.GetDistance();
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCamKeyNode FSoCamNodeList::GetInterpolatedCamData(float DistanceOnSpline, int32& InOutIndex, bool bForceSplineBased) const
{
	if (CamKeys.Num() == 0)
		return {};

	InOutIndex = FMath::Clamp(InOutIndex, 0, CamKeys.Num() - 1);

	while (DistanceOnSpline < CamKeys[InOutIndex].SplinePoint.GetDistance() && InOutIndex > 0) --InOutIndex;
	while (InOutIndex < CamKeys.Num() - 1 && DistanceOnSpline > CamKeys[InOutIndex + 1].SplinePoint.GetDistance()) ++InOutIndex;

	if (InOutIndex == CamKeys.Num() - 1)
	{
		FSoCamKeyNode OutNode = CamKeys[InOutIndex];
		OutNode.Position = GetPosition(InOutIndex);
		OutNode.Rotation = GetRotation(InOutIndex);
		return OutNode;
	}

	const int32 CorrectedIndex0 = FMath::Clamp(InOutIndex + CamKeys[InOutIndex].LockOffset, 0, CamKeys.Num() - 1);
	const int32 CorrectedIndex1 = FMath::Clamp(InOutIndex + 1 + CamKeys[InOutIndex + 1].LockOffset, 0, CamKeys.Num() - 1);
	if (CorrectedIndex0 == CorrectedIndex1)
	{
		FSoCamKeyNode Node = CamKeys[CorrectedIndex0];
		Node.Position = GetPosition(InOutIndex);
		Node.Rotation = GetRotation(InOutIndex);
		return Node;
	}

	FSoCamKeyNode InterpKey = FSoCamKeyNode::GetInterpolatedCamData(CamKeys[CorrectedIndex0],
																	CamKeys[CorrectedIndex1],
																	CamKeys[InOutIndex].SplinePoint.GetDistance(),
																	CamKeys[InOutIndex + 1].SplinePoint.GetDistance(),
																	DistanceOnSpline);
	if (CamKeys[InOutIndex].bUseSplineOnInterpolation || bForceSplineBased)
	{
#if WITH_EDITOR
		if (CamKeys[InOutIndex].LockOffset != 0 || CamKeys[InOutIndex + 1].LockOffset != 0)
			UE_LOG(LogSoCameraData, Warning, TEXT("LockOffset doesn't make any sense with bUseSplineOnInterpolation (%d)"), InOutIndex);
#endif

		InterpKey.Position = InterpKey.SplinePoint;
		InterpKey.Position.Z = 0;
		InterpKey.Rotation = InterpKey.GetRotation();
	}
	else
	{
		const float FirstDistance = CamKeys[InOutIndex].SplinePoint.GetDistance();
		const float SecondDistance = CamKeys[InOutIndex + 1].SplinePoint.GetDistance();
		const float OtherWeight = (DistanceOnSpline - FirstDistance) / (SecondDistance - FirstDistance);

		// InterpKey.Position = Interpolate(GetPosition(CorrectedIndex0), GetPosition(CorrectedIndex1), OtherWeight, CamKeys[CorrectedIndex0].InterpolationMethod);
		InterpKey.Position = Interpolate(GetPosition(CorrectedIndex0), GetPosition(CorrectedIndex1), OtherWeight, ESoCamInterpolationMethod::ECIM_Linear);

		FRotator Rot1 = GetRotation(CorrectedIndex0);
		FRotator Rot2 = GetRotation(CorrectedIndex1);

		if (fabs(Rot1.Yaw - Rot2.Yaw) > 180.0f)
		{
			if (Rot1.Yaw < Rot2.Yaw)
				Rot1.Yaw += 360;
			else
				Rot2.Yaw += 360;
		}

		InterpKey.Rotation = Interpolate(Rot1, Rot2, OtherWeight, CamKeys[CorrectedIndex0].InterpolationMethod);

	}
	return InterpKey;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCamNodeList::ReScale(float SplineLength)
{
	const float LastDistance = CamKeys.Top().SplinePoint.GetDistance();
	if (fabs(SplineLength - LastDistance) > KINDA_SMALL_NUMBER)
	{
		const float ScaleFactor = SplineLength / LastDistance;
		for (int i = 0; i < CamKeys.Num(); ++i)
		{
			const FSoSplinePoint OldPoint = CamKeys[i].SplinePoint;
			CamKeys[i].SplinePoint = OldPoint.GetScaledSplinePoint(ScaleFactor);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector FSoCamNodeList::GetPosition(int32 CamKeyIndex) const
{
	int32 Ix;
	if (!CalcAndCheckLockOffset(CamKeyIndex, Ix)) return FVector(0, 0, 0);
	FVector Pos = CamKeys[Ix].SplinePoint;
	Pos.Z = 0.f;
	return Pos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRotator FSoCamNodeList::GetRotation(int32 CamKeyIndex) const
{
	int32 Ix;
	if (!CalcAndCheckLockOffset(CamKeyIndex, Ix)) return FRotator();

	return CamKeys[Ix].GetRotation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoCamNodeList::CalcAndCheckLockOffset(int32 CamKeyIndex, int32& OutIndex) const
{
	OutIndex = CamKeyIndex + CamKeys[CamKeyIndex].LockOffset;
#ifdef WITH_EDITOR
	if (OutIndex < 0 || OutIndex >= CamKeys.Num())
	{
		UE_LOG(LogSoCameraData, Error, TEXT("InValid LockOffset on CamKey %d"), CamKeyIndex);
		return false;
	}
#endif
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCamNodeList::Modify(const FSoCamKeyNodeModifier& Modifier)
{
	if (Modifier.KeyIndex < 0 || Modifier.KeyIndex >= CamKeys.Num())
	{
		UE_LOG(LogSoCameraData, Error, TEXT("InValid CamKey index in modifier %d"), Modifier.KeyIndex);
		return;
	}
	FSoCamKeyNode& Node = CamKeys[Modifier.KeyIndex];

	if (Modifier.bModifyYaw) Node.DeltaYaw = Modifier.DeltaYaw;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoCamKeyNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRotator FSoCamKeyNode::GetRotation() const
{
	FVector Direction = SplinePoint.GetDirection();
	Direction.Z = 0.f;
	return Direction.Rotation() + FRotator(DeltaPitch, 90.f + DeltaYaw, 0.f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoCamKeyNode::GetDirectionModifier() const
{
	if (fabs(DeltaYaw) < 90.f - KINDA_SMALL_NUMBER) return 1;
	if (fabs(fabs(DeltaYaw) - 90.f) < KINDA_SMALL_NUMBER) return 0;
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoCamKeyNode FSoCamKeyNode::GetInterpolatedCamData(const FSoCamKeyNode& First,
													const FSoCamKeyNode& Second,
													float FirstSplineDistance,
													float SecondSplineDistance,
													float DistanceOnSpline)
{
#ifdef WITH_EDITOR
	if (SecondSplineDistance < FirstSplineDistance)
	{
		UE_LOG(LogSoCameraData, Error, TEXT("GetInterpolatedCamData called on wrong CamKey"));
		return FSoCamKeyNode();
	}
#endif

	if (fabs(SecondSplineDistance - FirstSplineDistance) < KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogSoCameraData, Error, TEXT("FSoCamKeyNode::GetInterpolatedCamData called with invaid values \
									 (maybe more than one spline point is on the same splinepoint!)"));
		if (FirstSplineDistance < KINDA_SMALL_NUMBER)
			FirstSplineDistance = 0;
		else
			FirstSplineDistance = 1000; // just random number to stop (SecondSplineDistance - FirstSplineDistance) being 0
	}

	const float SecondWeight = (DistanceOnSpline - FirstSplineDistance) / (SecondSplineDistance - FirstSplineDistance);
	const float FirstWeight = 1.0f - SecondWeight;

	FSoCamKeyNode InterpolatedCamData;
	// InterpolatedCamData.Distance = Interpolate(First.Distance, Second.Distance, SecondWeight, First.InterpolationMethod);
	InterpolatedCamData.Distance = DistanceOnSpline;
	InterpolatedCamData.ArmLength = Interpolate(First.ArmLength, Second.ArmLength, SecondWeight, First.InterpolationMethod);
	InterpolatedCamData.DeltaYaw = Interpolate(First.DeltaYaw, Second.DeltaYaw, SecondWeight, First.InterpolationMethod);
	InterpolatedCamData.DeltaPitch = Interpolate(First.DeltaPitch, Second.DeltaPitch, SecondWeight, First.InterpolationMethod);
	InterpolatedCamData.MaxHorizontalOffset = Interpolate(First.MaxHorizontalOffset, Second.MaxHorizontalOffset, SecondWeight, First.InterpolationMethod);
	InterpolatedCamData.DeltaOffset = Interpolate(First.DeltaOffset, Second.DeltaOffset, SecondWeight, First.InterpolationMethod);
	InterpolatedCamData.bUseSplineOnInterpolation = First.bUseSplineOnInterpolation;
	InterpolatedCamData.InterpolationMethod = First.InterpolationMethod;
	InterpolatedCamData.SplinePoint = First.SplinePoint;
	InterpolatedCamData.SplinePoint.SetDistance(DistanceOnSpline);

	return InterpolatedCamData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoCamKeyNode::CopyParams(const FSoCamKeyNode& _Source, bool CopyDistance)
{
	if (CopyDistance)
		Distance = _Source.Distance;
	ArmLength = _Source.ArmLength;
	DeltaYaw = _Source.DeltaYaw;
	DeltaPitch = _Source.DeltaPitch;
	MaxHorizontalOffset = _Source.MaxHorizontalOffset;
	DeltaOffset = _Source.DeltaOffset;
	bUseSplineOnInterpolation = _Source.bUseSplineOnInterpolation;
	LockOffset = _Source.LockOffset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoCamKeyNode::CompareParams(const FSoCamKeyNode& _Source) const
{
	return	fabs(ArmLength - _Source.ArmLength) < KINDA_SMALL_NUMBER &&
			fabs(DeltaYaw - _Source.DeltaYaw) < KINDA_SMALL_NUMBER &&
			fabs(DeltaPitch - _Source.DeltaPitch) < KINDA_SMALL_NUMBER &&
			fabs(MaxHorizontalOffset - _Source.MaxHorizontalOffset) < KINDA_SMALL_NUMBER &&
			(DeltaOffset - _Source.DeltaOffset).Size() < KINDA_SMALL_NUMBER &&
			LockOffset == _Source.LockOffset;
}
