// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/CharacterMovementComponent.h"
#include "SplineLogic/SoSplinePoint.h"
#include "CharacterBase/SoIMortalTypes.h"

#include "SoCharacterMovementComponent.generated.h"

class USplineComponent;
class ASoSpline;
class ASoCharacterBase;

/** Custom movement modes for characters */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	/** None (movement is disabled). */
	ECM_Swing		UMETA(DisplayName = "Swing"),
	/** Movement defined via curves as offset based on time - check FSoControlledMovementDesc */
	// ECM_Controlled	UMETA(DisplayName = "Controlled"),

	ECM_MAX			UMETA(Hidden),
};


/**
 *
 */
UCLASS()
class SORB_API USoCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	USoCharacterMovementComponent();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Inherited functions
	// Called when the game starts
	virtual void InitializeComponent() override;

	virtual void SetDefaultMovementMode() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/** Update position based on Base movement */
	virtual void UpdateBasedMovement(float DeltaSeconds) override;

	// TEST TEST TEST
	virtual FVector CalcAnimRootMotionVelocity(const FVector& RootMotionDeltaMove, float DeltaSeconds, const FVector& CurrentVelocity) const override;

	virtual FVector ConstrainAnimRootMotionVelocity(const FVector& RootMotionVelocity, const FVector& CurrentVelocity) const override { return RootMotionVelocity; }

	virtual FVector GetImpartedMovementBaseVelocity() const override;

protected:
	// overloaded for UpdateBasedMovement() - we force the position to be on the spline
	virtual FVector ConstrainLocationToPlane(FVector Location) const override;

	// SPLINE MOVEMENT:
	virtual void PhysFalling(float deltaTime, int32 Iterations) override;

	virtual void PhysFlying(float deltaTime, int32 Iterations) override;

	virtual void PhysWalking(float deltaTime, int32 Iterations) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const override;

	virtual void MoveAlongFloor(const FVector& InVelocity, float DeltaSeconds, FStepDownResult* OutStepDownResult = NULL) override;


	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;
	bool ResolvePenetrationOnSpline(const FVector& ProposedAdjustment, const FHitResult& Hit, const FQuat& NewRotationQuat);

	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;


	/* Code copied from charactermovement/SlideAlongSurface in order to get the same results for SlideVector **/
	FVector ComputeFixedNormalForDelta(const FVector& Delta, const FVector& InNormal, const FHitResult& Hit);

	/** Custom version of SlideAlongSurface that calculates the percent of the movement in spline-space */
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact) override;

	virtual void OnCharacterStuckInGeometry(const FHitResult* Hit) override;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Spline Movement

public:
	FORCEINLINE FVector GetDirection() const { return SplineLocation.GetDirection(); }
	FORCEINLINE const FSoSplinePoint& GetSplineLocation() const { return SplineLocation; }
	FORCEINLINE FSoSplinePoint& GetSplineLocation() { return SplineLocation; }

	// doesn't modify the actor's location in the world, it just modify the stored spline location
	void SetSplineLocation(ASoSpline* NewSpline, float Distance = 0.f);
	// doesn't modify the actor's location in the world, it just modify the stored spline location
	UFUNCTION(BlueprintCallable)
	void SetSplineLocation(const FSoSplinePoint& InSplineLocation);

	/**
	* Adds a delta to the location of the component in spline space.
	* @param Offset				Change in location, absolute value
	* @param SweepHitResult		Hit result from any impact if sweep is true.
	* @param bSweep				Whether we sweep to the destination location, triggering overlaps along the way and stopping short of the target if blocked by something.
	*							Only the root component is swept and checked for blocking collision, child components move without sweeping. If collision is off, this has no effect.
	*/
	void AddOffsetOnSpline(float Offset, const FVector& Direction, bool bSweep = false, FHitResult* HitResult = nullptr);

	const USplineComponent* GetActiveSplineComponent() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	UFUNCTION(BlueprintCallable, Category = Movement)
	void SetRootMotionDesc(const FSoRootMotionDesc& Desc)
	{
		SoRootMotionDesc = Desc;
	}

	UFUNCTION(BlueprintCallable, Category = Movement)
	void ClearRootMotionDesc()
	{
		SoRootMotionDesc.bHorizontalRootMotionEnabled = false;
		SoRootMotionDesc.bVerticalRootMotionEnabled = false;
		SoRootMotionDesc.Multiplier = 1.0f;
	}

	void SetFallVelocityOverride(const bool bInFallVelocityOverride, const float InFallVelocityOverride = 0.0f);
	bool IsFallVelocityOverriden() const { return bFallVelocityOverride; }

	void SetSlowDownToMaxWalkSpeedInFall(bool bEnable) { bSlowDownToMaxWalkSpeedInFall = bEnable; }

	void Dash(const FVector& Dir, const float Distance);

	void StartSwinging(const FVector& InSwingCenter, const float InSwingInputForce);

	void StartSliding(const FVector& SurfaceNormal, float SurfaceSlidingModifier);

	void Push(const FVector2D& PushVelocity);

	// UFUNCTION(BlueprintCallable, Category = Movement)
	// void StartControlledMovementBasedOnOffset(const FSoControlledMovementDesc& MovementDesc);

	UFUNCTION(BlueprintCallable, Category = Movement)
	FVector GetSwingCenter() const { return SwingCenter; }

	UFUNCTION(BlueprintPure, Category = Movement)
	bool IsSwingForward() const { return bIsSwingForward; }

	void AddSwingDelta(float Delta) { SwingArmDelta += Delta; }

	float GetMaxSwingArm() const { return SwingArmMax; }
	bool IsPointInSwingReach(const FVector& InSwingCenter) const;

	void SetPushedAndStuck(AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage);
	void ForceUpdateFloor();

	void SetOppressSetDefaultMovementMode(bool bOppress) { bOppressSetDefaultMovementMode = bOppress; }


	bool TryLateWallJump();
	void ToggleAllowLateWallJump();
	void SetLateWallJumpInterval(float Time);
	float GetLateWallJumpInterval() const;


private:

	/** Handle falling movement. */
	void SplinePhysFalling(float deltaTime, int32 Iterations);

	void SplinePhysFlying(float deltaTime, int32 Iterations);

	// void PhysCustomForcedWalking(float deltaTime, int32 Iterations);
	void PhysCustomSwinging(float deltaTime, int32 Iterations);

	// void PhysControlledMovement(float deltaTime, int32 Iterations);

	// // ret val: amplitude
	// float UpdateSwingState();

	void StuckedAndShouldBeKilled();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Variables:

protected:

	// blender -> unreal fbx pipeline ftw, should be 1.0 for skeletons exported without this little issue
	UPROPERTY(EditAnywhere)
	int32 SkeletonIsExportedTerriblyModifier = 1.0f;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Spline Movement
protected:

	// used if Velocity.size2d is greater than the MaxWalkSpeed
	UPROPERTY(EditAnywhere)
	float BrakingDecelerationFallingOverMaxWalkSpeed = 2000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoSplinePoint SplineLocation;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	ASoCharacterBase* SoOwner;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoRootMotionDesc SoRootMotionDesc;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAllowRootMotionRotation = false;

	/** keeps the orientation at the spline, only used if it is not kept on movement direction */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bOrientRotationToSpline = false;

	bool bOppressSetDefaultMovementMode = false;
	bool bFly = false;

	// swing
	UPROPERTY(BlueprintReadWrite)
	FVector SwingCenter;

	FVector SwingInput;
	float SwingInputForce;

	bool bIsSwingForward;
	float SwingArmDelta = 0.0f;

	UPROPERTY(EditAnywhere)
	float SwingArmMin = 150.0f;

	UPROPERTY(EditAnywhere)
	float SwingArmMax = 400.0f;


	bool bFallVelocityOverride = false;
	float FallVelocityOverrideValue;

	bool bAlrearyRollHitThisFrame = false;


	/**
	 *  set to true if the owner was pushed into something last frame
	 *  if moving him out of the geometry fails he is simply executed
	 */
	bool bWasPushedAndStuckLastFrame = false;

	UPROPERTY()
	AActor* LastPushRematerializeLocation = nullptr;

	int32 LastPushCrushDamage = 0;


	/** Late WallJump */
	bool bAllowLateWallJump = true;
	float LateWallJumpInterval = 0.1f;

	float MissedWallJumpTime;
	FVector MissedWallJumpVelocity;
	float MissedRollStoredValue;
	FVector MissedImpactPointOffset;
	FVector MissedImpactNormal;

	// only one wall jump is allowed in a frame to avoid being stuck on negative surfaces
	bool bWallJumpedThisFrame = false;


	/**
	 * the character has to slow down in air if he jumped from a moving platform, otherwise he would be way too fast in some cases
	 * however in some cases this slow has to be avoided
	 * (e.g. when he jumps from sliding, because the MaxWalkSpeed is smaller in "walk" mode, than it was in "slide" mode)
	 * this bool is modified each time the actor's base changes (the platform he is standing on) based on the base mobility
	 * static - no slow down, moveable - slow down!
	 * (maybe it should be updated based on the velocity of the base?!)
	 * TEMP: disabled, LET'S SEE IF IT IS NEEDED OR NOT (2018.03.15).
	*/
	bool bSlowDownToMaxWalkSpeedInFall = false;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
};
