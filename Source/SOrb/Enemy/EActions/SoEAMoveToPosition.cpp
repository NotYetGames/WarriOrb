// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAMoveToPosition.h"

#include "Engine/World.h"

#include "Enemy/SoEnemy.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SplineLogic/SoMarker.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveToPosition::OnEnter(ASoEnemy* Owner)
{
	InitialSplineLocation = Owner->GetSoMovement()->GetSplineLocation();
	InitialZ = Owner->GetActorLocation().Z;

	Counter = 0.0f;

	TargetZ = bToPlayer ? USoStaticHelper::GetPlayerCharacterAsActor(Owner)->GetActorLocation().Z + ZOffset
						: Owner->GetInitialLocation().Z + ZOffset;

	TargetX = bToPlayer ? (USoStaticHelper::GetPlayerSplineLocation(Owner) + SplineOffset).GetDistance()
						: (Owner->GetInitialSplineLocation() + SplineOffset).GetDistance();

	if (bClamBetweenMarkers)
	{
		ASoMarker* Marker0 = Owner->GetTargetMarker(MarkerIndex0);
		ASoMarker* Marker1 = Owner->GetTargetMarker(MarkerIndex1);
		if (Marker0 != nullptr && Marker1 != nullptr)
		{
			const float MarkerDist0 = Marker0->GetSplineLocation().GetDistance();
			const float MarkerDist1 = Marker1->GetSplineLocation().GetDistance();
			TargetX = FMath::Clamp(TargetX, FMath::Min(MarkerDist0, MarkerDist1), FMath::Max(MarkerDist0, MarkerDist1));
		}
	}

	Super::OnEnter(Owner);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAMoveToPosition::OnLeave(ASoEnemy* Owner)
{
	Super::OnLeave(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAMoveToPosition::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Counter = FMath::Min(Counter + DeltaSeconds, Duration);

	const float Percent = Counter / Duration;

	float CurrentZ = InitialZ;
	FSoSplinePoint CurrentX = InitialSplineLocation;

	if (bAlongZ)
		CurrentZ = bSmoothStep ? USoMathHelper::InterpolateSmoothStep(InitialZ, TargetZ, Percent)
							   : USoMathHelper::InterpolateDeceleration(InitialZ, TargetZ, Percent);

	if (bAlongSpline)
	{
		CurrentX.SetDistance(bSmoothStep ? USoMathHelper::InterpolateSmoothStep(CurrentX.GetDistance(), TargetX, Percent)
										 : USoMathHelper::InterpolateDeceleration(CurrentX.GetDistance(), TargetX, Percent));

		Owner->SetActorLocation(CurrentX.GetWorldLocation(CurrentZ));
		Owner->GetSoMovement()->SetSplineLocation(CurrentX);
	}
	else
	{
		FVector Location = Owner->GetActorLocation();
		Location.Z = CurrentZ;
		Owner->SetActorLocation(Location);
	}

	return Counter < Duration - KINDA_SMALL_NUMBER;
}
