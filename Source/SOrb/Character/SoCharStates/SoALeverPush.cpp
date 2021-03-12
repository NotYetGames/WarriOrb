// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoALeverPush.h"

#include "EngineMinimal.h" // ANY_PACKAGE enum search

#include "SoADefault.h"
#include "Interactables/SoInteractableActor.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "SoAWait.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoALeverPush::USoALeverPush() :
	USoActivity(EActivity::EA_LeverPush)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoALeverPush::OnEnter(USoActivity* OldActivity)
{
	bIsFinished = false;
	USoActivity::OnEnter(OldActivity);
	Orb->OnInteractionLabelChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoALeverPush::OnExit(USoActivity* NewActivity)
{
	bIsFinished = true;
	USoActivity::OnExit(NewActivity);
	Orb->OnInteractionLabelChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoALeverPush::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	const float PushedDelta = Orb->InputComponent->GetAxisValue("Move") * LeverData.PushDirection * (Orb->bRotatedCamera ? -1 : 1);
	const float Delta = LeverData.bForcePull ? 1.0f : PushedDelta;
	LeverData.ActualValue = FMath::Clamp(LeverData.ActualValue + Delta * LeverData.Speed * USoDateTimeHelper::DenormalizeTime(DeltaSeconds), 0.0f, 1.0f);
	Orb->DynamicAnimValue = LeverData.ActualValue;

	if (LeverData.bForcePull && (fabs(LeverData.TargetValue - LeverData.ActualValue) < KINDA_SMALL_NUMBER))
	{
		SwitchActivity(Orb->SoADefault);
		if (LeverData.WaitAfterLeverPull > 0.0f)
			Orb->SoAWait->Enter(LeverData.WaitAfterLeverPull);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoALeverPush::DecreaseHealth(const FSoDmg& Damage)
{
	SwitchActivity(Orb->SoADefault);
	return USoActivity::DecreaseHealth(Damage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoALeverPush::InteractKeyPressed(bool bPrimary)
{
	if (!LeverData.bForcePull)
		SwitchActivity(Orb->SoADefault);
}
