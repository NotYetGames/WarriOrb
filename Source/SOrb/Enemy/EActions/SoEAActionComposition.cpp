// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEAActionComposition.h"
#include "Enemy/SoEnemy.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEAActionChainElement::Evaluate(const ASoEnemy* Owner) const
{
	const float Value = USoEAction::Evaluate(Owner);
	if (Value > KINDA_SMALL_NUMBER)
		return Value * PrimaryAction->Evaluate(Owner);

	return Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAActionChainElement::Interrupt(ASoEnemy* Owner)
{
	if (bPrimaryPhase)
		PrimaryAction->Interrupt(Owner);
	else
	{
		ActiveSecondaryAction->Interrupt(Owner);
		ActiveSecondaryAction = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAActionChainElement::IsInterruptible() const
{
	if (bPrimaryPhase)
		return PrimaryAction->IsInterruptible();
	
	if (ActiveSecondaryAction != nullptr)
		return ActiveSecondaryAction->IsInterruptible();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAActionChainElement::OnEnter(ASoEnemy* Owner)
{
	bPrimaryPhase = true;
	PrimaryAction->Start(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAActionChainElement::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (bPrimaryPhase)
	{
		const bool bStillRuning = PrimaryAction->Tick(DeltaSeconds, Owner);
		if (!bStillRuning)
		{
			bPrimaryPhase = false;
			if (bPickFirstSecond)
				ActiveSecondaryAction = Owner->GetFirstSatisfiedAction(SecondActions);
			else
				ActiveSecondaryAction = Owner->ChooseActionFromList(SecondActions);

			if (ActiveSecondaryAction == nullptr)
				return false;

			ActiveSecondaryAction->Start(Owner);
		}
		return true;
	}
	
	check(ActiveSecondaryAction);
	return ActiveSecondaryAction->Tick(DeltaSeconds, Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEAActionSequence::Evaluate(const ASoEnemy* Owner) const
{
	if (ActionSequence.Num() == 0)
		return 0.0f;

	const float Value = USoEAction::Evaluate(Owner);
	if (Value > KINDA_SMALL_NUMBER)
		return Value * ActionSequence[0]->Evaluate(Owner);

	return Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAActionSequence::Interrupt(ASoEnemy* Owner)
{
	if (ActionSequence.IsValidIndex(ActiveSequenceID))
	{
		ActionSequence[ActiveSequenceID]->Interrupt(Owner);
		UE_LOG(LogSoEnemyAI, Display, TEXT("Action sequence interrupted!"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAActionSequence::OnEnter(ASoEnemy* Owner)
{
	ActiveSequenceID = -1;
	LoopCounter = 0;

	if (ActionSequence.Num() == 0 || ActionSequence[0]->Evaluate(Owner) < KINDA_SMALL_NUMBER)
		return;

	ActionSequence[0]->Start(Owner);
	ActiveSequenceID = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAActionSequence::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (ActiveSequenceID < 0)
		return false;

	const bool bStillRuning = ActionSequence[ActiveSequenceID]->Tick(DeltaSeconds, Owner);
	if (bStillRuning)
		return true;

	ActiveSequenceID += 1;

	if (ActiveSequenceID == ActionSequence.Num())
	{
		LoopCounter += 1;
		if (LoopCounter >= LoopCount || 
			(bCheckPreconditionEachLoop && (Evaluate(Owner) < KINDA_SMALL_NUMBER)))
		{
			ActiveSequenceID = -1;
			return false;
		}
		
		ActiveSequenceID = 0;
	}

	if (ActionSequence[ActiveSequenceID]->Evaluate(Owner) < KINDA_SMALL_NUMBER)
		return false;

	ActionSequence[ActiveSequenceID]->Start(Owner);
	return true;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEAActionList::Evaluate(const ASoEnemy* Owner) const
{
	const float BaseChance = Super::Evaluate(Owner);
	float Value = 0.0f;
	if (BaseChance > KINDA_SMALL_NUMBER)
	{
		for (const USoEAction* Action : Actions)
		{
			Value = FMath::Max(Value, Action->Evaluate(Owner));
		}
	}

	return Value * BaseChance;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAActionList::Interrupt(ASoEnemy* Owner)
{
	if (ActiveAction != nullptr)
		ActiveAction->Interrupt(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAActionList::OnEnter(ASoEnemy* Owner)
{
	ActiveAction = Owner->ChooseActionFromList(Actions);
	ActiveAction->Start(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAActionList::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (ActiveAction == nullptr)
		return false;

	const bool bStillRuning = ActiveAction->Tick(DeltaSeconds, Owner);
	if (bStillRuning)
		return true;

	ActiveAction = nullptr;
	return false;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEARandomer::Interrupt(ASoEnemy* Owner)
{
	if (ActiveAction != nullptr)
		ActiveAction->Interrupt(Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEARandomer::OnEnter(ASoEnemy* Owner)
{
	ActiveAction = nullptr;

	TArray<float> Multipliers;
	Multipliers.SetNum(Actions.Num());

	float Sum = 0.0f;
	for (int32 i = 0; i < Actions.Num(); ++i)
	{
		Multipliers[i] = Actions[i]->Evaluate(Owner);
		Sum += (Chances.IsValidIndex(i) ? Chances[i] : 1.0f) * Multipliers[i] * GetRepeatAvoidanceMultiplier(i);
	}
	const float RandValue = FMath::RandRange(0.0f, Sum);
	Sum = -KINDA_SMALL_NUMBER;
	for (int32 i = 0; i < Actions.Num(); ++i)
	{
		Sum += (Chances.IsValidIndex(i) ? Chances[i] : 1.0f) * Multipliers[i] * GetRepeatAvoidanceMultiplier(i);
		if (RandValue <= Sum)
		{
			ActiveAction = Actions[i];
			ActiveAction->Start(Owner);
			PrevPrevSelectedIndex = PrevSelectedIndex;
			PrevSelectedIndex = i;
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEARandomer::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	if (ActiveAction == nullptr)
		return false;

	const bool bStillRuning = ActiveAction->Tick(DeltaSeconds, Owner);
	if (bStillRuning)
		return true;

	ActiveAction = nullptr;
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEARandomer::GetRepeatAvoidanceMultiplier(int32 Index) const
{
	if (bLowerChanceToRepeat && PrevSelectedIndex == Index)
	{
		return (PrevPrevSelectedIndex == Index) ? 0.3f : 0.6f;
	}

	return 1.0f;
}