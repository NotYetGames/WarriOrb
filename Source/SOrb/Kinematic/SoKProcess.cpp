// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoKProcess.h"

#include "TimerManager.h"
#include "Engine/World.h"

#include "Tasks/SoKTask.h"

DEFINE_LOG_CATEGORY(LogSoKinematicSystem);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoKProcess::Initialize(TArray<USceneComponent*>& TargetComponents, AActor* RematerializeLocation, int32 CrushDamage)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKProcess - Initialize"), STAT_SoKProcessInitialize, STATGROUP_SoKinematic);

	ActiveTaskID = 0;

#if WITH_EDITOR
	for (int32 i = 0; i < Tasks.Num(); ++i)
	{
		if (Tasks[i] == nullptr)
		{
			if (TargetComponents.Num() > 0 && TargetComponents[0] != nullptr)
				UE_LOG(LogSoKinematicSystem,
					   Error,
					   TEXT("%s: Task is nullptr! It is removed now, but it should not happen outside of the editor!"), 
					   *TargetComponents[0]->GetOwner()->GetName())
			else
				UE_LOG(LogSoKinematicSystem, Error, TEXT("Task is nullptr! It is removed now, but it should not happen outside of the editor!"));
		}
	}
	Tasks.RemoveAll([](const USoKTask* Ptr) { return Ptr == nullptr; });
#endif

	if (!Tasks.IsValidIndex(ActiveTaskID))
	{
		bRun = false;
		UE_LOG(LogSoKinematicSystem, Error, TEXT("Process with invalid Task ID = %d!"), ActiveTaskID);
	}

	USceneComponent* Target = GetTarget(TargetComponents);
	if (Target != nullptr)
	{
		for (auto& Task : Tasks)
			Task->Initialize(Target, RematerializeLocation, CrushDamage);

		if (bRun)
		{

			if (FirstTimeOffset < KINDA_SMALL_NUMBER)
				Tasks[ActiveTaskID]->Start(Target, true);
			else
			{
				// handle initial delay
				if (bWorkAhead)
				{
					// hard working solution
					if (Tasks[ActiveTaskID])
						Tasks[ActiveTaskID]->Start(Target, true);
					Tick(TargetComponents, FirstTimeOffset);
				}
				else
				{
					bRun = false;
					FirstTimeOffsetCounter = FirstTimeOffset;
				}
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoKProcess::ForceFinish(TArray<USceneComponent*>& TargetComponents)
{
	FirstTimeOffsetCounter = -1.0f;
	for (int32 i = 0; i < 10 && bRun; ++i)
		Tick(TargetComponents, 10.0f);

	if (bRun)
		UE_LOG(LogSoKinematicSystem, Error, TEXT("ForceFinish failed - is this process infinite?"));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoKProcess::Tick(TArray<USceneComponent*>& TargetComponents, float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKProcess - Tick"), STAT_SoKProcessTick, STATGROUP_SoKinematic);

	if (FirstTimeOffsetCounter > 0.0f)
	{
		FirstTimeOffsetCounter -= DeltaSeconds;

		if (FirstTimeOffsetCounter <= 0.0f)
		{
			DeltaSeconds = FMath::Abs(FirstTimeOffsetCounter);

			bRun = true;
			if (Tasks.IsValidIndex(ActiveTaskID) && Tasks[ActiveTaskID])
				Tasks[ActiveTaskID]->Start(GetTarget(TargetComponents), true);
			FirstTimeOffsetCounter = -1.0f;
		}
	}


	if (bRun)
	{
		USceneComponent* Target = GetTarget(TargetComponents);
#if WITH_EDITOR
		if (Target == nullptr)
			return DeltaSeconds;

		const int32 MaxIterationNum = 20;
		int32 IterationNum = 0;
#endif
		FSoTaskExecutionResults TaskResults = Tasks[ActiveTaskID]->Execute(DeltaSeconds, !bCurrentlyReverse);
		if (TaskResults.bChangeDirection)
			bCurrentlyReverse = !bCurrentlyReverse;

		while (TaskResults.RestTime > KINDA_SMALL_NUMBER)
		{
#if WITH_EDITOR
			IterationNum += 1;
			if (IterationNum > MaxIterationNum)
			{
				UE_LOG(LogSoKinematicSystem, Error, TEXT("Too many iteration - process does not have progress at all. Hopeless tasks!"));
				return 0.0f;
			}
#endif
			// handle task switch
			ActiveTaskID += bCurrentlyReverse ? -1 : 1;
			if (bLoop)
			{
				if (ActiveTaskID < 0)
				{
					ActiveTaskID = 0;
					bCurrentlyReverse = false;
				}
				else if (ActiveTaskID >= Tasks.Num())
				{
					if (bPlayReverse)
					{
						bCurrentlyReverse = true;
						ActiveTaskID = Tasks.Num() - 1;
					}
					else
						ActiveTaskID = 0;
				}
				Tasks[ActiveTaskID]->Start(Target, !bCurrentlyReverse);
				TaskResults = Tasks[ActiveTaskID]->Execute(TaskResults.RestTime, !bCurrentlyReverse);
				if (TaskResults.bChangeDirection)
					bCurrentlyReverse = !bCurrentlyReverse;
			} // bLoop
			else
			{
				if (ActiveTaskID < 0 || ActiveTaskID >= Tasks.Num())
				{
					if (bPlayReverse && ActiveTaskID >= Tasks.Num())
					{
						bCurrentlyReverse = true;
						ActiveTaskID = Tasks.Num() - 1;
					}
					else
					{
						bRun = false;
						return TaskResults.RestTime;
					}
				}

				Tasks[ActiveTaskID]->Start(Target, !bCurrentlyReverse);
				TaskResults = Tasks[ActiveTaskID]->Execute(TaskResults.RestTime, !bCurrentlyReverse);
				if (TaskResults.bChangeDirection)
					bCurrentlyReverse = !bCurrentlyReverse;
			}
		} // RestTime > 0.0f

		return -1.0f;
	} // bRun

	return DeltaSeconds;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USceneComponent* FSoKProcess::GetTarget(TArray<USceneComponent*>& TargetComponents) const
{
#if WITH_EDITOR
	if (TargetIndex < 0 || TargetIndex >= TargetComponents.Num() || TargetComponents[TargetIndex] == nullptr)
	{
		UE_LOG(LogSoKinematicSystem, Error, TEXT("Process with invalid target ID!"));
		return nullptr;
	}
#endif
	return TargetComponents[TargetIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoKProcess::Start(bool bReverse, TArray<USceneComponent*>& TargetComponents)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SoKProcess - Start"), STAT_SoKProcessStart, STATGROUP_SoKinematic);

	auto* Target = GetTarget(TargetComponents);
#if WITH_EDITOR
	if (Target == nullptr)
		return;

	if (Tasks.Num() == 0)
	{
		// I would really like to sleep at this point
		UE_LOG(LogSoKinematicSystem, Error, TEXT("FSoKProcess::Start, but no task to run..."));
		return;
	}
#endif

	bRun = true;
	bCurrentlyReverse = bReverse;
	ActiveTaskID = bReverse ? Tasks.Num() - 1 : 0;

	Tasks[ActiveTaskID]->Start(Target, !bCurrentlyReverse);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoKProcess FSoKProcess::CopyParamsButNoTasks() const
{
	FSoKProcess NewProcess = *this;
	NewProcess.Tasks.Empty();
	return NewProcess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoKProcess::CopyParamsButNoTasks(const FSoKProcess& Ref)
{
	bRun				= Ref.bRun;
	bPlayReverse		= Ref.bPlayReverse;
	bLoop				= Ref.bLoop;
	bWorkAhead			= Ref.bWorkAhead;
	FirstTimeOffset		= Ref.FirstTimeOffset;
	TargetIndex			= Ref.TargetIndex;
	bCurrentlyReverse	= Ref.bCurrentlyReverse;
	ActiveTaskID		= Ref.ActiveTaskID;
}
