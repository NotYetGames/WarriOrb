// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once
#include "Kinematic/SoKinematicActor.h"
#include "SoKTask.h"
#include "SoKTProcessControl.generated.h"



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Used to start other processes
 *  Should be *after* the started process in the process array (with > index)
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTProcessStart :public USoKTask
{
	GENERATED_BODY()

public:

	USoKTProcessStart() {};
	virtual void Start(USceneComponent* Target, const bool bForward) override;
	virtual FSoTaskExecutionResults Execute(const float DeltaSeconds, const bool bForward) override;

	UFUNCTION()
	virtual void OnProcessFinished(int32 ProcessIndex, float RestTime);

protected:

	/**
	 *  Synchrone: task is ready when the started process is over | otherwise: task is finished immediately, time is not taken
	 */
	UPROPERTY(EditAnywhere, Category = Params)
	bool bSynchrone = true;

	UPROPERTY(EditAnywhere, Category = Params)
	int32 ProcessToStart;

	UPROPERTY(EditAnywhere, Category = Params)
	bool bStartReverse;

	UPROPERTY(EditAnywhere, Category = Params)
	bool bTickOnStart = true;

	bool bStarted = false;
	ASoKinematicActor* Owner = nullptr;

	bool bFinished;
	float TimeAfterFinished;
};
