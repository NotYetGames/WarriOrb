// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once
#include "UObject/Object.h"
#include "SplineLogic/SoSpline.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoKTask.generated.h"

DECLARE_DYNAMIC_DELEGATE(FSoKNotify);

class UFMODEvent;
class UFMODAudioComponent;

UENUM(BlueprintType)
enum class ESoTaskCollisionResponse : uint8
{
	// obstacles are simply ignored
	ETCR_Ignore			UMETA(DisplayName = "Ignore"),
	// OnHit event is fired, but after that the object simply goes on
	// WARNING: only the first Hit event can be fired in a frame
	// works perfectly with rotation as well
	ETCR_Notify			UMETA(DisplayName = "Notify"),
	// task stops and waits until the path is free
	// does not work with rotation
	ETCR_Wait			UMETA(DisplayName = "Wait"),
	// the task is skipped and the process goes on with the next one
	ETCR_Skip			UMETA(DisplayName = "Skip"),
	// the whole process (not just the task) is played reverse from the point when the collision occurred
	// does not work with rotation
	ETCR_TurnBack		UMETA(DisplayName = "Turn"),
	// pushes the obstacles if it can
	// notes:
	// use with caution: the object moved with this collision type should not collide with static objects
	// pushing does not care about the terrain surface, objects may get stuck (maybe later they will be moved along surface?!)
	// if the pushed objects are physic simulated things should work out nicely
	// if they are not, the directly pushed object can't push other objects, it will get stuck should another collision occur
	// ISplineWalker objects are pushed along the spline, which works correctly only if the movement direction is more or less the same as the spline direction in the given point
	// ISPlineWalkers should not be pushed down from a spline at not connected spline ends
	ETCR_Push			UMETA(DisplayName = "Push"),

	// like push but damages target on stuck even if he could escape next frame
	ETCR_PushAggressive	UMETA(DisplayName = "PushAggressive"),

	// behaves like ETCR_Push for ISoSplineWalker-s, ETCR_Wait otherwise
	// Ignores destructibles meshes and PhysicsBody collision object types, does not work with rotation
	ETCR_PushOrWait		UMETA(DisplayName = "PushAndIgnore")
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct FSoTaskExecutionResults
{
	float RestTime = 0.0f;
	bool bChangeDirection = false;

	FSoTaskExecutionResults() {};
	FSoTaskExecutionResults(float InRestTime) { RestTime = InRestTime; }
	FSoTaskExecutionResults(float InRestTime, bool bInChangeDirection) : RestTime(InRestTime), bChangeDirection(bInChangeDirection) {};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew, Abstract)
class SORB_API USoKTask : public UObject
{
	GENERATED_BODY()

public:
	USoKTask() {};

	// called only once in BeginPlay(), can be used e.g. to store the start world transform of the actor
	virtual void Initialize(USceneComponent* Target, AActor* InRematerializeLocation, int32 InCrushDamage);

	// resets state / store some init value if necessary - sad that unreal has a weird idea about a class being abstract
	virtual void Start(USceneComponent* Target, bool bForward) { check(false); }

	// if @return >= 0, the task is done
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) { check(false); return {}; };

protected:
	// some helper function, shared code between children

	bool ShouldUseSweep(ESoTaskCollisionResponse CollisionType) const;

	// rotates target object
	void AddDeltaRotation(const FVector& InDeltaRot, bool bLocal, ESoTaskCollisionResponse CollisionType, float DeltaSeconds);

	// translates TargetComponent with the given offset
	// return value: applied percentage
	float AddDeltaOffset(const FVector& InDeltaOffset, bool bLocal, ESoTaskCollisionResponse CollisionType, float DeltaSeconds);

	// Pushes target on spline, works both on ISplineWalkers and on normal objects
	// bTeleportAndSweepBack expects that the movement responsible for this push is already happened (used in rotation atm):
	//		1. it moves the object without a sweep, expecting DeltaOffset to be bigger than the distance we actually have to move
	//		2. it moves back to towards the initial location with sweep, expecting a blocking collision from the original object
	void OnPushObject(AActor* ActorToPush, const FVector& DeltaOffset, float DeltaSeconds, bool bTeleportAndSweepBack = false, bool bForceDamage = false) const;


	/**
	*  CurrentTime is increased by DeltaSeconds, then it is clamped between 0 and MaxCurrentTime
	*  Return value: fabs(UnClampedCurrent - CurrentTime)
	*/
	float UpdateCurrentTime(float MaxCurrentTime, float DeltaSeconds, float& CurrentTime) const;

protected:
	// currently cached target
	UPROPERTY()
	USceneComponent* TargetComponent;

	UPROPERTY()
	AActor* RematerializeLocation;

	int32 CrushDamage;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I should have called it USoKTSleep, now I could write "zzzz" as a comment...
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTWait :public USoKTask
{
	GENERATED_BODY()

public:

	USoKTWait() {};
	virtual void Start(USceneComponent* Target, bool bForward) { CurrentTime = 0.0f; }
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) override;

protected:

	UPROPERTY(EditAnywhere, Category = Params)
	float TimeToWait;

	float CurrentTime = 0.0f;

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTCallFunction :public USoKTask
{
	GENERATED_BODY()

public:

	USoKTCallFunction() {};
	virtual void Initialize(USceneComponent* Target, AActor* InRematerializeLocation, int32 InCrushDamage);
	virtual void Start(USceneComponent* Target, bool bForward);
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) { return { DeltaSeconds }; }

protected:
	void BindDelegate(AActor* Actor, FName FunctionName, FSoKNotify& FunctionDelegate);


	UPROPERTY(EditAnywhere, Category = Params)
	FName FunctionNameForward;

	UPROPERTY(EditAnywhere, Category = Params)
	FName FunctionNameBackwards;

	UPROPERTY()
	FSoKNotify ForwardFunction;

	UPROPERTY()
	FSoKNotify BackwardsFunction;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTActivateComponent :public USoKTask
{
	GENERATED_BODY()

public:

	USoKTActivateComponent() {};
	virtual void Start(USceneComponent* Target, bool bForward) override;
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) { return { DeltaSeconds}; }

protected:

	UPROPERTY(EditAnywhere, Category = Params)
	bool bReset = true;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTPlaySound :public USoKTask
{
	GENERATED_BODY()

public:

	USoKTPlaySound() {};
	virtual void Start(USceneComponent* Target, bool bForward) override;
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) { return { DeltaSeconds}; }

protected:

	UPROPERTY(EditAnywhere, Category = Params)
	UFMODEvent* SFX;

	UPROPERTY(EditAnywhere, Category = Params)
	bool bAttached = false;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/** Only played backwards */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTPlaySoundForReversePlay :public USoKTask
{
	GENERATED_BODY()

public:

	USoKTPlaySoundForReversePlay() {};
	virtual void Start(USceneComponent* Target, bool bForward) override;
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) { return{ DeltaSeconds }; }

protected:

	UPROPERTY(EditAnywhere, Category = Params)
	UFMODEvent* SFX;

	UPROPERTY(EditAnywhere, Category = Params)
	bool bAttached = false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTPlayLoopedSound :public USoKTask
{
	GENERATED_BODY()

public:

	USoKTPlayLoopedSound() {};
	virtual void Start(USceneComponent* Target, bool bForward) override;
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) { return { DeltaSeconds}; }

	void StartStopSFX(bool bStop);

protected:

	UPROPERTY(EditAnywhere, Category = Params)
	UFMODEvent* SFX;

	UPROPERTY()
	UFMODAudioComponent* FMODComponent = nullptr;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// modifies location or orientation
UCLASS(BlueprintType, EditInlineNew, Abstract)
class SORB_API USoKTTimeBasedTransform :public USoKTask
{
	GENERATED_BODY()

public:
	virtual void Start(USceneComponent* Target, bool bForward) override;
	virtual FSoTaskExecutionResults Execute(float DeltaSeconds, bool bForward) override;

protected:

	// all children has to override this function
	virtual FVector GetOffset(float StartPercent, float EndPercent) const;

	virtual bool IsRotation() const { return false; }
	virtual bool IsLocalAxis() const { return false; }

	virtual void OnMoved(const FVector& Offset, float AppliedPercent) {};

public:

	// determines the collision response of the task, use with caution
	UPROPERTY(EditAnywhere, Category = Params)
	ESoTaskCollisionResponse CollisionType;

	// the amount of time the task takes after Start() is called
	UPROPERTY(EditAnywhere, Category = Params, meta = (ClampMin = "0.01", ClampMax = "100000"))
	float TimeInSec = 0.01;

	float CurrentTime = 0.0f;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// translation / rotation with const velocity in given time
// Value(t) = StartingValue + Axis * t
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTTransformBasedOnOffset :public USoKTTimeBasedTransform
{
	GENERATED_BODY()

	virtual void Initialize(USceneComponent* Target, AActor* InRematerializeLocation, int32 InCrushDamage) override;
	virtual void Start(USceneComponent* Target, bool bForward) override;

protected:
	virtual FVector GetOffset(float StartPercent, float EndPercent) const override;

	virtual bool IsRotation() const override { return bRotation; }
	virtual bool IsLocalAxis() const override { return bLocalAxis; }

	// the distance the object will cover in the given time
	UPROPERTY(EditAnywhere, Category = Params)
	FVector Offset;

	// decides if the transform is translation or rotation
	UPROPERTY(EditAnywhere, Category = Params)
	bool bRotation;

	// axis can be global, or the local axis of the target component
	UPROPERTY(EditAnywhere, Category = Params)
	bool bLocalAxis = true;

	// if true the starting position of the target will be saved
	// the movement is performed to the location defined by starting pos + offset
	// the time in order to keep the original speed
	// WORKS only with local axis, but seriously why the hell do we even have the world one?
	UPROPERTY(EditAnywhere, Category = Params)
	bool bAlwaysRelativeToStartPosition;

	// time it would take if we would start in the StartPos, used if bAlwaysRelativeToStartPosition
	float FullTime;
	// world position of the target component when Initilaize() was called
	FVector SavedStartPos;
	// Offset, or the offset leading to the position defined by (StartPos + Offset) depending on bAlwaysRelativeToStartPosition
	FVector RealOffset;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// calculates new position/rotation based on offset from starting position/rotation in given time: Value(t) = StartingValue + OffsetCurve(t/TimeInSec) * Axis
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTTransformBasedOnOffsetCurve :public USoKTTimeBasedTransform
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", DisplayName = "Create TransformBasedOnOffsetCurve task", Keywords = "new create kinematic task"), Category = KinematicTask)
	static USoKTTransformBasedOnOffsetCurve* ConstructTransformBasedOnOffsetCurve(UObject* WorldContextObject,
																				  float InTimeInSec,
																				  UCurveFloat* InCurve,
																				  const FVector& InOffset,
																				  bool bInRotation,
																				  ESoTaskCollisionResponse InCollisionType);

protected:
	virtual FVector GetOffset(float StartPercent, float EndPercent) const override;

	virtual bool IsRotation() const override { return bRotation; }
	virtual bool IsLocalAxis() const override { return bLocalAxis; }

protected:

	// Value(t) = StartingValue(t) + OffsetCurve(t) * Axis
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	UCurveFloat* OffsetCurve;

	// the const part of the distance the object will cover in the given time, it is multiplied with the current Offset value each frame
	UPROPERTY(EditAnywhere, Category = Params)
	FVector Offset;

	// decides if the transform is translation or rotation
	UPROPERTY(EditAnywhere, Category = Params)
	bool bRotation;

	// axis can be global, or the local axis of the target component
	UPROPERTY(EditAnywhere, Category = Params)
	bool bLocalAxis;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// translation / rotation with const velocity in given time
// Value(t) = StartingValue + Axis * dt
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTTransformBasedOnVelocity :public USoKTTimeBasedTransform
{
	GENERATED_BODY()

protected:
	virtual FVector GetOffset(float StartPercent, float EndPercent) const override;

	virtual bool IsRotation() const override { return bRotation; }
	virtual bool IsLocalAxis() const override { return bLocalAxis; }

public:

	// the velocity the target is transformed with
	UPROPERTY(EditAnywhere, Category = Params)
	FVector Velocity;

	// decides if the transform is translation or rotation
	UPROPERTY(EditAnywhere, Category = Params)
	bool bRotation;

	// axis can be global, or the local axis of the target component
	UPROPERTY(EditAnywhere, Category = Params)
	bool bLocalAxis;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// target is moved along a spline
// start offset from spline is calculated in start() and is kept
// does not handle collisions too well atm
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoKTTranslateOnSpline :public USoKTTimeBasedTransform
{
	GENERATED_BODY()

public:

	USoKTTranslateOnSpline() {};

	// USoKTask - closest point on spline is calculated here
	virtual void Start(USceneComponent* Target, bool bForward) override;

	void Start(USceneComponent* Target, bool bForward, const FSoSplinePoint& SplinePoint);

protected:
	// USoKTTimeBasedTransform:

	virtual FVector GetOffset(float StartPercent, float EndPercent) const override;
	// spline location update
	virtual void OnMoved(const FVector& Offset, float AppliedPercent) override;

public:

	// the spline the target is moving on
	// spline switch is not supported at the moment
	// the exact spline location is calculated based on world position in Start()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	TAssetPtr<ASoSpline> SplinePtr;

	// if true Value is velocity, otherwise it is the offset the target will move in the given time
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	bool bVelocityBased;

	// offset to move if bVelcityBased is false, velocity otherwise, .X is along spline, .Y is vertical
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	FVector2D Value;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	bool bFreezeRotationRelativeToSplineDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	bool bForceDistanceFromSpline;


	UPROPERTY(BlueprintReadOnly, Category = RuntimeValues)
	FVector DistanceFromSplinePoint;

	UPROPERTY(BlueprintReadOnly, Category = RuntimeValues)
	FRotator RotationDelta;

	UPROPERTY(BlueprintReadWrite, Category = RuntimeValues)
	FSoSplinePoint SplineLocation;
};


// Task ideas:
// Task to signal triggerables?
// we will see
