// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoSplinePoint.h"
#include "Components/SplineComponent.h"
#include "SoSpline.h"


static constexpr int32 ACCEPTED_SPLINECONNECTION_DISTANCE = 10;

DEFINE_LOG_CATEGORY(LogSoSplineSys);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint::FSoSplinePoint()
{
	Distance = 0.f;
	Spline = nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint::FSoSplinePoint(ASoSpline* InSpline, float InDistance /*= 0.f*/) :
	Distance(InDistance),
	Spline(InSpline)
{
	ClampSplineData();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint::FSoSplinePoint(const FSoSplinePoint& TheOther) :
	Distance(TheOther.Distance),
	Spline(TheOther.Spline),
	ReferenceActor(TheOther.ReferenceActor),
	ReferenceZ(TheOther.ReferenceZ)
{
	ClampSplineData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::operator=(const FSoSplinePoint& OtherPoint)
{
	Distance = OtherPoint.Distance;
	Spline = OtherPoint.Spline;
	ReferenceActor = OtherPoint.ReferenceActor;
	ReferenceZ = OtherPoint.ReferenceZ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::CopySplineLocation(const FSoSplinePoint& OtherPoint)
{
	Distance = OtherPoint.Distance;
	Spline = OtherPoint.Spline;
	ReferenceZ = OtherPoint.ReferenceZ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint FSoSplinePoint::operator+(float DeltaDistance) const
{
	FSoSplinePoint NewPoint = *this;
	NewPoint += DeltaDistance;
	return NewPoint;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::SetDistance(float InDistance)
{
	if (InDistance < 0 || InDistance > GetActiveSplineLength())
	{
		UE_LOG(LogSoSplineSys, Error, TEXT("SetDistance: invalid distance %f for %s"), InDistance, *Spline->GetSplineName());
		InDistance = FMath::Clamp(InDistance, 0.f, GetActiveSplineLength());
	}
	Distance = InDistance;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::SetDistanceFromInputKey(float InputKey)
{
	if (Spline == nullptr)
		return;

	const USplineComponent* SplineComponent = Spline->GetSplineComponent();
	const float SplineLength = SplineComponent->GetSplineLength();
	float Left = 0.0f;
	float Right = SplineLength;
	float NewDistance = Right * 0.5f;
	// WARNING: if the bug is fixed in the engine "(SplineComponent->GetNumberOfSplinePoints() - 1.0f)" has to be removed
	float EstimatedInputKey = SplineComponent->GetInputKeyAtDistanceAlongSpline(NewDistance) * (SplineComponent->GetNumberOfSplinePoints() - 1.0f);
	while (fabs(EstimatedInputKey - InputKey) > KINDA_SMALL_NUMBER)
	{
		if (EstimatedInputKey > InputKey)
			Right = NewDistance;
		else
			Left = NewDistance;

		NewDistance = (Left + Right) / 2.0f;
		// WARNING: if the bug is fixed in the engine "(SplineComponent->GetNumberOfSplinePoints() - 1.0f)" has to be removed
		EstimatedInputKey = SplineComponent->GetInputKeyAtDistanceAlongSpline(NewDistance) * (SplineComponent->GetNumberOfSplinePoints() - 1.0f);

		SplineComponent->GetSplineLength();

		if (NewDistance < KINDA_SMALL_NUMBER || NewDistance > SplineLength - 1.0f)
			break;
	}

	Distance = NewDistance;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::SetSpline(ASoSpline* InSpline)
{
	Spline = InSpline;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::ClampSplineData()
{
	if (Spline)
		Distance = FMath::Clamp(Distance, 0.f, Spline->GetSplineComponent()->GetSplineLength());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::operator+=(float DeltaDistance)
{
	const FVector DirectionVector = GetDirection();
	if (DirectionVector.Z != 0)
	{
		FVector HorizontalDirection = FVector(DirectionVector.X, DirectionVector.Y, 0.f);
		HorizontalDirection.Normalize();

		DeltaDistance /= (DirectionVector | HorizontalDirection);
	}

	bool bSplineEnded = false;
	float Rest = 0;
	AddToDistance(DeltaDistance, bSplineEnded, Rest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::ProjectAndAddToDistance(float DeltaDistance, bool& bOutSplineEnd, float& OutRestDistanceTime)
{
	const FVector DirectionVector = GetDirection();
	if (DirectionVector.Z != 0)
	{
		FVector HorizontalDirection = FVector(DirectionVector.X, DirectionVector.Y, 0.f);
		HorizontalDirection.Normalize();

		DeltaDistance /= (DirectionVector | HorizontalDirection);
	}
	AddToDistance(DeltaDistance, bOutSplineEnd, OutRestDistanceTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::AddToDistance(const float DeltaDistance, bool& bOutSplineEnd, float& OutRestDistanceTime)
{
	bOutSplineEnd = false;
	OutRestDistanceTime = 0.f;

	if (fabs(DeltaDistance) < KINDA_SMALL_NUMBER)
		return;

	const float NewDistance = Distance + DeltaDistance * Spline->GetSplineDirection();

	if (NewDistance < 0.f)
	{
		const float ZValue = ReferenceActor == nullptr ? ReferenceZ : ReferenceActor->GetActorLocation().Z;
		ASoSpline* PrevSpline = Spline->GetPrevSpline(ZValue);
		if (PrevSpline == nullptr)
		{
			bOutSplineEnd = true;
			OutRestDistanceTime = fabs(NewDistance) / fabs(DeltaDistance);
			Distance = 0;
			return;
		}
		const FVector Offset = PrevSpline->GetEndWorldLocation() - Spline->GetStartWorldLocation();

#if WITH_EDITOR
		{
			const FVector Dist1 = Offset;
			const FVector Dist2 = PrevSpline->GetStartWorldLocation() - Spline->GetStartWorldLocation();

			if (Dist1.Size2D() > ACCEPTED_SPLINECONNECTION_DISTANCE && Dist2.Size2D() > ACCEPTED_SPLINECONNECTION_DISTANCE)
				UE_LOG(LogSoSplineSys, Error, TEXT("Two spline isn't connected properly: %s and %s"), *Spline->GetSplineName(), *PrevSpline->GetSplineName());
		}
#endif
		if (Offset.Size2D() > ACCEPTED_SPLINECONNECTION_DISTANCE)
			Distance = -NewDistance;
		else
			Distance = PrevSpline->GetSplineComponent()->GetSplineLength() + NewDistance;
		Spline = PrevSpline;
	}
	else
	{
		const float SplineLength = Spline->GetSplineComponent()->GetSplineLength();
		if (NewDistance > SplineLength)
		{
			const float ZValue = ReferenceActor == nullptr ? ReferenceZ : ReferenceActor->GetActorLocation().Z;
			ASoSpline* NextSpline = Spline->GetNextSpline(ZValue);
			if (NextSpline == nullptr)
			{
				bOutSplineEnd = true;
				OutRestDistanceTime = (NewDistance - SplineLength) / fabs(DeltaDistance);
				Distance = SplineLength;
				return;
			}
			Distance = NewDistance - SplineLength;
			const FVector Offset = Spline->GetEndWorldLocation() - NextSpline->GetStartWorldLocation();

#if WITH_EDITOR
			{
				const FVector Dist1 = Offset;
				const FVector Dist2 = NextSpline->GetEndWorldLocation() - Spline->GetEndWorldLocation();

				if (Dist1.Size2D() > ACCEPTED_SPLINECONNECTION_DISTANCE && Dist2.Size2D() > ACCEPTED_SPLINECONNECTION_DISTANCE)
					UE_LOG(LogSoSplineSys, Error, TEXT("Two spline isn't connected properly: %s and %s"), *Spline->GetSplineName(), *NextSpline->GetSplineName());
			}
#endif

			if (Offset.Size2D() > ACCEPTED_SPLINECONNECTION_DISTANCE)
				Distance = NextSpline->GetSplineComponent()->GetSplineLength() - Distance;
			Spline = NextSpline;
		}
		else
			Distance = NewDistance;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::SetDistanceFromWorldLocation(const FVector& WorldLocation)
{
	if (Spline == nullptr)
	{
		UE_LOG(LogSoSplineSys, Error, TEXT("FSoSplinePoint::SetDistanceFromWorldLocation called, but Spline is nullptr!"));
		return;
	}
	const float InputKey = Spline->GetSplineComponent()->FindInputKeyClosestToWorldLocation(WorldLocation);
	SetDistanceFromInputKey(InputKey);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoSplinePoint::operator-(const FSoSplinePoint& OtherPoint) const
{
#if WITH_EDITOR
	if (Spline == nullptr || OtherPoint.GetSpline() == nullptr)
	{
		UE_LOG(LogSoSplineSys, Error, TEXT("FSoSplinePoint - FSoSplinePoint: at least one spline is nullptr!"));
		return BIG_NUMBER;
	}
#endif
	return GetDistanceFromSplinePoint(OtherPoint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A - B == A.GetDistanceFromSplinePoint(B)
float FSoSplinePoint::GetDistanceFromSplinePoint(const FSoSplinePoint& OtherPoint, float MaxAcceptedDistance) const
{
	if (Spline == nullptr || OtherPoint.Spline == nullptr)
		return BIG_NUMBER;

	struct FSoPointDesc
	{
		const ASoSpline* Spline = nullptr;	// the spline the point is on
		bool bStartPoint = true;			// indicates if the point is the first or the last point of the spline
		float Distance = 0.0f;				// shortest distance from B so far
		int32 Direction = 1;				// the direction we have to go along B from starter location to reach this point

		FSoPointDesc(const ASoSpline* S, bool B, float D, int Dir) : Spline(S), bStartPoint(B), Distance(D), Direction(Dir) {};
	};

	int32 TargetDirection = 1;
	float TargetDistance = BIG_NUMBER;

	if (Spline == OtherPoint.Spline)
	{
		// register the trivial way
		TargetDistance = (Distance - OtherPoint.Distance) * Spline->GetSplineDirection();
		TargetDirection = FMath::Sign(TargetDistance);
		TargetDistance = fabs(TargetDistance);
		// we still have to go on - another way could be shorter theoretically
	}

	// One Path is one path through the spline graph
	// Only the distance matters and the direction in the first spline from the starting point, those are stored
	// Each Path begins at "A" and ends at the beginning or at the end of a spline
	TArray<FSoPointDesc> Path;
	// used to stop not optimal Paths
	TArray<FSoPointDesc> BestWayTable;
	BestWayTable.Reserve(30);
	Path.Reserve(30);

	// Algorithm:

	// Get next Path from FIFO
	// If the Path's endpoint and the target is in the same spline, update MinDist if necessary
	// A Neighbor Point is a point, which is:
	//		not in the same spline
	//		(0 distance away, located near this end of the spline, or
	//		SplineLength distance away, located near the other end of the spline)
	// Make new Path from each neighbor if
	//		New PathEndPoint wasn't reached yet, or it is closer this way, and
	//		New Path distance is < minimum target distance
	//		New Path distance is < MaxAcceptedDistance
	// Update BestWayTable && push new Paths to FIFO
	// Repeat until we run out of Paths

	// init values
	const FSoPointDesc S1 = { OtherPoint.Spline, true, OtherPoint.Distance, -OtherPoint.Spline->GetSplineDirection() };
	const FSoPointDesc S2 = { OtherPoint.Spline, false, OtherPoint.Spline->GetSplineComponent()->GetSplineLength() - OtherPoint.Distance, OtherPoint.Spline->GetSplineDirection() };
	Path.Add(S1);
	Path.Add(S2);
	BestWayTable.Add(S1);
	BestWayTable.Add(S2);

	int32 PathIndex = 0;

	while (PathIndex < Path.Num())
	{
		const FSoPointDesc& Point = Path[PathIndex];
		PathIndex += 1;

		// Goal Spline? Let's calculate distance to target point
		if (Point.Spline == Spline)
		{
			float NewDistance = Point.Distance;
			if (Point.bStartPoint)
				NewDistance += Distance;
			else
				NewDistance += (GetSpline()->GetSplineComponent()->GetSplineLength() - Distance);

			if (NewDistance < TargetDistance)
			{
				TargetDistance = NewDistance;
				TargetDirection = Point.Direction;
			}
		}

		// generate new neighbors
		{
			// first turn prev
			auto* Splines = Point.bStartPoint ? &Point.Spline->GetPrevSplines() : &Point.Spline->GetNextSplines();
			float ExtraDistance = 0.0f;
			for (int32 i = 0; i < 2; ++i) // second turn nexts
			{
				for (const auto& SplinePtr : *Splines)
				{
					if (SplinePtr == nullptr)
					{
						UE_LOG(LogSoSplineSys, Error, TEXT("Nullptr as neighbor spline for %s"), *Point.Spline->GetSplineName());
						break;
					}

					const float DistToThisPoint = Point.Distance + ExtraDistance;
					const FVector P0 = Point.bStartPoint ? Point.Spline->GetStartWorldLocation() : Point.Spline->GetEndWorldLocation();
					const FVector PS = SplinePtr->GetStartWorldLocation();
					const FVector PE = SplinePtr->GetEndWorldLocation();
					const bool bStartPoint = (SplinePtr == Point.Spline) ? (!Point.bStartPoint) : ((P0 - PS).SizeSquared2D() < (P0 - PE).SizeSquared2D());

					bool bAdd = (TargetDistance > DistToThisPoint + KINDA_SMALL_NUMBER) && (DistToThisPoint < MaxAcceptedDistance);
					bool bAlreadyInBestTable = false;
					if (bAdd)
					{
						for (auto& BestEntry : BestWayTable)
						{
							if (BestEntry.Spline == SplinePtr && BestEntry.bStartPoint == bStartPoint)
							{
								bAlreadyInBestTable = true;
								bAdd = BestEntry.Distance > DistToThisPoint + KINDA_SMALL_NUMBER;
								if (bAdd)
									BestEntry = { SplinePtr, bStartPoint, DistToThisPoint, Point.Direction };
								break;
							}
						}
						if (bAdd)
						{
							const FSoPointDesc NewEntry = { SplinePtr, bStartPoint, DistToThisPoint, Point.Direction };
							Path.Add(NewEntry);
							if (!bAlreadyInBestTable)
								BestWayTable.Add(NewEntry);
						}
					}
				}
				// switch from prev to next
				Splines = !Point.bStartPoint ? &Point.Spline->GetPrevSplines() : &Point.Spline->GetNextSplines();
				ExtraDistance = Point.Spline->GetSplineComponent()->GetSplineLength();
			} // generate new neighbors | {prev, next}
		}
	} // PathIndex < Paths.Num()
	// UE_LOG(LogSoSplineSys, Display, TEXT("Distance: %f"), TargetDirection * TargetDistance);
	return TargetDirection * TargetDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoSplinePoint::GetDistanceFromSplinePointWithZSquered(const FSoSplinePoint& OtherPoint, float MaxAcceptedDistance) const
{
	const float XDistance = GetDistanceFromSplinePoint(OtherPoint, MaxAcceptedDistance);
	const float ZDistance = GetReferenceZ() - OtherPoint.GetReferenceZ();

	return XDistance * XDistance + ZDistance * ZDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector FSoSplinePoint::GetWorldLocation(float ZValue) const
{
	FVector Location = *this;
	Location.Z = ZValue;
	return Location;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint::operator FVector() const
{
	if (Spline == nullptr)
	{
		UE_LOG(LogSoSplineSys, Warning, TEXT("FSoSplinePoint -> FVector cast failed, FSoSplinePoint doesn't have a valid SoSpline"));
		return FVector(0, 0, 0);
	}
	return Spline->GetWorldLocationAtDistance(Distance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint::operator FVector2D() const
{
	return FVector2D(FVector(*this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector2D FSoSplinePoint::ToVector2D() const
{
	if (Spline == nullptr)
	{
		UE_LOG(LogSoSplineSys, Warning, TEXT("FSoSplinePoint -> FVector2D failed, FSoSplinePoint doesn't have a valid SoSpline"));
		return FVector2D(0, 0);
	}
	return FVector2D(Spline->GetWorldLocationAtDistance(Distance));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector FSoSplinePoint::ToVector(float ZValue) const
{
	if (Spline == nullptr)
	{
		UE_LOG(LogSoSplineSys, Warning, TEXT("FSoSplinePoint::ToVector failed, FSoSplinePoint doesn't have a valid SoSpline"));
		return FVector(0, 0, 0);
	}
	FVector Vec = Spline->GetWorldLocationAtDistance(Distance);
	Vec.Z = ZValue;
	return Vec;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint::operator FRotator() const
{
	if (Spline == nullptr)
	{
		UE_LOG(LogSoSplineSys, Warning, TEXT("FSoSplinePoint -> FRotator cast failed, FSoSplinePoint doesn't have a valid SoSpline"));
		return FRotator();
	}
	return Spline->GetWorldRotationAtDistance(Distance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector FSoSplinePoint::GetPlaneNormal() const
{
	return FVector::CrossProduct(GetDirection(), FVector(0.f, 0.f, 1.f)).GetSafeNormal();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector FSoSplinePoint::GetDirection() const
{
	if (!Spline)
	{
		UE_LOG(LogSoSplineSys, Warning, TEXT("FSoSplinePoint::GetDirection called, FSoSplinePoint doesn't have a valid Spline"));
		return FVector(0, 0, 0);
	}

	return Spline->GetSplineComponent()->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World) * Spline->GetSplineDirection();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoSplinePoint::IsValid(bool bPrintWarning/* = true*/) const
{
	if (!Spline)
	{
		if (bPrintWarning)
		{
			FString ActorName = FString("???");
			if (ReferenceActor != nullptr)
				ActorName = ReferenceActor->GetName();
			UE_LOG(LogSoSplineSys, Warning, TEXT("FSoSplinePoint IsValid check inside %s failed: Spline is missing"), *ActorName);
		}
		return false;
	}

	if (Distance < -KINDA_SMALL_NUMBER || Distance > Spline->GetSplineComponent()->GetSplineLength() + KINDA_SMALL_NUMBER)
	{
		if (bPrintWarning)
			UE_LOG(LogSoSplineSys, Warning, TEXT("FSoSplinePoint IsValid check failed: Distance is out of range(%f outside of [0..%f]"), Distance, Spline->GetSplineComponent()->GetSplineLength());
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoSplinePoint::Invalidate()
{
	Spline = nullptr;
	Distance = 0.0f;
	ReferenceZ = 0.0f;
	ReferenceActor = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoSplinePoint::GetActiveSplineLength() const
{
	if (!Spline)
		return 0.f;
	return Spline->GetSplineComponent()->GetSplineLength();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint FSoSplinePoint::GetEstimatedSplineLocation(const FVector WorldLocation, const float MaxDistance) const
{
	FSoSplinePoint A = *this;
	FSoSplinePoint B = *this + MaxDistance;
	float ActualDistance = MaxDistance;

	for (int i = 0; i < 7; ++i)
	{
		ActualDistance /= 2.0f;

		if ((FVector(A) - WorldLocation).SizeSquared2D() > (FVector(B) - WorldLocation).SizeSquared2D())
			A = A + ActualDistance;
		else
			B = A + ActualDistance;
	}
	return A + ActualDistance / 2.0f;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector FSoSplinePoint::GetDirectionFromVector(const FVector V) const
{
	return FVector(FVector2D(GetDirection()), 0).GetSafeNormal() * GetDirectionModifierFromVector(V);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoSplinePoint::GetDirectionModifierFromVector(const FVector V) const
{
	const FVector2D Dir = FVector2D(GetDirection()).GetSafeNormal();
	const FVector2D V2D = FVector2D(V).GetSafeNormal();
	return ((Dir | V2D) < 0) ? -1 : 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoSplinePoint::GetReferenceZ() const
{
	return ReferenceActor == nullptr ? ReferenceZ : ReferenceActor->GetActorLocation().Z;
}

