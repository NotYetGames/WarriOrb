// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoKTProcessControl.h"
#include "Kinematic/SoKinematicActor.h"
#include "SoKTask.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTProcessStart::Start(USceneComponent* Target, const bool bForward)
{
	Owner = Cast<ASoKinematicActor>(Target->GetOwner());
	bStarted = false;
	bFinished = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoTaskExecutionResults USoKTProcessStart::Execute(const float DeltaSeconds, const bool bForward)
{
	if (Owner == nullptr)
	{
		UE_LOG(LogSoKinematicSystem, Error, TEXT("USoKTProcessStart error: invalid owner!"));
		return { 0.0f };
	}

	if (!bStarted)
	{
		if (bTickOnStart)
			Owner->StartProcessAndTick(ProcessToStart, bStartReverse, DeltaSeconds);
		else
			Owner->StartProcess(ProcessToStart, bStartReverse);

		bStarted = true;

		if (!bSynchrone)
			return {DeltaSeconds};

		Owner->OnProcessFinished.AddUniqueDynamic(this, &USoKTProcessStart::OnProcessFinished);
	}

	if (bFinished)
		return {TimeAfterFinished};

	return { -KINDA_SMALL_NUMBER };
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTProcessStart::OnProcessFinished(int32 ProcessIndex, float RestTime)
{
	if (ProcessIndex == ProcessToStart)
	{
		bFinished = true;
		Owner->OnProcessFinished.RemoveDynamic(this, &USoKTProcessStart::OnProcessFinished);
		TimeAfterFinished = RestTime;
	}
}
