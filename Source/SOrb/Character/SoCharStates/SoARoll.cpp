// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoARoll.h"

#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SoCharacter.h"
#include "Character/SoCharStates/SoAHitReact.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Basic/SoAudioManager.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"

#include "SoADefault.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoARoll::USoARoll() :
	USoActivity(EActivity::EA_Rolling)
{
	JumpType = ESoJumpType::EJT_Bounce;
	LookDirection = ESoLookDirection::ELD_Frozen;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// this is a useless comment, just like most of them
void USoARoll::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);
	UpdateRollAnimDynamicValue();

	if (Orb->SoMovement->IsMovingOnGround())
	{
		auto* EventVariant = Orb->GetVelocity().SizeSquared() > 100 ? Orb->SFXRollEnter : Orb->SFXRollEnterNoMovement;
		USoAudioManager::PlaySoundAtLocation(Orb, EventVariant, Orb->GetActorTransform());
	}

	bInverseRollAnim = false;
	if (OldActivity != nullptr && OldActivity->IsArmedState())
	{
		const FSoItem& Weapon = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
		if (USoWeaponTemplate* WeaponTemplate = Cast<USoWeaponTemplate>(Weapon.Template))
			bInverseRollAnim = WeaponTemplate->GetInverseRollAnimsIfArmed();
	}

	Orb->CurrentSmokeColor = Orb->SmokeColorRoll;
	Orb->CurrentGlowSphereColor = Orb->GlowSphereColorRoll;
	Orb->RefreshDefaultKeyMovementLights();

	// Apply Momentum for jump for a time interval
	const float TimeSinceJump = Orb->GetWorld()->GetTimeSeconds() - Orb->StoredTimeOnJump;
	if (FMath::Abs(Orb->StoredMovementInputDirOnJump) > KINDA_SMALL_NUMBER &&
		TimeSinceJump > 0.1f && TimeSinceJump < 0.5f)
	{
		MaxVelocityBoostCounter = TimeSinceJump - 0.1f;
		Orb->SoMovement->MaxWalkSpeed = 2 * MovementSpeed - Orb->SoADefault->GetMovementSpeed();
	}
	else
	{
		MaxVelocityBoostCounter = -1.0f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoARoll::OnExit(USoActivity* NewActivity)
{
	ClearMaxSpeedBoost();

	// update orientation based on input, for e.g. strike
	if (Orb->LastInputDirection.SizeSquared() > KINDA_SMALL_NUMBER)
	{
		Orb->SetActorRotation(Orb->LastInputDirection.Rotation());
		UpdateCamera(0.01f);
	}

	if (Orb->RollSFX->IsActive())
		Orb->RollSFX->Deactivate();

	if (Orb->SoMovement->IsMovingOnGround())
	{
		auto* EventVariant = Orb->GetVelocity().SizeSquared() > 100 ? Orb->SFXRollLeave : Orb->SFXRollLeaveNoMovement;
		USoAudioManager::PlaySoundAtLocation(Orb, EventVariant, Orb->GetActorTransform());
	}

 	USoActivity::OnExit(NewActivity);
	bInverseRollAnim = false;

	Orb->CurrentSmokeColor = Orb->SmokeColorDefault;
	Orb->CurrentGlowSphereColor = Orb->GlowSphereColorDefault;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoARoll::OnPostExit(USoActivity* NewActivity)
{
	Super::OnPostExit(NewActivity);
	Orb->RefreshDefaultKeyMovementLights();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
void USoARoll::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	if (MaxVelocityBoostCounter > 0.0f)
	{
		MaxVelocityBoostCounter -= DeltaSeconds;
		if (MaxVelocityBoostCounter < 0.0f)
		{
			ClearMaxSpeedBoost();
		}
	}

	UpdateRollAnimDynamicValue();

	if (Orb->SoMovement->IsMovingOnGround() && Orb->GetVelocity().SizeSquared() > 100 * 100)
	{
		if (!Orb->RollSFX->IsActive())
			Orb->RollSFX->Activate(true);
	}
	else
	{
		if (Orb->RollSFX->IsActive())
			Orb->RollSFX->Deactivate();
	}

	// most likely we couldn't leave roll mode when we released the button... let's try again
	// calling CanBeInterrupted can end up causing dmg to the character
	if (!Orb->bRollPressed && CanBeInterrupted())
		// || (Orb->bUmbrellaPressed && Orb->SoMovement->MovementMode == EMovementMode::MOVE_Falling))
		SwitchToRelevantState(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoARoll::JumpPressed()
{
	if (!Orb->SoMovement->TryLateWallJump())
		Super::JumpPressed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoARoll::ClearMaxSpeedBoost()
{
	MaxVelocityBoostCounter = -1.0f;
	Orb->SoMovement->MaxWalkSpeed = MovementSpeed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoARoll::ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const
{
	if (HitResult != nullptr && HitResult->GetActor() != nullptr && HitResult->GetActor()->ActorHasTag(NoBounceSurface))
		return false;

	OutStepUpHitVelocity = Orb->GetRollJumpHeight(0) / 2.f;
	OutBounceDamping = DefaultBounceDampening;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoARoll::OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal)
{
	ClearMaxSpeedBoost();

	if (fabs(NewStoredMoveValue) > KINDA_SMALL_NUMBER)
	{
		Orb->ForcedMovementCounter = bWallJump ? Orb->GetForcedToMoveTimeAfterWallJump() : Orb->GetForcedToMoveTimeAfterHit();
		Orb->ForcedMovementValue = NewStoredMoveValue;
		if (bWallJump)
		{
			LastWallJumpTime = Orb->GetWorld()->GetTimeSeconds();
			if (Orb->bFloatingActive)
			{
				Orb->bUmbrellaPressed = false;
				StopFloating();
			}
		}
	}
	Orb->SpawnBounceEffects(bWallJump, HitPoint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoARoll::CanBeInterrupted(EActivity DesiredState) const
{
	const float CurrentTime = Orb->GetWorld()->GetTimeSeconds();
	const float ThresholdSeconds = LastWallJumpTime + Orb->GetForcedToMoveTimeAfterWallJump();

	// allow this for item usage so wall jump + test stone works :thinking:
	if (ThresholdSeconds > CurrentTime && DesiredState != EActivity::EA_ItemUsage)
		return false;

	// if we are falling collision has to be decreased anyway
	if (Orb->SoMovement->MovementMode == EMovementMode::MOVE_Falling ||
		DesiredState == EActivity::EA_Teleport)
		return true;

	AActor* Base = APawn::GetMovementBaseActor(Orb);

	// //  SEEMS like we don't need this
	// if (Base != nullptr && Base->ActorHasTag(RollOnlySurface))
	// 	return false;

	return Orb->CanIncreaseCollision();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoARoll::OnPushed(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage)
{
	USoActivity::OnPushed(DeltaMovement, DeltaSeconds, bStuck, RematerializeLocation, DamageAmountIfStuck, bForceDamage);

	if (Orb->SoActivity == this)
	{
		const float OwnerZVelocity = Orb->GetVelocity().Z;
		const float BaseZVelocity = DeltaMovement.Z / DeltaSeconds;
		if (OwnerZVelocity < -5.0f && BaseZVelocity > 5.0f)
		{
			FHitResult HitResult;
			Orb->AddActorWorldOffset(FVector(0.0f, 0.0f, -5.0f), true, &HitResult);
			if (HitResult.bBlockingHit)
			{
				OnPreLandedBounce(HitResult);
			}
			else
			{
				// WTH?!
				Orb->SoMovement->SetMovementMode(EMovementMode::MOVE_Falling);
				Orb->SoMovement->Velocity.Z = BaseZVelocity - OwnerZVelocity * DefaultBounceDampening;
			}
			Orb->SetBase(nullptr);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoARoll::OnJumped()
{
	Orb->OnInteractionLabelChanged.Broadcast();
}
