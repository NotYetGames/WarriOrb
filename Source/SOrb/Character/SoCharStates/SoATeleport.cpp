// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoATeleport.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/LevelSequence/Public/LevelSequenceActor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Particles/ParticleSystemComponent.h"

#include "SoADefault.h"
#include "SoAWaitForActivitySwitch.h"
#include "Character/SoCharacter.h"
#include "Character/SoSpringArmComponent.h"
#include "Character/SoCharStates/SoADead.h"
#include "Character/SoCharStates/SoAHitReact.h"
#include "Character/SoCharStates/SoAWeaponInArm.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoAudioManager.h"
#include "Basic/SoGameMode.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Levels/SoLevelHelper.h"
#include "Levels/SoLevelManager.h"
#include "Basic/SoGameInstance.h"
#include "Enemy/SoEnemy.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoATeleport, All, All);


// used for music and screen fade
const float FadeOutTimeDuringTeleport = 0.5f;
const float FadeInTimeDuringTeleport = 0.5f;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoATeleport::USoATeleport() :
	USoActivity(EActivity::EA_Teleport)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::OnEnter(USoActivity* OldActivity)
{
	// before onenter because it is used for collision
	bArmed = OldActivity->IsArmedState();

	USoActivity::OnEnter(OldActivity);
	bRevive = (OldActivity == Orb->SoADead);

	if (TeleportState != ESoATeleportState::EATS_QuickTeleportPre)
	{
		if (bHide)
			Orb->HideCharacterBP(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
	Orb->SetCameraLagEnabled(true);

	if (TeleportState == ESoATeleportState::EATS_QuickTeleportPost ||
		TeleportState == ESoATeleportState::EATS_QuickTeleportPre)
	{
		TeleportState = ESoATeleportState::EATS_Done;
		Orb->ShowCharacterBP(true);
		USoAudioManager::Get(Orb).SetAmbientFreeze(false);

		return;
	}

	if (TeleportState != ESoATeleportState::EATS_Done)
	{
		if (!Orb->bCameraFadeOutBlocked)
		{
			// APlayerCameraManager* CamManager = Orb->GetPlayerController()->PlayerCameraManager;
			// CamManager->StartCameraFade(1.0f, 0.0f, FadeInTimeDuringTeleport, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false, true);
			Orb->FadeFromBlack.Broadcast();
		}

		if (TeleportState != ESoATeleportState::EATS_Faded && bRevive)
			SetEnabledState();
	}

	LevelSequenceActor = nullptr;
	CameraActor = nullptr;
	Animation = nullptr;
	TeleportTargetAfterSequence = nullptr;

	// cause of different lights in different levels

	// TEMP FIX:
#if WARRIORB_WITH_EDITOR
	UGameplayStatics::GetPlayerController(Orb, 0)->ConsoleCommand("EnableAllScreenMessages");
#endif

	RestoreTextureMemoryPool();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoATeleport::SetupTeleport(const FSoSplinePoint& SplineTarget,
								 float ZTarget,
								 bool bForceFade,
								 bool bLookBackwards,
								 bool bHideDuringTeleport,
								 bool bCamAlreadyOutFaded)
{
	if (Orb != nullptr && Orb->SoActivity != nullptr && Orb->SoActivity->CanBeInterrupted(EActivity::EA_Teleport))
	{
		const bool bAllLevelLoaded = USoLevelHelper::AreLevelsAtSplineLocationLoaded(SplineTarget);

		StoredOldSpline = Cast<ASoPlayerSpline>(Orb->SoMovement->GetSplineLocation().GetSpline());
		StoredNewSpline = Cast<ASoPlayerSpline>(SplineTarget.GetSpline());

		if (StoredNewSpline == nullptr)
		{
			UE_LOG(LogSoATeleport, Error, TEXT("Failed to teleport: invalid spline target location!"));
			return true;
		}

		bLevelSwitchHappened = true;
		if (StoredOldSpline != nullptr)
		{
			const auto& OldClaims = StoredOldSpline->GetLevelClaims();
			const auto& NewClaims = StoredNewSpline->GetLevelClaims();
			bLevelSwitchHappened = (OldClaims.Num() > 0 && NewClaims.Num() > 0 && OldClaims[0].LevelName != NewClaims[0].LevelName);
		}

		if (bForceFade || !bAllLevelLoaded || bLevelSwitchHappened)
		{
			// fade
			StoredSplineTarget = SplineTarget;
			StoredZTarget = ZTarget;
			bStoredLookBackwards = bLookBackwards;

			bMusicSwitchInProgress = StoredOldSpline == nullptr || StoredOldSpline->GetMusic() != StoredNewSpline->GetMusic();
			if (bMusicSwitchInProgress)
				USoAudioManager::Get(Orb).FadeOutActiveMusic(/*FadeOutTimeDuringTeleport*/);

			APlayerCameraManager* CamManager = Orb->GetPlayerController()->PlayerCameraManager;
			const auto& GameInstance = USoGameInstance::Get(Orb);
			if (bCamAlreadyOutFaded || GameInstance.IsLoadingEpisode() || GameInstance.IsLoadingChapter())
			{
				StartWaitForLevel();
				// CamManager->StartCameraFade(1.0f, 1.0f, 0.01f, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false, true);
				// CamManager->SetManualCameraFade(1.0f, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false);
				Orb->FadeToBlackInstant.Broadcast();
			}
			else
			{
				TeleportState = ESoATeleportState::EATS_FadeOut;
				if (!Orb->bCameraFadeOutBlocked)
				{
					// CamManager->StartCameraFade(0.0f, 1.0f, FadeOutTimeDuringTeleport, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false, true);
					Orb->FadeToBlack.Broadcast();
				}
				Counter = FadeOutTimeDuringTeleport;
				Orb->OnLevelSwitchStart.Broadcast();
			}

			bHide = bHideDuringTeleport;
			Orb->SoActivity->SwitchActivity(this);
			return false;
		}

		TeleportState = ESoATeleportState::EATS_QuickTeleportPre;
		if (Orb->SoActivity != Orb->SoADead)
			Orb->SoQuickTeleportPre->Activate(true);

		Orb->SoActivity->SwitchActivity(this);

		StoredSplineTarget = SplineTarget;
		StoredZTarget = ZTarget;
		bStoredLookBackwards = bLookBackwards;
		Counter = Orb->QuickTeleportPreTime;

		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoATeleport::SetupQuickTeleport(const FSoSplinePoint& SplineTarget,
									  float ZTarget,
									  bool bLookBackwards)
{
	if (Orb != nullptr && SplineTarget.IsValid(false))
	{
		bool bAllLevelLoaded = false;
		const bool bAllLevelVisible = FSoLevelManager::Get().ClaimSplineLocation(SplineTarget, bAllLevelLoaded);

		StoredOldSpline = Cast<ASoPlayerSpline>(Orb->SoMovement->GetSplineLocation().GetSpline());
		StoredNewSpline = Cast<ASoPlayerSpline>(SplineTarget.GetSpline());
		bool bLevelSwitch = true;
		if (StoredOldSpline != nullptr)
		{
			const auto& OldClaims = StoredOldSpline->GetLevelClaims();
			const auto& NewClaims = StoredNewSpline->GetLevelClaims();
			bLevelSwitch = (OldClaims.Num() > 0 && NewClaims.Num() > 0 && OldClaims[0].LevelName != NewClaims[0].LevelName);
		}

		if (!bAllLevelVisible || bLevelSwitch)
			return false;

		const FVector OldLocation = Orb->GetActorLocation();
		USoAudioManager::Get(Orb).SetAmbientFreeze(true);
		// instant
		Orb->SetPositionOnSplineSP(SplineTarget, ZTarget, true, false, bLookBackwards);
		USoAudioManager::Get(Orb).SetAmbientFreeze(false);

		Orb->OnPostQuickTravelBP(OldLocation);

		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoATeleport::CanSwapPlayerWithSplineWalker(float MaxDistance)
{
	if (Orb == nullptr || !ISoMortal::Execute_IsAlive(Orb))
		return false;
	//DrawDebugLine(Orb->GetWorld(),
	//			  Orb->GetActorLocation(),
	//			  Orb->GetActorLocation() + GetCharRelevantDirection() * 700,
	//			  FColor(255, 0, 0),
	//			  false, -1, 0, 12.333);

	return USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, GetCharRelevantDirection(), true) != nullptr ||
		   USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, -GetCharRelevantDirection(), true) != nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoEnemy* USoATeleport::SwapPlayerWithSplineWalker(float MaxDistance, UParticleSystem* SwapVFX)
{
	if (Orb == nullptr || !ISoMortal::Execute_IsAlive(Orb))
		return nullptr;

	ASoEnemy* Target = USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, GetCharRelevantDirection(), true);
	if (Target == nullptr)
		Target = USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, -GetCharRelevantDirection(), true);

	if (Target != nullptr)
	{
		FVector OldCharLocation = Orb->GetActorLocation();
		if (SwapVFX != nullptr)
		{
			UGameplayStatics::SpawnEmitterAtLocation(Orb,
													 SwapVFX,
													 OldCharLocation - FVector(0.0f, 0.0f, Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
													 FRotator::ZeroRotator,
													 FVector(1.f),
													 true,
													 EPSCPoolMethod::AutoRelease);
			UGameplayStatics::SpawnEmitterAtLocation(Orb, SwapVFX, Target->GetFloorLocation(), FRotator::ZeroRotator, FVector(1.f), true, EPSCPoolMethod::AutoRelease);
		}

		const FSoSplinePoint OldCharSplineLocation = Orb->GetSoMovement()->GetSplineLocation();
		Orb->SoActivity->SwitchActivity(Orb->SoADefault);
		const FVector OldVelocity = Orb->SoMovement->Velocity;
		SetDisabledState();


		const FSoSplinePoint NewSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Target);

		const FVector OldEnemyLocation = Target->GetActorLocation();
		Orb->SetActorLocation(OldEnemyLocation);
		ISoSplineWalker::Execute_SetSplineLocation(Orb, NewSplineLocation, true);

		if (Target->ShouldKeepZLocationOnSwap())
			OldCharLocation.Z = OldEnemyLocation.Z;

		Target->SetActorLocation(OldCharLocation);
		ISoSplineWalker::Execute_SetSplineLocation(Target, OldCharSplineLocation, true);
		Target->Stun(0.3f, ESoStatusEffect::ESE_Stun);

		SetEnabledState();
		// fix enable/disable state "sideeffects"
		Orb->SoMovement->Velocity = OldVelocity;
		Orb->MovementStopMultiplier = 1;

		return Target;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoEnemy* USoATeleport::TeleportBehindSplineWalker(float MaxDistance, UParticleSystem* VFX, float Distance)
{
	if (Orb == nullptr || !ISoMortal::Execute_IsAlive(Orb))
		return nullptr;

	ASoEnemy* Target = USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, GetCharRelevantDirection(), true);
	if (Target == nullptr)
		Target = USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, -GetCharRelevantDirection(), true);

	if (Target != nullptr)
	{
		auto SpawnVfx = [VFX, this]()
		{
			if (VFX != nullptr)
		{
			UGameplayStatics::SpawnEmitterAtLocation(Orb,
													 VFX,
													 Orb->GetActorLocation() - FVector(0.0f, 0.0f, Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
													 FRotator::ZeroRotator,
													 FVector(1.f),
													 true,
													 EPSCPoolMethod::AutoRelease);
		}
		};

		SpawnVfx();

		Orb->SoActivity->SwitchActivity(Orb->SoADefault);
		// const FVector OldVelocity = Orb->SoMovement->Velocity;
		SetDisabledState();

		const FVector TargetLoc = Target->GetActorLocation();

		const FVector NewDirectionVector = (Orb->GetActorLocation() - TargetLoc).GetSafeNormal();
		FSoSplinePoint NewSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Target);
		NewSplineLocation += NewSplineLocation.GetDirectionModifierFromVector(-NewDirectionVector) * (Target->GetCapsuleComponent()->GetScaledCapsuleRadius() + Distance);

		const FVector NewCharLoc = FVector(NewSplineLocation, TargetLoc.Z);
		Orb->SetActorLocation(NewCharLoc);
		ISoSplineWalker::Execute_SetSplineLocation(Orb, NewSplineLocation, true);

		Orb->SetActorRotation(NewSplineLocation.GetDirectionFromVector(TargetLoc - NewCharLoc).Rotation());

		SetEnabledState();
		// fix enable/disable state "sideeffects"
		// Orb->SoMovement->Velocity = OldVelocity;
		Orb->MovementStopMultiplier = 1;

		SpawnVfx();

		return Target;
	}

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoATeleport::GetCharRelevantDirection() const
{
	FVector DirVector = Orb->LastVelocityDirection;
	if (DirVector.SizeSquared2D() < KINDA_SMALL_NUMBER)
		DirVector = Orb->GetActorForwardVector();

	return DirVector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoATeleport::CanTeleportBehindClosestEnemy(float MaxDistance)
{
	if (Orb == nullptr || !ISoMortal::Execute_IsAlive(Orb))
		return false;

	return USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, Orb->GetActorForwardVector(), false) != nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::TeleportBehindClosestEnemy(float MaxDistance)
{
	if (Orb == nullptr || !ISoMortal::Execute_IsAlive(Orb))
		return;

	if (ASoEnemy* Target = USoStaticHelper::GetClosestEnemyInDirection(Orb, Orb->GetActorLocation(), MaxDistance, Orb->GetActorForwardVector(), false))
	{
		Orb->SoActivity->SwitchActivity(Orb->SoADefault);
		SetDisabledState();

		const FVector OldCharLocation = Orb->GetActorLocation();
		const FVector TargetLocation = Target->GetActorLocation();
		const FVector NewDirectionVector = (OldCharLocation - TargetLocation).GetSafeNormal();

		FSoSplinePoint NewSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Target);
		NewSplineLocation += NewSplineLocation.GetDirectionModifierFromVector(-NewDirectionVector) * (Target->GetCapsuleComponent()->GetScaledCapsuleRadius() + 20.0f);
		Orb->SetPositionOnSplineSP(NewSplineLocation,
								   Target->GetActorLocation().Z,
								   true,
								   false,
								   NewSplineLocation.GetDirectionModifierFromVector(NewDirectionVector) < 0);
		SetEnabledState();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::Tick(float DeltaSeconds)
{
	switch (TeleportState)
	{
		case ESoATeleportState::EATS_FadeOut:
			Counter -= DeltaSeconds;
			if (Counter < KINDA_SMALL_NUMBER)
			{
				StartWaitForLevel();
				if (bArmed)
				{
					bArmed = false;
					Orb->OnArmedStateLeft();
				}
				if (bResetCameraViewAfterPortal)
				{
					bResetCameraViewAfterPortal = true;
					USoStaticHelper::GetPlayerController(Orb)->SetViewTarget(Orb);
				}
			}
			else
				break;

		case ESoATeleportState::EATS_WaitForLevelToBeVisible:
			UpdateWaitForLevel();
			break;

		case ESoATeleportState::EATS_Faded:
			// wait 4 frame for camera to update properly
			FrameCounter -= 1;
			if (FrameCounter < 0 && !USoLevelHelper::IsAnyLevelVisible(Orb, LevelsToHide))
			{
				if (LevelSequenceActor == nullptr)
					LevelSequenceActor = LevelSequenceActorPtr.Get();
				LevelSequenceActorPtr = nullptr;

				Counter = FadeInTimeDuringTeleport;
				if (LevelSequenceActor != nullptr || Animation != nullptr)
				{
					TeleportState = ESoATeleportState::EATS_Done;
					if (LevelSequenceActor != nullptr)
					{
						LevelSequenceActor->SequencePlayer->JumpToSeconds(0.0f);
						LevelSequenceActor->SequencePlayer->Play();
						Orb->FadeFromBlackInstant.Broadcast();
					}
					else
					{
						// APlayerCameraManager* CamManager = Orb->GetPlayerController()->PlayerCameraManager;
						// CamManager->StartCameraFade(1.0f, 0.0f, FadeInTimeDuringTeleport, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false, true);
						Orb->FadeFromBlack.Broadcast();
					}
					Orb->SoASoAWaitForActivitySwitch->Enter(LevelSequenceActor, CameraActor, Animation, TeleportTargetAfterSequence, true);

					if (CharacterHideTimeDuringSequence > 0.0f)
					{
						Orb->HideCharacterBP(true);
						GetWorld()->GetTimerManager().SetTimer(Orb->ShowCharacterTimer, Orb, &ASoCharacter::ShowCharacter, CharacterHideTimeDuringSequence);

						CharacterHideTimeDuringSequence = -1.0f;
					}

					LevelSequenceActor = nullptr;
					CameraActor = nullptr;
					Animation = nullptr;
					TeleportTargetAfterSequence = nullptr;

					if (bLevelSwitchHappened)
						ASoGameMode::Get(Orb).ResetGroupData();
				}
				else
				{
					TeleportState = ESoATeleportState::EATS_FadeIn;
					if (!Orb->bCameraFadeOutBlocked)
					{
						// APlayerCameraManager* CamManager = Orb->GetPlayerController()->PlayerCameraManager;
						// CamManager->StartCameraFade(1.0f, 0.0f, FadeInTimeDuringTeleport, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false, true);
						Orb->FadeFromBlack.Broadcast();

						if (ParticleAfterPortal != nullptr)
						{
							UGameplayStatics::SpawnEmitterAtLocation(Orb->GetWorld(), ParticleAfterPortal, Orb->GetActorLocation(), FRotator::ZeroRotator, true);
							ParticleAfterPortal = nullptr;
						}
					}
				}
			}
			break;

		case ESoATeleportState::EATS_FadeIn:
			Counter -= DeltaSeconds;

			// if done: wait for one frame before exit for camera to be updated properly
			if (Counter < KINDA_SMALL_NUMBER)
				TeleportState = ESoATeleportState::EATS_Done;
			break;

		case ESoATeleportState::EATS_Done:
			SwitchToRelevantState(false);
			if (bLevelSwitchHappened)
			{
				ASoGameMode::Get(Orb).ResetGroupData();
				Orb->OnAreaChanged.Broadcast(StoredOldSpline, StoredNewSpline);
			}
			break;

		case ESoATeleportState::EATS_QuickTeleportPre:
			Counter -= DeltaSeconds;
			if (Counter < 0.0f)
			{
				USoAudioManager::Get(Orb).SetAmbientFreeze(true);

				if (bRevive)
				{
					Orb->SetPositionOnSplineSP(StoredSplineTarget, StoredZTarget, false, false, bStoredLookBackwards);
					SetEnabledState();
				}
				Orb->SetPositionOnSplineSP(StoredSplineTarget, StoredZTarget, true, false, bStoredLookBackwards);

				if (bRevive)
				{
					(Orb->bLastRespawnWasSK ? Orb->SoResVFX : Orb->SoResCPVFX)->Activate(true);
					(Orb->bLastRespawnWasSK ? Orb->SoResSFX : Orb->SoResCPSFX)->Activate(true);
				}
				else
					Orb->SoQuickTeleportPost->Activate(true);

				if (Orb->SoActivity == this)
				{
					Counter = Orb->QuickTeleportPostTime + Counter;
					TeleportState = ESoATeleportState::EATS_QuickTeleportPost;
					Orb->HideCharacterBP(true);
				}
			}
			break;

		case ESoATeleportState::EATS_QuickTeleportPost:
			Counter -= DeltaSeconds;
			if (Counter < 0.0f)
			{
				Orb->ShowCharacterBP(true);
				USoAudioManager::Get(Orb).SetAmbientFreeze(false);

				if (LastActivity == Orb->SoADead ||
					(LastActivity == Orb->SoASoAWaitForActivitySwitch && LastActivity->CanBeInterrupted(EActivity::EA_Teleport)) ||
					LastActivity == Orb->SoAWeaponInArm)
					SwitchActivity(Orb->SoADefault);
				else
					SwitchToRelevantState(true);
			}
			break;

		default:
			break;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::UpdateCamera(float DeltaSeconds)
{
	if (TeleportState == ESoATeleportState::EATS_QuickTeleportPre && bRevive)
	{
		Orb->SoADead->UpdateCamera(DeltaSeconds);
	}
	else if (TeleportState == ESoATeleportState::EATS_FadeOut ||
		TeleportState == ESoATeleportState::EATS_QuickTeleportPre ||
		TeleportState == ESoATeleportState::EATS_QuickTeleportPost)
	{
		Super::UpdateCamera(DeltaSeconds);
	}
	else
	{
		Orb->LastLandZ = Orb->GetActorLocation().Z - Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Super::UpdateCamera(100.0f);
		Orb->CameraBoom->ForceUpdate();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoATeleport::ShouldDecreaseCollision() const
{
	return bArmed || USoActivity::ShouldDecreaseCollision();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::StartWaitForLevel()
{
	TeleportState = ESoATeleportState::EATS_WaitForLevelToBeVisible;
	bool bAllLoaded = false;
	FSoLevelManager::Get().ClaimSplineLocation(StoredSplineTarget, bAllLoaded);
	Orb->OnWaitForLevelShowStart.Broadcast(!bAllLoaded);
	if (!bAllLoaded)
		ReduceTextureMemoryPool();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::UpdateWaitForLevel()
{
	bool bAllLoaded = false;
	if (!FSoLevelManager::Get().ClaimSplineLocation(StoredSplineTarget, bAllLoaded))
		return;

	Orb->OnWaitForLevelShowEnd.Broadcast();
	RestoreTextureMemoryPool();

	TeleportState = ESoATeleportState::EATS_Faded;
	FrameCounter = 4;

	if (bHide)
		Orb->ShowCharacterBP(true);

	if (bMusicSwitchInProgress)
	{
		USoAudioManager::Get(Orb).SetMusic(StoredNewSpline->GetMusic(), true);
	}

	TArray<FSoLevelClaim> OldClaims;
	if (StoredOldSpline != nullptr)
		OldClaims = StoredOldSpline->GetLevelClaims();
	const auto& NewClaims = StoredNewSpline->GetLevelClaims();

	if (!bLevelSwitchHappened)
		USoAudioManager::Get(Orb).SetAmbientFreeze(true);

	Orb->SetPositionOnSplineSP(StoredSplineTarget, StoredZTarget, true, false, bStoredLookBackwards);

	if (!bLevelSwitchHappened)
		USoAudioManager::Get(Orb).SetAmbientFreeze(false);

	Orb->SetCameraLagEnabled(false);

	Orb->bUseSavedCameraBoomZ = false;
	Orb->bSavedCameraBoomZUpdateBlock = false;
	Orb->SavedCameraBoomZCounter = 0.0f;
	Orb->CameraUnfreezeCounter = 0.0f;
	Orb->CameraXDeltaCurrent = 0.0f;
	Orb->CameraXDeltaCounter = 0.0f;

	UpdateCamera(1.0f);

	if (bRevive)
	{
		SetEnabledState();
		// again cause of collision and stuff
		Orb->SetPositionOnSplineSP(StoredSplineTarget, StoredZTarget, true, false, bStoredLookBackwards);

		(Orb->bLastRespawnWasSK ? Orb->SoResVFX : Orb->SoResCPVFX)->Activate(true);
		(Orb->bLastRespawnWasSK ? Orb->SoResSFX : Orb->SoResCPSFX)->Activate(true);
	}


	// forced hide levels, cause they might overlap :( :( :(
	LevelsToHide.Empty();
	for (auto& Claim : OldClaims)
		LevelsToHide.Add(Claim.LevelName);

	for (auto& Claim : NewClaims)
		LevelsToHide.Remove(Claim.LevelName);

	for (auto& LevelName : LevelsToHide)
		FSoLevelManager::Get().HideActiveLevel(Orb, LevelName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::ReduceTextureMemoryPool()
{
#if PLATFORM_SWITCH
	if (!bTexturePoolReduced)
	{
		UGameplayStatics::GetPlayerController(Orb, 0)->ConsoleCommand("r.Streaming.PoolSize 600", true);
		bTexturePoolReduced = true;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoATeleport::RestoreTextureMemoryPool()
{
#if PLATFORM_SWITCH
	if (bTexturePoolReduced)
	{
		UGameplayStatics::GetPlayerController(Orb, 0)->ConsoleCommand("r.Streaming.PoolSize 1200", true);
		bTexturePoolReduced = false;
	}
#endif
}
