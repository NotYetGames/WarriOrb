// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAWaitForActivitySwitch.h"

#include "Runtime/LevelSequence/Public/LevelSequenceActor.h"

#include "SoADefault.h"
#include "Animation/AnimSequenceBase.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerProgress.h"
#include "SoAInUI.h"
#include "SplineLogic/SoMarker.h"
#include "SoATeleport.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SplineLogic/SoPlayerSpline.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoASoAWaitForActivitySwitch::USoASoAWaitForActivitySwitch() :
	USoActivity(EActivity::EA_WaitForActivitySwitch)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASoAWaitForActivitySwitch::OnEnter(USoActivity* OldActivity)
{
	Super::OnEnter(OldActivity);

	AnimationToPlay = nullptr;
	LevelSequenceActor = nullptr;
	CameraActor = nullptr;
	if (bHideUI)
		Orb->ChangeUIVisibility.Broadcast(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASoAWaitForActivitySwitch::OnExit(USoActivity* NewActivity)
{
	Super::OnExit(NewActivity);

	Orb->bShowInteractionTip = true;
	Orb->OnInteractionLabelChanged.Broadcast();

	if (bTriggerAreaChangeAfterSequence)
	{
		Orb->OnAreaChanged.Broadcast(nullptr, Cast<ASoPlayerSpline>(Orb->SoMovement->GetSplineLocation().GetSpline()));
		bTriggerAreaChangeAfterSequence = false;
	}

	if (bHideUI)
		Orb->ChangeUIVisibility.Broadcast(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoASoAWaitForActivitySwitch::Enter(EActivity InActivityToWaitFor, bool bForce, bool bInArmed)
{
	bHideUI = false;
	bTriggerAreaChangeAfterSequence = false;
	ActivityToWaitFor = InActivityToWaitFor;
	if (bForce || Orb->SoActivity->CanBeInterrupted(EActivity::EA_WaitForActivitySwitch))
	{
		bArmed = bInArmed;

		Orb->SoActivity->SwitchActivity(this);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoASoAWaitForActivitySwitch::Enter(ALevelSequenceActor* SequenceActor,
										 AActor* InCameraActor,
										 UAnimSequenceBase* AnimToPlay,
										 ASoMarker* TelTargetAfterSequence,
										 bool bInTriggerAreaChangeAfterSequence,
										 bool bInHideUI)
{
	bHideUI = bInHideUI;
	ActivityToWaitFor = TelTargetAfterSequence == nullptr ? EActivity::EA_Max : EActivity::EA_Teleport;
	Orb->SoActivity->SwitchActivity(this);

	LevelSequenceActor = SequenceActor;
	TeleportAfterSequence = TelTargetAfterSequence;
	CameraActor = InCameraActor;
	Counter = 0.2f;
	Orb->bShowInteractionTip = false;
	Orb->OnInteractionLabelChanged.Broadcast();
	AnimationToPlay = AnimToPlay;
	bPingAnimation = !bPingAnimation;

	bAnimationFinished = false;
	bTriggerAreaChangeAfterSequence = bInTriggerAreaChangeAfterSequence;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoASoAWaitForActivitySwitch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Counter > 0.0f)
	{
		Counter -= DeltaSeconds;
		if (Counter <= 0.0f && CameraActor != nullptr)
			Orb->GetPlayerController()->SetViewTarget(CameraActor);
	}

	if (LevelSequenceActor != nullptr && !LevelSequenceActor->SequencePlayer->IsPlaying())
	{
		Orb->GetPlayerProgress()->StartGatheringPlayerData();
		LevelSequenceActor = nullptr;
		if (TeleportAfterSequence != nullptr)
		{
			Orb->GetPlayerController()->SetViewTarget(Orb);
			Orb->SoATeleport->SetSequence(nullptr, nullptr, AnimationToPlay);
			Orb->SoATeleport->SetupTeleport(TeleportAfterSequence->GetSplineLocation(), TeleportAfterSequence->GetActorLocation().Z, true, false, false, true);
			return;
		}

		Orb->GetPlayerController()->SetViewTargetWithBlend(Orb, 1.5f, EViewTargetBlendFunction::VTBlend_EaseInOut, 2);

		// APlayerCameraManager* CamManager = Orb->GetPlayerController()->PlayerCameraManager;
		// CamManager->SetManualCameraFade(0.0f, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), false);
		// CamManager->StopCameraFade();
		Orb->FadeFromBlackInstant.Broadcast();

		Orb->bShowInteractionTip = true;
		Orb->OnInteractionLabelChanged.Broadcast();

		if (AnimationToPlay == nullptr)
			SwitchActivity(Orb->SoADefault);
	}
	else
		if (LevelSequenceActor == nullptr && bAnimationFinished)
		{
			bAnimationFinished = false;
			SwitchActivity(Orb->SoADefault);
		}
}
