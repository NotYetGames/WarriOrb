// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

DECLARE_DELEGATE(FSoTimerDelegate);

struct FSoTimer
{
public:

	/** ID which can be used to remove this timer */
	int32 ID = -1;
	/** Time this timer has left before it is fired */
	float RemainingTime = 0.0f;
	/** Called when the RemainingTime is over */
	FSoTimerDelegate Delegate;

	FSoTimer(const FSoTimerDelegate& InDelegate) : Delegate(InDelegate) {};
};


/** 
 *  Use if the exact frame time matters when you create a new timer after an old timer is executed
 *  Firing order inside a single frame is not granted to be chronological
 *  If we run out of positive int32 IDs we simply restart the counting from 0
 */
class FSoTimerManager : public FNoncopyable
{

public:

	/** Ticks the timer manager, should only be called if the game isn't paused */
	void Tick(float DeltaSeconds);

	/** Sets timer, returns with timer ID */
	template< class UserClass >
	FORCEINLINE int32 SetTimer(UserClass* InObj, typename FSoTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr InTimerMethod, float Duration)
	{
		return InternalSetTimer(FSoTimer(FSoTimerDelegate::CreateUObject(InObj, InTimerMethod)), Duration, InObj);
	}

	/** Sets timer, returns with timer ID */
	template< class UserClass >
	FORCEINLINE int32 SetTimer(UserClass* InObj, typename FSoTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InTimerMethod, float Duration)
	{
		return InternalSetTimer(FSoTimer(FSoTimerDelegate::CreateUObject(InObj, InTimerMethod)), Duration, InObj);
	}

	/** Clears the timer with TimerID if there is any, returns with -1 */
	int32 ClearTimer(int32 TimerID);

	void SetExtraTime(UObject* Object, float Time);
	float GetExtraTime(UObject* Object) const;

private:

	int32 InternalSetTimer(FSoTimer const& InDelegate, float Duration, UObject* Object);

private:

	/** used to know what to subtract on newly added timers if it happens inside a frame while the timers are updated */
	float RestFrameTime = 0.0f;

	/** Active timers */
	TArray<FSoTimer> Timers;

	/** Timers cached here if they wanna be removed during timer manager Tick() */
	TArray<int32> TimersToRemoveCache;

	int32 NextTimerID = 0;

	bool bCurrentlyInTick = false;

	/** Time stored for some object to be used instantly */
	TMap<UObject*, float> InstantTimeMap;
};

