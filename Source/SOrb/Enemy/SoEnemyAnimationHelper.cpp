// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEnemyAnimationHelper.h"

#include "Animation/AnimSequenceBase.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UAnimSequenceBase* USoEnemyAnimationHelper::GetSingleAnimSequence(
	const TMap<FName, UAnimSequenceBase*>& AnimationMap,
	const FSoEnemyAnimData& AnimData,
	const bool bInterruptible,
	const bool bPing,
	bool& bLoop,
	float& PlayRate
)
{

	const FSoPingPongAnimName& PingPong = bInterruptible ? AnimData.InterruptibleSingle : AnimData.UninterruptibleSingle;

	UAnimSequenceBase* const* SequencePtr = AnimationMap.Find(PingPong.GetName(bPing));
	if (SequencePtr == nullptr || (*SequencePtr) == nullptr)
		return nullptr;

	bLoop = PingPong.IsLooped(bPing);
	PlayRate = bLoop ? 1.0f : (*SequencePtr)->SequenceLength / PingPong.GetDuration(bPing);

	return *SequencePtr;
}
