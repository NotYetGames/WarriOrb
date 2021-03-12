// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAction.h"

#include "Engine/World.h"

#include "Enemy/SoEnemy.h"
#include "Enemy/EPreconditions/SoEPrecondition.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAction::Start(ASoEnemy* Owner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("EAction - Start"), STAT_EAction_Start, STATGROUP_SoEnemy);

	LastUsageTime = Owner->GetWorld()->GetTimeSeconds();
	OnEnter(Owner);
	// UE_LOG(LogSoEnemyAI, Display, TEXT("Action entered: %s (%s)"), *GetName(), *Owner->GetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAction::Interrupt(ASoEnemy* Owner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("EAction - Interrupt"), STAT_EAction_Interrupt, STATGROUP_SoEnemy);

	OnLeave(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEAction::Evaluate(const ASoEnemy* Owner) const
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("EAction - Evaluate"), STAT_EAction_Evaluate, STATGROUP_SoEnemy);

	float GetMe = 1.0f;

	for (const auto* Prec : Preconditions)
	{
		if (Prec != nullptr)
		{
			GetMe *= Prec->Evaluate(Owner, this);
			// first prec can stop the others, this way we can save some energy
			// harder to calculate predictions should be placed at the end of the array
			if (GetMe < KINDA_SMALL_NUMBER)
				return 0.0f;
		}
	}

	return GetMe;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAction::Tick(float DeltaSeconds, ASoEnemy* Owner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("EAction - Tick"), STAT_EAction_Tick, STATGROUP_SoEnemy);

	if (bForceSplineDirection)
		Owner->SetActorRotation(ISoSplineWalker::Execute_GetSplineLocationI(Owner).GetDirectionFromVector(Owner->GetActorForwardVector()).Rotation());

	const bool bResult = OnTick(DeltaSeconds, Owner);
	if (!bResult ||
		(bOnlyWhileEvaluated && Evaluate(Owner) < KINDA_SMALL_NUMBER))
	{
		OnLeave(Owner);
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEAction::GetLastUsageTime() const
{
	return LastUsageTime;
}
