// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoKHelper.h"

#include "EngineUtils.h"

#include "Tasks/SoKTask.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USokHelper::StartStopLoopedSounds(TArray<struct FSoKProcess>& Processes, bool bStop)
{
	for (FSoKProcess& Process : Processes)
		for (int32 i = 0; i < Process.GetTasks().Num(); ++i)
			if (USoKTPlayLoopedSound* PLTask = Cast<USoKTPlayLoopedSound>(Process.GetTasks()[i]))
				PLTask->StartStopSFX(bStop);
}
