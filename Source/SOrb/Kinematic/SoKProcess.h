// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoKProcess.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSoKinematicSystem, Log, All);

DECLARE_STATS_GROUP(TEXT("SoKinematic"), STATGROUP_SoKinematic, STATCAT_Advanced);

class USoKTask;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoKProcess
{
	GENERATED_USTRUCT_BODY()
public:
	FSoKProcess() {};

	// has to be called once at the beginning
	void Initialize(TArray<USceneComponent*>& TargetComponents, AActor* RematerializeLocation, int32 CrushDamage);

	// it's just a struct, not an AActor, functions must be called!
	// return value: rest time < - KINDA_SMALL_NUMBER -> still running
	float Tick(TArray<USceneComponent*>& TargetComponents, float DeltaSeconds);

	// come on
	void Start(bool bReverse, TArray<USceneComponent*>& TargetComponents);

	/** simply calls Tick 10 times with 10 seconds, but stops calling it if it is finished */
	void ForceFinish(TArray<USceneComponent*>& TargetComponents);

	/** Stops the process. This can totally leave the object in invalid state - who know what happens if the object isn't reset outside of this function!!! */
	void Terminate() { bRun = false; FirstTimeOffsetCounter = -1.0f; ActiveTaskID = 0; }

	bool IsActive() const { return bRun || FirstTimeOffsetCounter > 0.0f; }

	void SetDirection(bool bForward) { bCurrentlyReverse = !bForward; }
	void ReverseDirection() { bCurrentlyReverse = !bCurrentlyReverse; }

	/** Creates a new process with the params but without any task */
	FSoKProcess CopyParamsButNoTasks() const;

	/** resets the params based on the other process */
	void CopyParamsButNoTasks(const FSoKProcess& Ref);

	TArray<USoKTask*>& GetTasks() { return Tasks; }
private:

	USceneComponent* GetTarget(TArray<USceneComponent*>& TargetComponents) const;

protected:
	// indicates if the process is running or not
	// default true: process starts automatically, otherwise it will be set to true later (in Run())
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRun = false;

	// true -> after the last task tasks are executed again in reverse order
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPlayReverse = false;

	// true -> after the last task the first will begin again
	// can work with bPlayReverse, then the { Tasks, TasksBackwards } loops
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLoop = false;

	// if true, FirstTimeOffset is Executed at the beginning if bRun
	// if false, FirstTimeOffset is waited at the beginning if bRun
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWorkAhead = false;

	// if it should run by default it will simulate FirstTimeOffset time at the beginning so it's easier to setup cool offsets
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FirstTimeOffset = 0.f;

	// the index of the SceneComponent this process modifies in the SoKinematicActor::TargetComponents array
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
	int32 TargetIndex = 0;

	UPROPERTY(Instanced, EditAnywhere, NoClear, BlueprintReadWrite)
	TArray<USoKTask*> Tasks;

private:

	// if the tasks are executed in reverse order currently
	bool bCurrentlyReverse = false;

	// active task = Tasks[ActiveTaskID]
	int32 ActiveTaskID = 0;

	float FirstTimeOffsetCounter = -1.0f;
};
