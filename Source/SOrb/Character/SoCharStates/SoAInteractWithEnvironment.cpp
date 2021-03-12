// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAInteractWithEnvironment.h"
#include "Animation/AnimSequenceBase.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "SoAInUI.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoAInteractWithEnvironment::USoAInteractWithEnvironment() :
	USoActivity(EActivity::EA_InteractWithEnvironment)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInteractWithEnvironment::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);

	if (Anim != nullptr)
		Counter = Anim->SequenceLength / Anim->RateScale;

	bCanBeInterrputed = false;
	Orb->SoMovement->SetRootMotionDesc({true, true, 1.0f});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInteractWithEnvironment::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
	// Anim = nullptr;
	Counter = -1.0f;
	Orb->SoMovement->ClearRootMotionDesc();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInteractWithEnvironment::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	Counter -= DeltaSeconds;

	if (Anim == nullptr || Counter < CounterTarget)
	{
		bCanBeInterrputed = true;
		if (UISource == nullptr || !Orb->SoAInUI->Enter(UISource, UIActivityClass))
			SwitchToRelevantState(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAInteractWithEnvironment::Enter(UAnimSequenceBase* AnimToPlay,
										const TSubclassOf<USoInGameUIActivity> InUIActivityClass,
										UObject* InUISource,
										float InCounterTarget,
										bool bInUseAnimBlendIn)
{
	UISource = InUISource;
	UIActivityClass = InUIActivityClass;
	CounterTarget = InCounterTarget;
	bUseAnimBlendIn = bInUseAnimBlendIn;

	if (AnimToPlay != nullptr && Orb != nullptr && Orb->SoActivity != nullptr && Orb->SoActivity->CanBeInterrupted(EActivity::EA_InteractWithEnvironment))
	{
		Anim = AnimToPlay;
		Orb->SoActivity->SwitchActivity(this);
		return true;
	}

	return false;
}
