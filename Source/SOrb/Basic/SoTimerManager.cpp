// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.


#include "SoTimerManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoTimerManager::Tick(float DeltaSeconds)
{
	// addition to the array is possible during iteration!

	bCurrentlyInTick = true;
	const int32 TimerCount = Timers.Num();
	for (int32 i = TimerCount - 1; i >= 0; --i)
	{
		Timers[i].RemainingTime -= DeltaSeconds;
		if (Timers[i].RemainingTime <= 0.0f)
		{
			RestFrameTime = fabs(Timers[i].RemainingTime);
			Timers[i].Delegate.Execute();
			Timers.RemoveAtSwap(i);
		}
	}
	RestFrameTime = 0.0f;
	bCurrentlyInTick = false;

	// process clear requests which were executed during the timer executions
	for (int32 TimerToRemove : TimersToRemoveCache)
		ClearTimer(TimerToRemove);
	TimersToRemoveCache.Empty();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoTimerManager::InternalSetTimer(FSoTimer const& InDelegate, float Duration, UObject* Object)
{
	if (InDelegate.RemainingTime < 0.0f)
		return -1;

	Duration -= RestFrameTime;

	if (float* ExtraTime = InstantTimeMap.Find(Object))
	{
		if (*ExtraTime <= Duration)
		{
			Duration -= *ExtraTime;
			InstantTimeMap.Remove(Object);
		}
		else
		{
			*ExtraTime -= Duration;
			InDelegate.Delegate.Execute();
			return -1;
		}
	}


	if (Duration < 0.0f)
	{
		RestFrameTime = fabs(Duration);
		InDelegate.Delegate.Execute();
		return -1;
	}
	
	Timers.Add(InDelegate);
	Timers.Last(0).RemainingTime = Duration;
	Timers.Last(0).ID = NextTimerID;
	NextTimerID = FMath::Max(NextTimerID + 1, 0);
	return Timers.Last(0).ID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoTimerManager::ClearTimer(int32 TimerID)
{
	if (bCurrentlyInTick)
		TimersToRemoveCache.Add(TimerID);
	else
	{
		for (int32 i = 0; i < Timers.Num(); ++i)
			if (Timers[i].ID == TimerID)
			{
				Timers.RemoveAtSwap(i);
				return -1;
			}
	}

	return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoTimerManager::SetExtraTime(UObject* Object, float Time)
{
	if (float* ExtraTime = InstantTimeMap.Find(Object))
		*ExtraTime = Time;
	else
		InstantTimeMap.Add(Object, Time);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float FSoTimerManager::GetExtraTime(UObject* Object) const
{
	if (const float* ExtraTime = InstantTimeMap.Find(Object))
		return *ExtraTime;

	return -1.0f;
}
