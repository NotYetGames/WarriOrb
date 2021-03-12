// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SplineLogic/SoSplineHelper.h"

#include "EngineUtils.h"

#include "Components/SplineComponent.h"
#include "SplineLogic/SoSplineWalker.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "SplineLogic/SoSplinePointPtr.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

bool USoSplineHelper::bSplinePosCorrectionEnabled = true;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint USoSplineHelper::ExtractSplinePointPtr(const FSoSplinePointPtr& SplinePointPtr)
{
	return SplinePointPtr.Extract();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoSplineHelper::SetSplinePointFromPtr(UPARAM(Ref)FSoSplinePoint& SplineLocation, const FSoSplinePointPtr& SplinePointPtr)
{
	SplineLocation = SplinePointPtr.Extract();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePointPtr USoSplineHelper::ConstructSplinePointPtr(const FSoSplinePoint& SplineLocation)
{
	FSoSplinePointPtr Ptr(SplineLocation);
	return Ptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoSplineHelper::SetSplineLocationReferenceZ(UPARAM(Ref)FSoSplinePoint& SplineLocation, float ReferenceZ)
{
	SplineLocation.SetReferenceZ(ReferenceZ);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoSplineHelper::GetRandomLocationNearSplineLocation(UPARAM(Ref)FSoSplinePoint& SplineLocation, float Range, float LocationZ)
{
	if (!SplineLocation.IsValid())
		return FVector::ZeroVector;

	const float RangeMax = fabs((SplineLocation + Range) - SplineLocation);
	const float RangeMin = -fabs((SplineLocation - Range) - SplineLocation);

	const float Distance = FMath::RandRange(RangeMin, RangeMax);

	return (SplineLocation + Distance).GetWorldLocation(LocationZ);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoSplineHelper::UpdateSplineLocationRef(AActor* Actor,
											  UPARAM(Ref)FSoSplinePointPtr& SplineLocationPtr,
											  bool bPlayerSplineOnly,
											  bool bIgnoreZ,
											  bool bForcedCanModifySpline)
{
	bool bCanModifySpline = bForcedCanModifySpline;
	bool bJustReturn = !bSplinePosCorrectionEnabled;
#if WITH_EDITOR
	if (GEditor && GEditor->GetActiveViewport())
	{
		bCanModifySpline = bCanModifySpline || GEditor->GetActiveViewport()->KeyState(EKeys::E);
		bJustReturn = bJustReturn || GEditor->GetActiveViewport()->KeyState(EKeys::Q);
	}
#endif

	if (bJustReturn || Actor == nullptr)
		return;

	FSoSplinePoint SplineLocation = SplineLocationPtr.Extract();
	const FVector ActorLocation = Actor->GetActorLocation();
	FVector NewLocation = FVector(0, 0, 0);
	if (bCanModifySpline)
	{
		// iterate through all spline, find the closest
		struct FSoSplineResult
		{
			float DistanceSquared;
			float InputKey;
			ASoSpline* Spline;
		};

		FSoSplineResult BestFit = { BIG_NUMBER, 0.0f, nullptr };

		TActorIterator<ASoSpline> SplineItr(Actor->GetWorld());
		TActorIterator<ASoPlayerSpline> PlayerSplineItr(Actor->GetWorld());
		while ((bPlayerSplineOnly && PlayerSplineItr) || (!bPlayerSplineOnly && SplineItr))
		{
			ASoSpline* SoSpline = bPlayerSplineOnly ? Cast<ASoSpline>(*PlayerSplineItr) : *SplineItr;
			// GetSplineComponent()->GetSplinePointPosition().InaccurateFindNearest(WhateverLocation, DistanceSquared); doesn't work, distance is wrong!!!
			FVector OldLoc = ActorLocation;
			if (bIgnoreZ)
				OldLoc.Z = SoSpline->GetWorldLocationAtDistance(0).Z;
			const float InputKey = SoSpline->GetSplineComponent()->FindInputKeyClosestToWorldLocation(OldLoc);
			const FVector Loc = SoSpline->GetSplineComponent()->GetLocationAtSplineInputKey(InputKey, ESplineCoordinateSpace::World);
			const float SizeSquared = (Loc - OldLoc).SizeSquared();
			if (SizeSquared < BestFit.DistanceSquared)
				BestFit = { SizeSquared, InputKey, SoSpline };

			if (bPlayerSplineOnly)
				++PlayerSplineItr;
			else
				++SplineItr;
		}

		if (BestFit.Spline != nullptr)
		{
			SplineLocation.SetSpline(BestFit.Spline);
			SplineLocation.SetDistanceFromInputKey(BestFit.InputKey);
			NewLocation = BestFit.Spline->GetSplineComponent()->GetLocationAtSplineInputKey(BestFit.InputKey, ESplineCoordinateSpace::World);
		}
	}
	else
	{
		const ASoSpline* SoSpline = SplineLocation.GetSpline();
		if (SoSpline != nullptr)
		{
			FVector Loc = ActorLocation;
			if (bIgnoreZ)
				Loc.Z = SoSpline->GetWorldLocationAtDistance(0).Z;
			const float InputKey = SoSpline->GetSplineComponent()->FindInputKeyClosestToWorldLocation(Loc);
			SplineLocation.SetDistanceFromInputKey(InputKey);
			NewLocation = SoSpline->GetSplineComponent()->GetLocationAtSplineInputKey(InputKey, ESplineCoordinateSpace::World);
		}
	}

	const ASoSpline* SoSpline = SplineLocation.GetSpline();
	if (SoSpline != nullptr)
		Actor->SetActorLocation(FVector(NewLocation.X, NewLocation.Y, ActorLocation.Z));

	if (SplineLocation.GetSpline() != nullptr)
		SplineLocationPtr = FSoSplinePointPtr(SplineLocation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoSplineHelper::ClampSplinePoint(FSoSplinePoint& SplinePoint)
{
	SplinePoint.ClampSplineData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint USoSplineHelper::GetOffsetedSplinePoint(const FSoSplinePoint& SplinePoint, float Delta)
{
	return SplinePoint + Delta;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoSplineHelper::GetWorldLocationFromSplinePoint(const FSoSplinePoint& SplinePoint)
{
	return SplinePoint;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoSplineHelper::GetWorldLocationFromSplinePointZ(const FSoSplinePoint& SplinePoint, float ZValue)
{
	const FVector F = SplinePoint;
	return FVector(F.X, F.Y, ZValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FRotator USoSplineHelper::GetSplineLookAtRotation(const FSoSplinePoint& Source, const FSoSplinePoint& Target)
{
	const float TargetDir = FMath::Sign((Target.GetDistanceFromSplinePoint(Source, 10000)));
	return (Source.GetDirection() * TargetDir).Rotation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 USoSplineHelper::GetSplineLookAtSign(const FSoSplinePoint& Source, const FSoSplinePoint& Target)
{
	return FMath::Sign((Target.GetDistanceFromSplinePoint(Source, 10000)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoSplineHelper::GetSplineVelocity(const FSoSplinePoint& SplinePoint, float SplineDirVelocity, float ZVelocity)
{
	return FVector(FVector2D(SplinePoint.GetDirection() * SplineDirVelocity), ZVelocity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector2D USoSplineHelper::GetDirVec2D(const FSoSplinePoint& StartSP, float StartZ, const FSoSplinePoint& EndSP, float EndZ)
{
	return FVector2D(EndSP - StartSP, EndZ - StartZ).GetSafeNormal();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoSplineHelper::CheckSplinePoint(const FSoSplinePoint& SplinePoint, bool bPrintWarnings)
{
	return SplinePoint.IsValid(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoSplineHelper::GetDistanceBetweenSplinePoints(const FSoSplinePoint& First, const FSoSplinePoint& Second)
{
	return Second - First;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoSplineHelper::IsSplinepointBetweenPoints(const FSoSplinePoint& Point, const FSoSplinePoint& Start, const FSoSplinePoint& End)
{
	const float SegmentLength = fabs(End - Start);

	const float StartDistance = fabs(Point - Start);
	const float EndDistance = fabs(Point - End);

	return (StartDistance < SegmentLength) && (EndDistance < SegmentLength);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoSplineHelper::ClampOffsetBetweenSplinePoints(const FSoSplinePoint& Source,
													  const FSoSplinePoint& Border0,
													  const FSoSplinePoint& Border1,
													  float Offset)
{
	const float DistanceToBorder0 = Border0.GetDistanceFromSplinePoint(Source);
	const float DistanceToBorder1 = Border1.GetDistanceFromSplinePoint(Source);

	if (fabs(DistanceToBorder0) < fabs(Offset) && FMath::Sign(DistanceToBorder0) == FMath::Sign(Offset))
		Offset = DistanceToBorder0;
	else if (fabs(DistanceToBorder1) < fabs(Offset) && FMath::Sign(DistanceToBorder1) == FMath::Sign(Offset))
		Offset = DistanceToBorder1;

	return Offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoSplineHelper::IsSplinePointTargetInRange(const FSoSplinePoint& Source,
												 const FSoSplinePoint& Target,
												 const FVector& ReferenceDirection,
												 float RangeDistance,
												 bool& OutForward,
												 bool& OutInRange)
{
	OutInRange = false;
	const float Distance = Target.GetDistanceFromSplinePoint(Source, RangeDistance);
	if (Distance <= RangeDistance)
	{
		OutInRange = true;
		OutForward = FMath::Sign(Source.GetDirectionModifierFromVector(ReferenceDirection)) == FMath::Sign(Distance);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoSplineHelper::PushSplineWalker(AActor* ActorToPush,
									   const FVector& DeltaMovement, 
									   float DeltaSeconds, 
									   FHitResult* HitResult,
									   AActor* RematerializeLocation,
									   int32 DamageAmountIfStuck,
									   bool bForceDamage)
{
	// TODO: maybe ground should be considered (if it is not horizontal the object does not move if it would be pushed into the ground, it should move along the surface)
	const FVector OldActorLocation = ActorToPush->GetActorLocation();
	const FSoSplinePoint SplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(ActorToPush);

	if (!SplineLocation.IsValid(false))
		return false;

	const int32 DirModifer = SplineLocation.GetDirectionModifierFromVector(DeltaMovement);
	FSoSplinePoint NewSplinePoint = SplineLocation.GetEstimatedSplineLocation(OldActorLocation + DeltaMovement, DeltaMovement.Size() * DirModifer);
	FHitResult HitRes;
	const FVector PointOnSpline = FVector(NewSplinePoint);

	ActorToPush->SetActorLocation(FVector(PointOnSpline.X, PointOnSpline.Y, OldActorLocation.Z + DeltaMovement.Z), HitResult != nullptr, HitResult);
	bool bStuck = false;
	if (HitResult != nullptr && HitResult->bBlockingHit)
	{
		const FVector DirNormal = DeltaMovement.GetSafeNormal();
		const FVector NegativeHitNormal = -HitResult->ImpactNormal;

		// if ((DirNormal | NegativeHitNormal) > 0.5f)
		{
			NewSplinePoint = SplineLocation + (NewSplinePoint - SplineLocation) * HitResult->Time;
			bStuck = true;
		}
	}

	ISoSplineWalker::Execute_SetSplineLocation(ActorToPush, NewSplinePoint, true);
	ISoSplineWalker::Execute_OnPushed(ActorToPush, DeltaMovement, DeltaSeconds, bStuck, RematerializeLocation, DamageAmountIfStuck, bForceDamage);

	return true;
}
