// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEATeleportToMarker.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Enemy/SoEnemy.h"
#include "SplineLogic/SoMarker.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEATeleportToMarker::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	const float NewFrameTime = LastFrameTime + DeltaSeconds;
	if (LastFrameTime < TeleportDelay && NewFrameTime >= TeleportDelay)
	{
		FSoSplinePoint SplineLocation;
		if (Owner->GetSplineLocationFromMarker(Index, SplineLocation))
		{
			const FVector Location = SplineLocation.GetWorldLocation(SplineLocation.GetReferenceZ());
			if (bPutToGround)
			{
				// TODO: DO STUFF
			}

			Owner->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
			SplineLocation.SetReferenceActor(Owner);
			Owner->GetSoMovement()->SetSplineLocation(SplineLocation);
		}
		else
			UE_LOG(LogSoEnemyAI, Error, TEXT("Enemy %s failed to perform USoEATeleportToMarker: invalid target location (%d)!"), *Owner->GetName(), Index);

		Owner->OnTeleportBP(false);
	}

	LastFrameTime = NewFrameTime;
	return LastFrameTime < Duration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEATeleportToMarker::OnEnter(ASoEnemy* Owner)
{
	Owner->OnTeleportBP(true);
	LastFrameTime = 0.0f;
}
