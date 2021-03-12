// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoTriggerChain.h"

#include "Engine/World.h"
#include "TimerManager.h"

#include "Online/Analytics/SoAnalyticsHelper.h"

#include "SaveFiles/SoWorldState.h"
#include "SplineLogic/SoSpline.h"
#include "SaveFiles/SoWorldStateBlueprint.h"
#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "DlgDialogueParticipant.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoTriggerChain::ASoTriggerChain()
{
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (Behavior == ESoTriggerChainBehavior::ETCB_RandomizedPattern)
		StepNum = FMath::Min(StepNum, Targets.Num());

	if (Behavior == ESoTriggerChainBehavior::ETCB_Pattern)
	{
		IndexTable.SetNum(StepNum);
		TimeTable.SetNum(StepNum);
	}
	else if (Behavior == ESoTriggerChainBehavior::ETCB_PatternNotHinted)
	{
		TimeTable.SetNum(0);
		IndexTable.SetNum(StepNum);
	}
	else
	{
		IndexTable.SetNum(0);
		TimeTable.SetNum(0);
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::BeginPlay()
{
	Super::BeginPlay();

	// fill the random pattern
	if (Behavior == ESoTriggerChainBehavior::ETCB_RandomizedPattern)
	{
		IndexTable.SetNum(StepNum);
		for (int32 i = 0; i < StepNum; ++i)
			IndexTable[i] = i;

		for (int32 i = 0; i < StepNum - 1; ++i)
		{
			const int32 Index = FMath::RandRange(i + 1, StepNum - 1);
			IndexTable.Swap(i, Index);
		}
	}

	bSequenceStarted = false;
	bIgnoreTriggers = false;

	USoEventHandlerHelper::SubscribeToPlayerRematerialize(this);
	USoEventHandlerHelper::SubscribeToSoPostLoad(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(ResetTimer);
	GetWorld()->GetTimerManager().ClearTimer(InputWaitTimer);

	USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(this);
	USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);

	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::Trigger_Implementation(const FSoTriggerData& TriggerData)
{
	// UE_LOG(LogTemp, Warning, TEXT("Trigger %d seconds %f"), TriggerData.SourceIdentifier, Seconds);

	GetWorld()->GetTimerManager().ClearTimer(InputWaitTimer);
	if (bIgnoreTriggers || StepCounter == StepNum)
		return;

	if (bSequenceStarted)
	{
		if (TriggerData.SourceIdentifier == ExpectedInput)
		{
			const int32 TriggerValue = ((Behavior != ESoTriggerChainBehavior::ETCB_Randomized) ? 1 : 0);
			ISoTriggerable::Execute_Trigger(Targets[ExpectedInput], { TriggerValue, {} });

			if (SFXOnCorrectInput.IsValidIndex(StepCounter))
				USoAudioManager::PlaySoundAtLocation(this, SFXOnCorrectInput[StepCounter], Targets[ExpectedInput]->GetActorTransform());

			StepCounter += 1;
			if (StepCounter == StepNum)
			{
				USoTriggerHelper::TriggerAllElement(TriggerOnDone);
				USoAudioManager::PlaySound2D(this, SFXOnDone);

				if (bRegisterAsMilestone)
					USoAnalyticsHelper::RecordGameplayMilestone(this, USoStaticHelper::GetNameFromActor(this, "TC_"), true);

				if (BoolToSetTrueOnCharacter != NAME_None)
					IDlgDialogueParticipant::Execute_ModifyBoolValue(USoStaticHelper::GetPlayerCharacterAsActor(this), BoolToSetTrueOnCharacter, true);

				if (bResetOnDone)
					ResetEverything();
				else
				{
					if (bSerializeState)
						USoWorldState::AddMyNameToSet(this);
				}
			}
			else
				UpdateExpectedInput();
		}
		else
			InputFailed();
	}
	else
	{
		bSequenceStarted = true;
		StepCounter = 0;
		UpdateExpectedInput();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::HandlePlayerRematerialize_Implementation()
{
	if (bResetOnPlayerRematerialize || StepCounter < StepNum)
		ResetEverything();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::HandleSoPostLoad_Implementation()
{
	if (bSerializeState && USoWorldState::IsMyNameInSet(this))
	{
		for (auto* Target : Targets)
			ISoTriggerable::Execute_Trigger(Target, { 1, {} });

		StepCounter = StepNum;
	}
	else
		ResetEverything();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::UpdateExpectedInput()
{
	switch (Behavior)
	{
		case ESoTriggerChainBehavior::ETCB_Randomized:
		if (Targets.Num() > 1)
		{
			const int32 OldExpectedInput = ExpectedInput;
			while (OldExpectedInput == ExpectedInput)
				ExpectedInput = FMath::RandRange(0, Targets.Num() - 1);
		}
		break;

		case ESoTriggerChainBehavior::ETCB_Pattern:
		case ESoTriggerChainBehavior::ETCB_PatternNotHinted:
		case ESoTriggerChainBehavior::ETCB_RandomizedPattern:
			ExpectedInput = IndexTable.IsValidIndex(StepCounter) ? IndexTable[StepCounter] : -1;
			break;

		default:
			break;
	}

	if (Behavior == ESoTriggerChainBehavior::ETCB_Randomized)
	{
		if (Targets.IsValidIndex(ExpectedInput))
			ISoTriggerable::Execute_Trigger(Targets[ExpectedInput], { 1,{} });

		GetWorld()->GetTimerManager().SetTimer(InputWaitTimer, this, &ASoTriggerChain::InputFailed, TimeTable.IsValidIndex(StepCounter) ? TimeTable[StepCounter] : DefaultWaitTime);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::ResetEverything()
{
	FSoTriggerData ResetData;
	ResetData.SourceIdentifier = 0;

	for (AActor* Target : Targets)
		if (Target != nullptr)
			ISoTriggerable::Execute_Trigger(Target, ResetData);

	bIgnoreTriggers = false;
	bSequenceStarted = (Behavior == ESoTriggerChainBehavior::ETCB_RandomizedPattern ||
						Behavior == ESoTriggerChainBehavior::ETCB_PatternNotHinted);
	StepCounter = 0;
	if (IndexTable.IsValidIndex(StepCounter))
		ExpectedInput = IndexTable[StepCounter];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoTriggerChain::InputFailed()
{
	FSoTriggerData InvalidInputData;
	InvalidInputData.SourceIdentifier = -1;
	for (AActor* Target : Targets)
		ISoTriggerable::Execute_Trigger(Target, InvalidInputData);

	GetWorld()->GetTimerManager().SetTimer(ResetTimer, this, &ASoTriggerChain::ResetEverything, 1.0f);
	bIgnoreTriggers = true;
}
