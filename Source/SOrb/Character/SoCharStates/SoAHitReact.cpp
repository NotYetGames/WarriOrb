// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAHitReact.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Particles/ParticleSystemComponent.h"

#include "SoADead.h"
#include "SoARoll.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"
#include "Character/SoSpringArmComponent.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Basic/SoAudioManager.h"
#include "Basic/SoGameMode.h"

const float HitReactVelocity = 600.f;
const float HitReactVelocityZ = 300.f;

DEFINE_LOG_CATEGORY_STATIC(LogSoAHitReact, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoAHitReact::USoAHitReact() :
	USoActivity(EActivity::EA_HitReact)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAHitReact::OnEnter(USoActivity* OldActivity)
{
	if (CanBeArmedState())
		bArmed = OldActivity->IsArmedState();

	USoActivity::OnEnter(OldActivity);

	bReturnToArmed = OldActivity->IsArmedState();
	CurrentTime = 0.0f;

	if (HitReactType == ESoHitReactType::EHR_FallToDeath)
		Orb->GetCapsuleComponent()->SetCollisionProfileName(DisabledCollisionProfileName);

	Orb->bFreezeOrientationUntilLand = true;

	// stop hit trails
	for (auto& Trail : Orb->WeaponTrailMeshFadeList)
	{
		static const FName SlideValueName = FName("Slide");
		Trail.Mesh->GetStaticMeshComponent()->SetScalarParameterValueOnMaterials(SlideValueName, 1.0f);

		Trail.Mesh->GetStaticMeshComponent()->SetVisibility(false);
		Trail.Mesh->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Orb->WeaponTrailMeshes.Add(Trail.Mesh);
	}
	Orb->WeaponTrailMeshFadeList.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAHitReact::OnExit(USoActivity* NewActivity)
{
	switch (HitReactType)
	{
		case ESoHitReactType::EHR_FallToDeath:
			Orb->GetCapsuleComponent()->SetCollisionProfileName(EnabledCollisionProfileName);
		case ESoHitReactType::EHR_BreakIntoPieces:
			Orb->SoMovement->StopMovementImmediately();
			break;

		default:
			break;
	}

	USoActivity::OnExit(NewActivity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAHitReact::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	// Orb->DynamicAnimValue = (bMoveToLookDir ? 2.0f : -2.0f) * (Orb->SoMovement->Velocity.Size2D() / Orb->SoMovement->MaxWalkSpeed);
	UpdateRollAnimDynamicValue();

	CurrentTime += DeltaSeconds;
	if (CurrentTime > Duration)
	{
		// switch to death, only legit if we fall to death
		if (!Orb->SoCharacterSheet->IsAlive())
		{
			if (HitReactType != ESoHitReactType::EHR_FallToDeath)
				UE_LOG(LogSoAHitReact, Error, TEXT("End of hitreact, the character is dead but he should not be?! (%d"), static_cast<int32>(HitReactType));

			USoActivity::OnDeath();
			Orb->SoADead->OverrideCameraData(SavedCamRotation, SavedCamWorldLocation);
			return;
		}

		// these hitreacts require respawn at splinepoint attached to the damage dealer
		if (HitReactType == ESoHitReactType::EHR_BreakIntoPieces || HitReactType == ESoHitReactType::EHR_FallToDeath)
		{
			if (StoredSplineLocation.IsValid())
			{
				if (Orb->IsSoulkeeperActive())
				{
					const FSoSplinePoint& OrbSplinePoint = Orb->SoMovement->GetSplineLocation();
					// use soulkeeper if that is closer and placed
					const FVector2D DistanceRematPoint = FVector2D(OrbSplinePoint - StoredSplineLocation,
																   Orb->GetActorLocation().Z - StoredSplineLocation.GetReferenceZ());
					const FVector2D DistanceSoulkeeper = FVector2D(OrbSplinePoint - Orb->ResLocation, Orb->GetActorLocation().Z - Orb->ResLocationZValue);
					if (DistanceSoulkeeper.SizeSquared() < DistanceRematPoint.SizeSquared())
					{
						StoredSplineLocation = Orb->ResLocation;
						StoredSplineLocation.SetReferenceActor(nullptr);
						StoredSplineLocation.SetReferenceZ(Orb->ResLocationZValue);
					}
				}

				Orb->OnPlayerRematerialized.Broadcast();
				Orb->SetPositionOnSplineSP(StoredSplineLocation, StoredSplineLocation.GetReferenceZ());
				if (HitReactType == ESoHitReactType::EHR_BreakIntoPieces)
				{
					Orb->OnRematerialized();
					SetEnabledState();
					Orb->MovementStopMultiplier = 0;
				}
				Orb->SoQuickTeleportPost->Activate(true);
			}
			else
			{
				UE_LOG(LogSoAHitReact, Error, TEXT("Character should be respawned but the respawn point isn't setup properly?!"));
				USoActivity::OnDeath();
				return;
			}

			SwitchToRelevantState(false);
		}

		SwitchToRelevantState(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAHitReact::OnDmgTaken(const FSoDmg& Damage, const FSoHitReactDesc& HitReactDesc)
{
	switch (HitReactType)
	{
		case ESoHitReactType::EHR_FallToDeath:
		case ESoHitReactType::EHR_BreakIntoPieces:
			return true;

		default:
			return USoActivity::OnDmgTaken(Damage, HitReactDesc);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAHitReact::DecreaseHealth(const FSoDmg& Damage)
{
	return Orb->SoCharacterSheet->IsAlive();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAHitReact::Activate(const FSoHitReactDesc& Desc, bool bAliveIfPossible)
{
	StoredSplineLocation.Invalidate();

	// if char is hitreacting only certain type can be overriden by some type
	// the reason for this is not to fall through respawn based damage boxes while being in other hitreact
	if (Orb->SoActivity == this)
	{
		if (HitReactType == ESoHitReactType::EHR_FallToDeath || HitReactType == ESoHitReactType::EHR_BreakIntoPieces)
			return true;

		if (Desc.HitReact != ESoHitReactType::EHR_FallToDeath && Desc.HitReact != ESoHitReactType::EHR_BreakIntoPieces)
			return true;
	}

	if (!bAliveIfPossible && Desc.HitReact != ESoHitReactType::EHR_FallToDeath)
	{
		USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFBreakIntoPieces, Orb->GetActorTransform());
		OnDeath();
		return false;
	}

	Orb->OnDamageTaken.Broadcast();

	bShouldPlayAnim = false;
	HitReactType = Desc.HitReact;
	bArmed = false;

	if (HitReactType != ESoHitReactType::EHR_FallToDeath && HitReactType != ESoHitReactType::EHR_BreakIntoPieces)
	{
		if (Orb->MaterialAnimations.Num() > 0)
			Orb->MaterialAnimations[0].Counter = 0.0f;
		Orb->OnDisplayVisualEffect(ESoVisualEffect::EVE_Damaged);
	}
	switch (Desc.HitReact)
	{
		case ESoHitReactType::EHR_JumpAwayLight:

			bForceStayInTime = true;
			Orb->gTimerManager->SetTimer(ForceAnimInTimer, this, &USoAHitReact::OnForceHitReactAnimTimeOver, JumpAwayTime, false);

		case ESoHitReactType::EHR_JumpAway:
		{
			Duration = (HitReactType == ESoHitReactType::EHR_JumpAway) ? JumpAwayTime : JumpAwayTimeLight;
			bShouldPlayAnim = true;
			int32 DirModifier = Desc.OutDir;
			if (Desc.OutDir == 0 && Desc.AssociatedActor != nullptr)
			{
				const FVector Dir = (Orb->GetActorLocation() - Desc.AssociatedActor->GetActorLocation()).GetSafeNormal();
				DirModifier = (FVector2D(Orb->SoMovement->GetSplineLocation().GetDirection()) | FVector2D(Dir)) > 0 ? 1 : -1;
			}

			const FVector VelocityDir = Orb->SoMovement->GetSplineLocation().GetDirection() * DirModifier;
			Orb->SoMovement->Velocity = VelocityDir * HitReactVelocity;
			Orb->SoMovement->Velocity.Z = HitReactVelocityZ;
			Orb->SoMovement->SetMovementMode(MOVE_Falling);
			Orb->SoActivity->SwitchActivity(Orb->SoAHitReact);

			bMoveToLookDir = FVector::DotProduct(VelocityDir, Orb->GetActorForwardVector()) > 0;
		}
		break;

		case ESoHitReactType::EHR_Pushed:
		{
			bShouldPlayAnim = true;
			Duration = Desc.PushDesc.StunDuration > 0.0f ? Desc.PushDesc.StunDuration : PushedStunTime;
			Orb->SoActivity->SwitchActivity(Orb->SoAHitReact);
			const FVector Dir = (Orb->GetActorLocation() - Desc.AssociatedActor->GetActorLocation()).GetSafeNormal();
			const int32 DirModifier = (FVector2D(Orb->SoMovement->GetSplineLocation().GetDirection()) | FVector2D(Dir)) > 0 ? 1 : -1;
			const float VerticalVelocity = Orb->SoMovement->IsMovingOnGround() ? FMath::Abs(Desc.PushDesc.VerticalVelocity) : Desc.PushDesc.VerticalVelocity;
			Orb->SoMovement->Push(FVector2D(Desc.PushDesc.HorizontalVelocity * DirModifier, VerticalVelocity));
		}
		break;

		case ESoHitReactType::EHR_BreakIntoPieces:
		{
			bool bHandled = false;
			if (Desc.AssociatedActor != nullptr && Desc.AssociatedActor->GetClass()->ImplementsInterface(USoSplineWalker::StaticClass()))
			{
				const FSoSplinePoint RespawnPoint = ISoSplineWalker::Execute_GetSplineLocationI(Desc.AssociatedActor);
				if (RespawnPoint.IsValid())
				{
					Orb->OnBreakIntoPieces();
					Orb->SoBreak->Activate(true);
					bHandled = true;
					SaveCamValues();
					SetDisabledState();
					Duration = BreakIntoPiecesTime;
					StoredSplineLocation = RespawnPoint;
					Orb->SoActivity->SwitchActivity(Orb->SoAHitReact);
				}
			}
			USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFBreakIntoPieces, Orb->GetActorTransform());
			if (!bHandled)
			{
				if (Desc.AssociatedActor != nullptr)
					UE_LOG(LogSoAHitReact, Error, TEXT("Failed to respawn - Associated Actor %s does not implement USoSplineWalker interface!!!"), *Desc.AssociatedActor->GetName())
				else
					UE_LOG(LogSoAHitReact, Error, TEXT("Failed to respawn - no respawn point defined"));
				OnDeath();
				return false;
			}
		}
		break;

		case ESoHitReactType::EHR_FallToDeath:
		{
			Duration = FallToDeathTime;
			SaveCamValues();
			if (Desc.AssociatedActor != nullptr && Desc.AssociatedActor->GetClass()->ImplementsInterface(USoSplineWalker::StaticClass()))
				StoredSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Desc.AssociatedActor);

			Orb->SoActivity->SwitchActivity(Orb->SoAHitReact);
			USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFFallToDeath, Orb->GetActorTransform());
		}
		break;

		default:
			break;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAHitReact::SaveCamValues()
{
	// update is needed before we can save the values, because if the character is rotating in air right now they may be wrong
	// would be nicer to get the values from the function or simply calculate them, but this works perfectly and less work so why not
	USoActivity::UpdateCamera(0.0f);
	SavedCamRotation = Orb->CameraBoom->GetComponentRotation();
	SavedCamWorldLocation = Orb->CameraBoom->GetComponentLocation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAHitReact::ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const
{
	if (HitResult != nullptr && HitResult->GetActor() != nullptr && HitResult->GetActor()->ActorHasTag(NoBounceSurface))
		return false;

	OutStepUpHitVelocity = Orb->GetRollJumpHeight(0) / 2.f;
	OutBounceDamping = 0.6;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAHitReact::OnPreLanded(const FHitResult& Hit)
{
	if (Orb->SoMovement->Velocity.Z < -KINDA_SMALL_NUMBER)
	{
		Orb->SoMovement->Velocity.Z *= -1;
		Orb->OnRollHitBP(Hit.ImpactPoint, Hit.ImpactNormal, false);
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAHitReact::OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal)
{
	Orb->SpawnBounceEffects(bWallJump, HitPoint);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAHitReact::UpdateCamera(float DeltaSeconds)
{
	if (HitReactType == ESoHitReactType::EHR_BreakIntoPieces ||
		HitReactType == ESoHitReactType::EHR_FallToDeath)
	{
		Orb->CameraBoom->SetWorldLocation(SavedCamWorldLocation);
		Orb->CameraBoom->SetWorldRotation(SavedCamRotation);
	}
	else
		USoActivity::UpdateCamera(DeltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAHitReact::CanBeArmedState() const
{
	return HitReactType != ESoHitReactType::EHR_BreakIntoPieces;
}
