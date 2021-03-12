// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAFlyHome.h"

#include "Engine/World.h"

#include "Enemy/SoEnemy.h"
#include "CharacterBase/SoCharacterMovementComponent.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAFlyHome::OnEnter(ASoEnemy* Owner)
{
	const FVector OldLocation = Owner->GetActorLocation();
	bTargetReached = false;

	const FVector TargetLocation = Owner->GetInitialLocation();
	if (bOnlyAlongZ)
		CurrentVelocity = FVector::UpVector * FMath::Sign(TargetLocation.Z - OldLocation.Z) * StartVelocity;
	else
		CurrentVelocity = (TargetLocation - OldLocation).GetSafeNormal() * StartVelocity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAFlyHome::OnLeave(ASoEnemy* Owner)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAFlyHome::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (DeltaSeconds < KINDA_SMALL_NUMBER)
		return !bTargetReached || CurrentVelocity.Size2D() > 1.0f;

	const FVector OldLocation = Owner->GetActorLocation();
	const FVector TargetLocation = bOnlyAlongZ ? FVector(OldLocation.X, OldLocation.Y, Owner->GetInitialLocation().Z) : Owner->GetInitialLocation();
	if (bTargetReached)
	{
		float ActualVelocity = CurrentVelocity.Size();
		ActualVelocity = FMath::Max(ActualVelocity - Acceleration * DeltaSeconds, 0.0f);
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * ActualVelocity;
	}
	else
	{
		const FVector ToTarget = TargetLocation - OldLocation;
		CurrentVelocity += ToTarget.GetSafeNormal() * DeltaSeconds * Acceleration;
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * FMath::Min(MaxVelocity, CurrentVelocity.Size());
	}

	FHitResult HitResult;
	Owner->AddActorWorldOffset(CurrentVelocity * DeltaSeconds, true, &HitResult);
	const FVector CurrentLocation = Owner->GetActorLocation();
	const FVector Delta = (CurrentLocation - OldLocation);
	CurrentVelocity = Delta / DeltaSeconds;

	FSoSplinePoint& SplineLocation = Owner->GetSoMovement()->GetSplineLocation();
	SplineLocation += (Delta.Size2D() * SplineLocation.GetDirectionModifierFromVector(Delta));

	if (bTargetReached)
		return CurrentVelocity.Size2D() > 1.0f;

	const float DistanceSquared = (TargetLocation - CurrentLocation).SizeSquared();
	bTargetReached = (DistanceSquared < AcceptedDistance * AcceptedDistance);
	return true;
}
