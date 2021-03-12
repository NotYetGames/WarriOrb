// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAParallel.h"
#include "Enemy/SoEnemy.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEAParallel::Evaluate(const ASoEnemy* Owner) const
{
	if (Action0 == nullptr || Action1 == nullptr)
	{
		UE_LOG(LogSoEnemyAI, Warning, TEXT("USoEAParallel::Evaluate failed: one of the actions isn't setup properly"));
		return 0.0f;
	}

	switch (Behaviour)
	{
		case ESoParallelBehaviour::EPB_TrySecondPauseFirst:
		case ESoParallelBehaviour::EPB_ExecuteFirstWhileNotSecond:
			return Super::Evaluate(Owner) * Action0->Evaluate(Owner);

		case ESoParallelBehaviour::EPB_ExecuteAtOnce:
		default:
			return Super::Evaluate(Owner) * Action0->Evaluate(Owner) * Action1->Evaluate(Owner);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAParallel::Interrupt(ASoEnemy* Owner)
{
	Action0->Interrupt(Owner);
	if (bSecondIsRunning)
		Action1->Interrupt(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAParallel::OnEnter(ASoEnemy* Owner)
{
	if (Action0)
		Action0->Start(Owner);

	if (Behaviour == ESoParallelBehaviour::EPB_ExecuteAtOnce)
	{
		Action1->Start(Owner);
		bSecondIsRunning = true;
	}
	else
	{
		Counter = SecondTryInterval;
		bSecondIsRunning = false;
	}

	bFirstFinished = false;
	bSecondFinished = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAParallel::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (Action0 == nullptr || Action1 == nullptr)
		return false;

	switch (Behaviour)
	{
		case ESoParallelBehaviour::EPB_ExecuteAtOnce:
		{
			if (bEndlessLoop)
			{
				if (!Action0->Tick(DeltaSeconds, Owner))
					Action0->Start(Owner);

				if (!Action1->Tick(DeltaSeconds, Owner))
					Action1->Start(Owner);

				return true;
			}
			
			if (!bFirstFinished)
				bFirstFinished = !Action0->Tick(DeltaSeconds, Owner);
			if (!bSecondFinished)
				bSecondFinished = !Action1->Tick(DeltaSeconds, Owner);
			
			return !bFirstFinished || !bSecondFinished;
		}

		case ESoParallelBehaviour::EPB_TrySecondPauseFirst:
		{
			if (bSecondIsRunning)
				bSecondIsRunning = Action1->Tick(DeltaSeconds, Owner);
			else
			{
				if (!Action0->Tick(DeltaSeconds, Owner))
					Action0->Start(Owner);

				Counter -= DeltaSeconds;
				if (Counter < 0.0f)
				{
					if (Action1->Evaluate(Owner) > KINDA_SMALL_NUMBER)
					{
						bSecondIsRunning = true;
						Action1->Start(Owner);
					}
					Counter = SecondTryInterval;
				}
			}
		}
		return true;

		case ESoParallelBehaviour::EPB_ExecuteFirstWhileNotSecond:
		{
			if (bSecondIsRunning)
			{
				if (!Action1->Tick(DeltaSeconds, Owner))
					return false;
			}
			else if (Action1->Evaluate(Owner) > KINDA_SMALL_NUMBER)
			{
				bSecondIsRunning = true;
				Action1->Start(Owner);
				Action0->Interrupt(Owner);
			}
			else if(!Action0->Tick(DeltaSeconds, Owner))
				Action0->Start(Owner);
		}
		return true;

		default:
			return false;
	}
}

