// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAExecuteFirst.h"
#include "Enemy/SoEnemy.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAExecuteFirst::Interrupt(ASoEnemy* Owner)
{
	if (ActiveAction != nullptr)
	{
		ActiveAction->Interrupt(Owner);
		ActiveAction = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAExecuteFirst::OnEnter(ASoEnemy* Owner)
{
	for (USoEAction* Action : Actions)
	{
		if (Action->Evaluate(Owner) > KINDA_SMALL_NUMBER)
		{
			ActiveAction = Action;
			ActiveAction->Start(Owner);
			return;
		}
	}

	ActiveAction = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAExecuteFirst::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (ActiveAction == nullptr)
		return false;

	const bool bStillRuning = ActiveAction->Tick(DeltaSeconds, Owner);
	if (bStillRuning)
		return true;

	ActiveAction = nullptr;
	return false;
}
