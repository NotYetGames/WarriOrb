// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAFlyToPlayer.h"

#include "Engine/World.h"

#include "Enemy/SoEnemy.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Basic/Helpers/SoStaticHelper.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAFlyToPlayer::OnEnter(ASoEnemy* Owner)
{
	const FVector OldLocation = Owner->GetActorLocation();
	const FVector PlayerLocation = USoStaticHelper::GetPlayerCharacterAsActor(Owner)->GetActorLocation() +
								   USoStaticHelper::GetPlayerSplineLocation(Owner).GetDirection() * SplineOffset;
	bTargetReached = false;

	if (bAway)
	{
		FVector Direction = (OldLocation - PlayerLocation);
		Direction.Z = FMath::Max(Direction.Z, ZOffset);
		AwayDirection = Direction;
		CurrentVelocity = Direction * StartVelocity;

		RestTime = AwayTime;
	}
	else
	{
		const FVector TargetLocation = PlayerLocation + FVector(0.0f, 0.0f, ZOffset);
		CurrentVelocity = (TargetLocation - OldLocation).GetSafeNormal() * StartVelocity;
		ActualZOffset = ZOffset;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAFlyToPlayer::OnLeave(ASoEnemy* Owner)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAFlyToPlayer::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (DeltaSeconds < KINDA_SMALL_NUMBER)
		return !bTargetReached || CurrentVelocity.Size2D() > 1.0f;

	const FVector OldLocation = Owner->GetActorLocation();
	const FVector TargetLocation = bAway ? OldLocation + AwayDirection * 400
										 : USoStaticHelper::GetPlayerCharacterAsActor(Owner)->GetActorLocation() +
										   USoStaticHelper::GetPlayerSplineLocation(Owner).GetDirection() * SplineOffset +
										   FVector(0.0f, 0.0f, ActualZOffset);
	if (bTargetReached)
	{
		float ActualVelocity = CurrentVelocity.Size();
		ActualVelocity = FMath::Max(ActualVelocity - Acceleration * DeltaSeconds, 0.0f);
		CurrentVelocity = CurrentVelocity.GetSafeNormal() * ActualVelocity;
	}
	else
	{
		const FVector ToTarget = TargetLocation - OldLocation;
		ActualZOffset = FMath::Min(ZOffset, ToTarget.Size2D());
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
	if (bAway)
	{
		RestTime -= DeltaSeconds;
		bTargetReached = RestTime > 0.0f;
	}
	else
		bTargetReached = (DistanceSquared < AcceptedDistance * AcceptedDistance);

	return true;
}
