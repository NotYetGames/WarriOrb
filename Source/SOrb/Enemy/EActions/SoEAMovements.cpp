// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAMovements.h"


#include "Basic/Helpers/SoDateTimeHelper.h"
#include "Engine/World.h"

#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Enemy/SoEnemy.h"
#include "Enemy/SoEnemyHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SplineLogic/SoMarker.h"
#include "SplineLogic/SoSplineHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveTo::OnEnter(ASoEnemy* Owner)
{
	TimeSinceProgress = 0.0f;
	OwnerLastFramePos = Owner->GetActorLocation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAMoveTo::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	const float Distance = TargetPoint - ISoSplineWalker::Execute_GetSplineLocationI(Owner);

	if (fabs(Distance) < AcceptedDistance)
		return false;

	const FVector OwnerPos = Owner->GetActorLocation();
	if (bGiveUpIfNoProgress)
	{
		if (fabs((OwnerPos - OwnerLastFramePos).SizeSquared()) > KINDA_SMALL_NUMBER)
			TimeSinceProgress = 0.0f;
		else
			TimeSinceProgress += DeltaSeconds;

		DistanceLastFrame = Distance;

		if (TimeSinceProgress > FMath::Max(DeltaSeconds, 0.3f))
			return false;
	}

	Owner->AddMovementInput(ISoSplineWalker::Execute_GetSplineLocationI(Owner).GetDirection(), FMath::Sign(Distance));
	OwnerLastFramePos = Owner->GetActorLocation();
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveToMarker::OnEnter(ASoEnemy* Owner)
{
	Super::OnEnter(Owner);

	Owner->GetSplineLocationFromMarker(Index, TargetPoint);
	if (TargetPoint.IsValid())
		return;

	UE_LOG(LogSoEnemyAI, Warning, TEXT("USoEAMoveToTargetMarker: invalid marker or marker index (%d)"), Index);
	TargetPoint = ISoSplineWalker::Execute_GetSplineLocationI(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAMoveToPlayer::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	TargetPoint = USoStaticHelper::GetPlayerSplineLocation(Owner) + SplineOffset;

	if (bOnlyBetweenMarkers && !Owner->IsPlayerBetweenMarkers(MarkerIndex0, MarkerIndex1, bCheckResultOnInvalidMarkers))
	{
		return false;
	}

	if (bClampBetweenMarkers)
	{
		ASoMarker* Marker0 = Owner->GetTargetMarker(MarkerIndex0);
		ASoMarker* Marker1 = Owner->GetTargetMarker(MarkerIndex1);
		if (Marker0 != nullptr && Marker1 != nullptr)
		{
			const float MarkerDist0 = Marker0->GetSplineLocation().GetDistance();
			const float MarkerDist1 = Marker1->GetSplineLocation().GetDistance();
			TargetPoint.SetDistance(FMath::Clamp(TargetPoint.GetDistance(), FMath::Min(MarkerDist0, MarkerDist1), FMath::Max(MarkerDist0, MarkerDist1)));
		}
	}


	return USoEAMoveTo::OnTick(DeltaSeconds, Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveToPlayerForDuration::OnEnter(ASoEnemy* Owner)
{
	Super::OnEnter(Owner);
	RestTime = Duration;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAMoveToPlayerForDuration::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	USoEAMoveToPlayer::OnTick(DeltaSeconds, Owner);
	RestTime -= DeltaSeconds;
	return RestTime > 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAMoveAwayFromPlayer::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Timer += DeltaSeconds;
	if (MaxMoveTime < Timer)
		return false;

	const FSoSplinePoint PlayerSplineLoc = USoStaticHelper::GetPlayerSplineLocation(Owner);
	const FSoSplinePoint OwnerSplineLoc = ISoSplineWalker::Execute_GetSplineLocationI(Owner);
	const float Distance = PlayerSplineLoc.GetDistanceFromSplinePoint(OwnerSplineLoc, 1000);

	if (fabs(Distance) < AcceptedDistance)
	{
		if (bBetweenMarkers)
		{
			ASoMarker* Marker0 = Owner->GetTargetMarker(0);
			ASoMarker* Marker1 = Owner->GetTargetMarker(1);
			if (Marker0 != nullptr && Marker1 != nullptr)
			{
				const float PlayerToMarker0 = PlayerSplineLoc.GetDistanceFromSplinePoint(Marker0->GetSplineLocation());
				const float PlayerToMarker1 = PlayerSplineLoc.GetDistanceFromSplinePoint(Marker1->GetSplineLocation());
				ASoMarker* SelectedMarker = (fabs(PlayerToMarker1) < fabs(PlayerToMarker0)) ? Marker0 : Marker1;

				const float SelectedDistance = Owner->GetSoMovement()->GetSplineLocation().GetDistanceFromSplinePoint(SelectedMarker->GetSplineLocation());
				if (fabs(SelectedDistance) < MarkerDistanceThreshold)
					return false;
			}
		}

		Owner->AddMovementInput(OwnerSplineLoc.GetDirection(), -FMath::Sign(Distance));
		if (bFacePlayer)
		{
			const float TargetDir = FMath::Sign((PlayerSplineLoc.GetDistanceFromSplinePoint(OwnerSplineLoc, 10000)));
			Owner->SetActorRotation((OwnerSplineLoc.GetDirection() * TargetDir).Rotation());
		}
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveAwayFromPlayer::OnEnter(ASoEnemy* Owner)
{
	Super::OnEnter(Owner);

	Timer = 0.0f;
	Owner->GetSoMovement()->bOrientRotationToMovement = !bFacePlayer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveAwayFromPlayer::OnLeave(ASoEnemy* Owner)
{
	Super::OnLeave(Owner);
	Owner->GetSoMovement()->bOrientRotationToMovement = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAMoveAwayFromClosestAlly::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Timer += DeltaSeconds;
	if (MaxMoveTime < Timer)
		return false;

	const FSoSplinePoint OwnerSplineLoc = ISoSplineWalker::Execute_GetSplineLocationI(Owner);
	const float Distance = USoEnemyHelper::GetSignedSplineDistanceToClosestEnemy(Owner, bIgnoreFloatings, bOnlySameClass);
	if (fabs(Distance) < AcceptedDistance)
	{
		Owner->AddMovementInput(OwnerSplineLoc.GetDirection(), FMath::Sign(Distance));
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveAwayFromClosestAlly::OnEnter(ASoEnemy* Owner)
{
	Super::OnEnter(Owner);

	Timer = 0.0f;
	Owner->GetSoMovement()->bOrientRotationToMovement = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveAwayFromClosestAlly::OnLeave(ASoEnemy* Owner)
{
	Super::OnLeave(Owner);
	Owner->GetSoMovement()->bOrientRotationToMovement = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEATurnToPlayer::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Owner->LookAtPlayer();
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEATurn::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Owner->SetActorRotation((-Owner->GetActorForwardVector()).Rotation());
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAMoveTowardsDirection::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Counter += DeltaSeconds;

	Owner->AddMovementInput(ISoSplineWalker::Execute_GetSplineLocationI(Owner).GetDirectionFromVector(Owner->GetActorForwardVector()), DirectionMultiplier);
	return Counter < Duration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveTowardsDirection::OnEnter(ASoEnemy* Owner)
{
	Super::OnEnter(Owner);

	Counter = 0.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAJumpTo::OnEnter(ASoEnemy* Owner)
{
	Super::OnEnter(Owner);

	USoCharacterMovementComponent* SoMovement = Owner->GetSoMovement();
	CachedWalkSpeed = SoMovement->MaxWalkSpeed;

	CalculateTarget(Owner);
	float Distance = SoMovement->GetSplineLocation().GetDistanceFromSplinePoint(TargetPoint);
	Distance += FMath::Sign(Distance) * DistanceOffset;

	if (bClampToMarkers)
	{
		ASoMarker* Marker0 = Owner->GetTargetMarker(0);
		ASoMarker* Marker1 = Owner->GetTargetMarker(1);
		if (Marker0 != nullptr && Marker1 != nullptr)
		{
			Distance = USoSplineHelper::ClampOffsetBetweenSplinePoints(SoMovement->GetSplineLocation(),
																	   Marker0->GetSplineLocation(),
																	   Marker1->GetSplineLocation(),
																	   Distance);
		}
	}

	const float Time = 2 * JumpZ / SoMovement->GetGravityZ();
	SoMovement->MaxWalkSpeed = fabs(Distance / Time);

	const FVector Direction = -SoMovement->GetSplineLocation().GetDirection() * FMath::Sign(Distance);
	SoMovement->Velocity = Direction * SoMovement->MaxWalkSpeed;

	SoMovement->JumpZVelocity = JumpZ;
	SoMovement->DoJump(false);

	Super::OnEnter(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAJumpTo::OnLeave(ASoEnemy* Owner)
{
	Owner->GetSoMovement()->MaxWalkSpeed = CachedWalkSpeed;
	Super::OnLeave(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAJumpTo::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Super::OnTick(DeltaSeconds, Owner);
	if (bClampToMarkers && !Owner->IsBetweenMarkers(0, 1, true))
	{
		ASoMarker* Marker0 = Owner->GetTargetMarker(0);
		ASoMarker* Marker1 = Owner->GetTargetMarker(1);
		if (Marker0 != nullptr && Marker1 != nullptr)
		{
			USoCharacterMovementComponent* SoMovement = Owner->GetSoMovement();
			const float DistanceToMarker0 = SoMovement->GetSplineLocation().GetDistanceFromSplinePoint(Marker0->GetSplineLocation());
			const float DistanceToMarker1 = SoMovement->GetSplineLocation().GetDistanceFromSplinePoint(Marker1->GetSplineLocation());
			ASoMarker* SelectedMarker = fabs(DistanceToMarker0) < fabs(DistanceToMarker1) ? Marker0 : Marker1;

			SoMovement->SetSplineLocation(SelectedMarker->GetSplineLocation());
			Owner->SetActorLocation(SoMovement->GetSplineLocation().ToVector(Owner->GetActorLocation().Z), true);
		}
	}

	// TODO: why is this here, this seems useless
	const float CurrentTime =  Owner->GetWorld()->GetTimeSeconds();
	const float ThresholdSeconds = LastUsageTime + 0.2f;
	return !Owner->GetSoMovement()->IsMovingOnGround() || ThresholdSeconds > CurrentTime;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAJumpTo::CalculateTarget(ASoEnemy* Owner)
{
	TargetPoint = Owner->GetSoMovement()->GetSplineLocation();
	TargetPoint += TargetPoint.GetDirectionModifierFromVector(Owner->GetActorForwardVector()) * 5.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAJumpToPlayer::CalculateTarget(ASoEnemy* Owner)
{
	TargetPoint = USoStaticHelper::GetPlayerSplineLocation(Owner);
}
