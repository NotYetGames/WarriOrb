// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEnemyDataTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "SoEnemyAnimationHelper.generated.h"

class UAnimSequenceBase;

UCLASS()
class SORB_API USoEnemyAnimationHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = EnemyAnimHelper, meta = (BlueprintThreadSafe))
	static UAnimSequenceBase* GetSingleAnimSequence(
		const TMap<FName, UAnimSequenceBase*>& AnimationMap,
		const FSoEnemyAnimData& AnimData,
		const bool bInterruptible,
		const bool bPing,
		bool& bLoop,
		float& PlayRate
	);
};
