// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoKProcess.h"
#include "SoKHelper.generated.h"

UCLASS()
class SORB_API USokHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = Kinematic)
	static void StartStopLoopedSounds(UPARAM(Ref)TArray<FSoKProcess>& Processes, bool bStop);
};
