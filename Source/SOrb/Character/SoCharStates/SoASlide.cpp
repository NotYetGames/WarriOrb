// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoASlide.h"

#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"

#include "FMODAudioComponent.h"

#include "SoADefault.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Basic/SoAudioManager.h"

const FName SoCharSFXSlideParamName = FName("slide_speed");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoASlide::USoASlide() :
	USoActivity(EActivity::EA_Sliding)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASlide::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);
	Orb->SoMovement->MaxWalkSpeed = Orb->bRollPressed ? CrouchSpeed : MovementSpeed;
	Orb->SlideSFX->SetParameter(SoCharSFXSlideParamName, Orb->bRollPressed ? 1.0f : 0.0f);

	if (bRollWasPressed != Orb->bRollPressed)
	{
		bRollWasPressed = Orb->bRollPressed;
		USoAudioManager::PlaySoundAtLocation(Orb, bRollWasPressed ? Orb->SFXSlideCrouchStart : Orb->SFXSlideCrouchEnd, Orb->GetActorTransform());
		Orb->RefreshDefaultKeyMovementLights();
	}

	if (bWasInAir != Orb->SoMovement->IsMovingOnGround())
	{
		bWasInAir = !bWasInAir;
		if (bWasInAir)
			Orb->SlideSFX->Activate();
		else
			Orb->SlideSFX->Deactivate();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASlide::HandleCollision()
{
	if (Orb->bRollPressed)
	{
		if (!Orb->IsCollisionDecreased())
			Orb->DecreaseCollisionSize();
	}
	else
	{
		if (Orb->IsCollisionDecreased())
			Orb->IncreaseCollisionSize();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASlide::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);
	HandleCollision();

	Orb->CapsuleBottomVFX->SetTemplate(Orb->VFXSlide);
	Orb->CapsuleBottomVFX->Activate();

	Orb->SlideSFX->Activate(true);
	Orb->SlideSFX->SetParameter(SoCharSFXSlideParamName, Orb->bRollPressed ? 1.0f : 0.0f);
	bRollWasPressed = Orb->bRollPressed;
	bWasInAir = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASlide::OnExit(USoActivity* NewActivity)
{
	Orb->SoMovement->SetMovementMode(EMovementMode::MOVE_Falling);
	USoActivity::OnExit(NewActivity);
	Orb->CapsuleBottomVFX->Deactivate();
	Orb->SlideSFX->Deactivate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASlide::OnBaseChanged(AActor* ActualMovementBase)
{
	if (ActualMovementBase != nullptr)
	{
		if (!(ActualMovementBase->ActorHasTag(SlipperyPosTag) || ActualMovementBase->ActorHasTag(SlipperyNegTag)))
			SwitchActivity(Orb->SoADefault);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASlide::JumpPressed()
{
	Orb->SoMovement->Velocity.Z += Orb->bRollPressed ? CrouchJumpVelocity : Orb->SoMovement->JumpZVelocity;
	SwitchActivity(Orb->SoADefault);

	Orb->ForcedMovementCounter = JumpControlBanTime;
	Orb->ForcedMovementValue = DirModifier;
	// USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXSlideJump, Orb->GetActorTransform());
	Orb->JumpSFX->SetEvent(Orb->SFXSlideJump);
	Orb->JumpSFX->Play();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASlide::Move(float Value)
{
	const FVector MovDir = Orb->SoMovement->GetDirection();
	Orb->AddMovementInput(MovDir, 1.0f * DirModifier);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoASlide::CanBeInterrupted(EActivity DesiredState) const
{
	return bAllowStrikeInSlide && DesiredState == EActivity::EA_Striking;
}
