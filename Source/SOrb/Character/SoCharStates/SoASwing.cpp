// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoASwing.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "SoADefault.h"

#include "Character/SoCharacter.h"
#include "Objects/SoSwingCenter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Basic/SoAudioManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoASwing::USoASwing() :
	USoActivity(EActivity::EA_Swinging)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASwing::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);

	check(Orb->SwingCenters.Num() > 0);

	// try to grab closest pole

	const FVector OwnerLocation = Orb->GetActorLocation();
	int32 Index = 0;
	const FVector SwingCenterLoc0 = Orb->SwingCenters[0]->GetActorLocation();
	float Distance = (SwingCenterLoc0 - OwnerLocation).SizeSquared();

	bool bInvalid = SwingCenterLoc0.Z < OwnerLocation.Z;
	for (int32 i = 1; i < Orb->SwingCenters.Num(); ++i)
	{
		const FVector SwingCenterLoc = Orb->SwingCenters[i]->GetActorLocation();
		const float Dist = (SwingCenterLoc - OwnerLocation).SizeSquared();
		if ((Dist < Distance || bInvalid) && SwingCenterLoc.Z > OwnerLocation.Z)
		{
			bInvalid = false;
			Distance = Dist;
			Index = i;
		}
	}

	Orb->SoMovement->StartSwinging(Orb->SwingCenters[Index]->GetSwingCenter(), SwingInputForce);

	bLeftHanded = !bLeftHanded;
	bForward = -Orb->SoMovement->GetSplineLocation().GetDirectionModifierFromVector(Orb->GetActorForwardVector()) > 0;

	//if (bForward)
	//	UE_LOG(LogTemp, Warning, TEXT("Forward"))
	//else
	//	UE_LOG(LogTemp, Warning, TEXT("Backwards"));

	Orb->CurrentSmokeColor = Orb->SmokeColorSwing;
	Orb->CurrentGlowSphereColor = Orb->GlowSphereColorSwing;

	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXSwing, Orb->GetActorTransform());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASwing::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
	Orb->SoMovement->SetMovementMode(EMovementMode::MOVE_Falling);

	Orb->CurrentSmokeColor = Orb->SmokeColorDefault;
	Orb->CurrentGlowSphereColor = Orb->GlowSphereColorDefault;

	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXSwingEnd, Orb->GetActorTransform());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
void USoASwing::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);
	Orb->SoMovement->AddSwingDelta(-Orb->GetMovementYAxisValue() * 100.0f * DeltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void USoASwing::Move(float Value)
//{
//	// const FVector MovDir = Orb->SoMovement->GetDirection();
//	// Orb->SoMovement->SetSwingInput(MovDir * Value);
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASwing::JumpPressed()
{
	Orb->SoMovement->Velocity.Z = SwingLeaverJumpVelocity;
	SwitchActivity(Orb->SoADefault);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASwing::OnTeleportRequest()
{
	SwitchActivity(Orb->SoADefault);
}