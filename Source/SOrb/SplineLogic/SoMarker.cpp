// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoMarker.h"

#include "SplineLogic/SoSplinePoint.h"
#include "SplineLogic/SoSpline.h"
#include "SplineLogic/SoSplineHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoMarker::ASoMarker()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	USoSplineHelper::UpdateSplineLocationRef(this, SplineLocationPtr, true, true);

	// store reference if they are on the same level - some child may work in editor
	if (SplineLocationPtr.IsValid())
	{
		FSoSplinePoint Point = SplineLocationPtr.Extract();
		if (Point.GetSpline()->GetLevel() == GetLevel())
			SplineLocation = Point;
	}
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoMarker::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == FName("DistanceAlongSpline"))
	{
		const FSoSplinePoint TempPoint = SplineLocationPtr.Extract();
		if (TempPoint.IsValid(false))
			SetActorLocation(TempPoint.ToVector(GetActorLocation().Z));
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoMarker::InitiliazeSplineLocation()
{
	SplineLocation = SplineLocationPtr.Extract();
	SplineLocation.SetReferenceZ(GetActorLocation().Z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoMarker::BeginPlay()
{
	Super::BeginPlay();
	InitiliazeSplineLocation();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoMarker::IsMarkerInRangeFromSplineLocation(const ASoMarker* Marker, const FSoSplinePoint& SplineLocation, float MinDistance, float MaxDistance)
{
	if (Marker == nullptr || !SplineLocation.IsValid(false))
		return false;

	if (!Marker->SplineLocation.IsValid(false))
		return 0.0f;

	const float Distance = fabs(SplineLocation.GetDistanceFromSplinePoint(Marker->SplineLocation, MaxDistance + 10.0f));
	return (Distance > MinDistance && Distance < MaxDistance);
}