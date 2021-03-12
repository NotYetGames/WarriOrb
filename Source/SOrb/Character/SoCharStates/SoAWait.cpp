// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAWait.h"
#include "Character/SoCharacter.h"
#include "SoAInUI.h"
#include "SoAWeaponInArm.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoAWait::USoAWait() :
	USoActivity(EActivity::EA_Wait)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWait::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	Counter -= DeltaSeconds;
	if (Counter < 0.0f)
	{
		if (bArmed)
			SwitchActivity(Orb->SoAWeaponInArm);
		else
			SwitchToRelevantState(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWait::Enter(float Duration, bool bInArmed)
{
	Orb->SoActivity->SwitchActivity(this);
	Counter = Duration;
	bArmed = bInArmed;
}
