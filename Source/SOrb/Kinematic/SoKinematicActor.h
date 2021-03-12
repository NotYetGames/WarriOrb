// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Basic/SoEventHandler.h"
#include "SoKProcess.h"
#include "GameFramework/Actor.h"
#include "SplineLogic/SoSplineWalker.h"
#include "SoKinematicActor.generated.h"

class ASoMarker;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKKinematicDelegateIntFloat, int32, ProcessIndex, float, RestTime);

UCLASS()
class SORB_API ASoKinematicActor : public AActor, public ISoSplineWalker, public ISoEventHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ASoKinematicActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;


	// spline walker interface - used to provide rematerialize location
	FSoSplinePoint GetSplineLocationI_Implementation() const override;
	void SetSplineLocation_Implementation(const FSoSplinePoint& SplinePoint, bool bUpdateOrientation) {};
	void OnPushed_Implementation(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage) {};

	virtual void HandleWhitelistedSplinesEntered_Implementation() override;
	virtual void HandleWhitelistedSplinesLeft_Implementation() override;

	UFUNCTION(BlueprintImplementableEvent, Category = Kinematic)
	void PostBeginPlayBP();

	UFUNCTION(BlueprintCallable, Category = Kinematic)
	void StartProcess(int32 ProcessID, bool bReverse);

	UFUNCTION(BlueprintCallable, Category = Kinematic)
	void StartProcessAndTick(int32 ProcessID, bool bReverse, float DeltaTime);

	/** simply calls Tick 10 times with 10 seconds, but stops calling it if it is finished */
	UFUNCTION(BlueprintCallable, Category = Kinematic)
	void ForceFinishProcess(int32 ProcessID);

	/** Stops the process. This can totally leave the object in invalid state - who know what happens if the object isn't reset outside of this function!!! */
	UFUNCTION(BlueprintCallable, Category = Kinematic)
	void TerminateProcess(int32 ProcessID);

	UFUNCTION(BlueprintCallable, Category = Kinematic)
	void ReverseProcessDirection(int32 ProcessID);

	UFUNCTION(BlueprintCallable, Category = Kinematic)
	void SetProcessDirection(int32 ProcessID, bool bForward);

	UFUNCTION(BlueprintPure, Category = Process)
	bool IsProcessActive(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = Process)
	bool HasAnyActiveProcess() const;

	/**
	 * called after the process is terminated - if ever
	 * RestTime: the not used time from this tick
	*/
	UPROPERTY(BlueprintAssignable)
	FKKinematicDelegateIntFloat OnProcessFinished;

protected:

	UFUNCTION(BlueprintCallable, Category = "KinematicActor")
	void ResetKinematicActor();

	/** Called after reset */
	UFUNCTION(BlueprintImplementableEvent, Category = "KinematicActor")
	void OnResetKinematicActorBP();

	bool ShouldHandleWhitelistedSplines() const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KinematicActor")
	TArray<FSoKProcess> Processes;

	// Processes effects an USceneComponent based on a stored index
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KinematicActor")
	TArray<USceneComponent*> TargetComponents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KinematicActor")
	float ProcessTimeMultiplier = 1.0f;

	/** If set the character will suffer CrushDamageOnPushAndStuck and can rematerialize at this location instead of instant death */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KinematicActor")
	ASoMarker* RematerializePoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KinematicActor")
	int32 CrushDamageOnPushAndStuck = 2;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KinematicActor")
	bool bResetOnRematerialize = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KinematicActor")
	bool bResetOnReload = false;

	/** has to be used if the actor is moving all the time - otherwise it will stay in invalid state after level reactivate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "KinematicActor")
	bool bResetInBeginPlay = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "KinematicActor")
	bool bCanDisableTickWhileNoProcessIsActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KinematicActor")
	TArray<TAssetPtr<ASoSpline>> WhitelistedSplines;

	/** weak hardware instead of switch now, but the name is kept because it is already used :( */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KinematicActor")
	bool bUseWhitelistedSplinesOnlyOnSwitch = false;

	/** Generated from Processes, does not have the tasks, only the process params. Used when the actor must be reset */
	TArray<FSoKProcess> InitStateOfProcesses;

	FTransform InitialTransform;
};
