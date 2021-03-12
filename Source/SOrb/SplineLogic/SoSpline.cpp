// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoSpline.h"

#include "Components/SplineComponent.h"

#include "Basic/SoGameMode.h"
#include "SoSplinePoint.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoSpline::ASoSpline(const FObjectInitializer& ObjectInitializer)
{
 	// spline tick isn't required
	PrimaryActorTick.bCanEverTick = false;

	Spline = ObjectInitializer.CreateDefaultSubobject<USplineComponent>(this, TEXT("Spline"));
	RootComponent = Spline;
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSpline::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// actor name update
	SplineName = GetActorLabel();

	// spline prev/next z level updates
	const int32 DesiredPrevSize = FMath::Max(PrevSplines.Num() - 1, 0);
	const int32 OldPrevSize = PrevZLevels.Num();
	if (PrevZLevels.Num() != DesiredPrevSize)
		PrevZLevels.SetNum(DesiredPrevSize);

	const FVector PrevLoc = Spline->GetLocationAtDistanceAlongSpline(15, ESplineCoordinateSpace::Local);
	for (int32 i = 0; i < DesiredPrevSize; ++i)
	{
		PrevZLevels[i].X = PrevLoc.X;
		PrevZLevels[i].Y = PrevLoc.Y;
		if (i >= OldPrevSize)
			PrevZLevels[i].Z = PrevLoc.Z;
	}

	const int32 DesiredNextSize = FMath::Max(NextSplines.Num() - 1, 0);
	const int32 OldNextSize = NextZLevels.Num();
	if (NextZLevels.Num() != DesiredNextSize)
		NextZLevels.SetNum(DesiredNextSize);

	const FVector NextLoc = Spline->GetLocationAtDistanceAlongSpline(Spline->GetSplineLength() - 15, ESplineCoordinateSpace::Local);
	for (int32 i = 0; i < DesiredNextSize; ++i)
	{
		NextZLevels[i].X = NextLoc.X;
		NextZLevels[i].Y = NextLoc.Y;
		if (i >= OldNextSize)
			NextZLevels[i].Z = NextLoc.Z;
	}

	// AI points
	if (bShowAIStopPoints)
	{
		if (AIStopPointsV.Num() != 0)
		{
			AIStopPoints.SetNum(AIStopPointsV.Num());
			auto Point = FSoSplinePoint(this);
			for (int32 i = 0; i < AIStopPointsV.Num(); ++i)
			{
				FVector WorldPoint = GetTransform().TransformPosition(AIStopPointsV[i]);
				WorldPoint.Z = GetActorLocation().Z;
				Point.SetDistanceFromWorldLocation(WorldPoint);
				AIStopPoints[i] = Point.GetDistance();
			}
		}

		// sort based on distance to keep the data valid
		AIStopPoints.Sort([](const float& First, const float& Second) { return First < Second; });

		AIStopPointsV.SetNum(AIStopPoints.Num());
		for (int32 i = 0; i < AIStopPoints.Num(); ++i)
			AIStopPointsV[i] = Spline->GetLocationAtDistanceAlongSpline(AIStopPoints[i], ESplineCoordinateSpace::Local);
	}
	else
		AIStopPointsV.Empty();


	// direction sanity check
	if (Direction != 1)
		Direction = -1;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// returns one spline which is "after" this spline
ASoSpline* ASoSpline::GetNextSpline(const float ZCoordinate) const
{
	if (NextSplines.Num() == 0)
		return nullptr;

	check(NextZLevels.Num() == NextSplines.Num() - 1);

	int32 Index = 0;
	// Spline->GetComponentLocation().Z +  is enough to convert to world space
	// because the spline has to be horizontal and only the Z coordinate matters anyway
	while (Index < NextZLevels.Num() && Spline->GetComponentLocation().Z + NextZLevels[Index].Z < ZCoordinate)
		++Index;

	return NextSplines[Index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// returns one spline which is "before" this spline
ASoSpline* ASoSpline::GetPrevSpline(const float ZCoordinate) const
{
	if (PrevSplines.Num() == 0)
		return nullptr;

	check(PrevZLevels.Num() == PrevSplines.Num() - 1);

	int32 Index = 0;
	// Spline->GetComponentLocation().Z +  is enough to convert to world space
	// because the spline has to be horizontal and only the Z coordinate matters anyway
	while (Index < PrevZLevels.Num() && Spline->GetComponentLocation().Z + PrevZLevels[Index].Z < ZCoordinate)
		++Index;

	return PrevSplines[Index];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoSpline::GetWorldLocationAtDistance(const float Distance) const
{
	return Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRotator ASoSpline::GetWorldRotationAtDistance(const float Distance) const
{
	return Spline->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoSpline::GetStartWorldLocation() const
{
	return Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoSpline::GetEndWorldLocation() const
{
	return Spline->GetLocationAtSplinePoint(Spline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSpline::IsInPrevSplines(ASoSpline* SplineToCheck) const
{
	for (int i = 0; i < PrevSplines.Num(); ++i)
		if (PrevSplines[i] == SplineToCheck) return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoSpline::IsInNextSplines(ASoSpline* SplineToCheck) const
{
	for (int i = 0; i < NextSplines.Num(); ++i)
		if (NextSplines[i] == SplineToCheck) return true;

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSpline::MakeCircleSpline(ASoSpline* Spline, float Radius, float TangentModifier, const FVector Scale)
{
	if (Spline == nullptr)
		return;

	USplineComponent* SplineComponent = Spline->Spline;

	const TArray<FVector> Points = {{ Radius,	0.0f,	0.0f },
									{ 0.0f,		Radius, 0.0f },
									{ -Radius,	0.0f,	0.0f },
									{ 0.0f,	   -Radius, 0.0f },
									{ Radius,	0.0f,	0.0f }};



	SplineComponent->SetSplinePoints(Points, ESplineCoordinateSpace::Local, true);


	SplineComponent->SetTangentAtSplinePoint(0, { 0.0f, Radius * TangentModifier, 0.0f }, ESplineCoordinateSpace::Local, true);
	SplineComponent->SetTangentAtSplinePoint(4, { 0.0f, Radius * TangentModifier, 0.0f }, ESplineCoordinateSpace::Local, true);
	SplineComponent->SetTangentAtSplinePoint(2, { 0.0f, -Radius * TangentModifier, 0.0f }, ESplineCoordinateSpace::Local, true);

	SplineComponent->SetTangentAtSplinePoint(1, { -Radius * TangentModifier, 0.0f, 0.0f }, ESplineCoordinateSpace::Local, true);
	SplineComponent->SetTangentAtSplinePoint(3, { Radius * TangentModifier, 0.0f, 0.0f }, ESplineCoordinateSpace::Local, true);

	TArray<FInterpCurvePointVector>& ScalePoints = SplineComponent->GetSplinePointsScale().Points;
	for (int32 i = 0; i < ScalePoints.Num(); ++i)
		ScalePoints[i].OutVal = Scale;

	SplineComponent->UpdateSpline();

	if (Spline->NextSplines.Num() == 0 || Spline->NextSplines[0] == nullptr)
		Spline->NextSplines = { Spline };

	if (Spline->PrevSplines.Num() == 0 || Spline->PrevSplines[0] == nullptr)
		Spline->PrevSplines = { Spline };
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSpline::MakeSymmetry(ASoSpline* Spline)
{
	if (Spline == nullptr)
		return;

	USplineComponent* SplineComponent = Spline->GetSplineComponent();

	const int32 SplinePointNum = SplineComponent->GetNumberOfSplinePoints();
	if (SplinePointNum < 2)
		return;

	FVector Center = { 0.0f, 0.0f, 0.0f };

	if ((SplinePointNum % 2) == 0)
	{
		const FVector Pos0 = SplineComponent->GetLocationAtSplinePoint(SplinePointNum / 2, ESplineCoordinateSpace::Local);
		const FVector Pos1 = SplineComponent->GetLocationAtSplinePoint(SplinePointNum / 2 - 1, ESplineCoordinateSpace::Local);
		Center = (Pos0 + Pos1) / 2.0f;
	}
	else
		Center = SplineComponent->GetLocationAtSplinePoint(SplinePointNum / 2, ESplineCoordinateSpace::Local);

	const int32 FirstIndex = (SplinePointNum / 2) + (SplinePointNum % 2);

	for (int32 i = FirstIndex; i < SplinePointNum; ++i)
	{
		const int32 ToCopy = SplinePointNum - i - 1;
		check(ToCopy >= 0 && ToCopy < SplinePointNum);

		// position
		const FVector MirrorPosition = SplineComponent->GetLocationAtSplinePoint(ToCopy, ESplineCoordinateSpace::Local);
		FVector NewPosition = Center + (Center - MirrorPosition);
		NewPosition.Z = MirrorPosition.Z;
		SplineComponent->SetLocationAtSplinePoint(i, NewPosition, ESplineCoordinateSpace::Local, true);

		// rotation
		TArray<FInterpCurvePointQuat>& RotationPoints = SplineComponent->GetSplinePointsRotation().Points;
		if (ToCopy < RotationPoints.Num() && i < RotationPoints.Num())
			RotationPoints[i].OutVal = RotationPoints[ToCopy].OutVal;

		// scale
		TArray<FInterpCurvePointVector>& ScalePoints = SplineComponent->GetSplinePointsScale().Points;
		if (ToCopy < ScalePoints.Num() && i < ScalePoints.Num())
			ScalePoints[i].OutVal = ScalePoints[ToCopy].OutVal;

		// up vector
		SplineComponent->SetUpVectorAtSplinePoint(i,
												  SplineComponent->GetUpVectorAtSplinePoint(ToCopy, ESplineCoordinateSpace::Local),
												  ESplineCoordinateSpace::Local);
		// tangent
		SplineComponent->SetTangentAtSplinePoint(i,
												 SplineComponent->GetTangentAtSplinePoint(ToCopy, ESplineCoordinateSpace::Local) * FVector(1.0f, 1.0f, -1.0f),
												 ESplineCoordinateSpace::Local);
	}
}
