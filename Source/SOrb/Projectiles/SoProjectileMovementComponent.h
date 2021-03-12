// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/ProjectileMovementComponent.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoProjectileMovementComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoProjectileMovementNotify);

/**
 *
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SORB_API USoProjectileMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	//Begin UActorComponent Interface
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//End UActorComponent Interface

	void SetSplineLocation(const FSoSplinePoint& InSplinePoint) { SplineLocation.CopySplineLocation(InSplinePoint); }
	const FSoSplinePoint& GetSplineLocation() const { return SplineLocation; }

	void SetOffsetFromSplineLocation(const FVector& Offset) { OffsetFromSplineLocation = Offset; }

	void SetExtraAcceleration(const FVector& Value) { ExtraAcceleration = Value; }

protected:

	virtual FVector ComputeMoveDelta(const FVector& InVelocity, float DeltaTime) const override;
	virtual bool MoveUpdatedComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = NULL, ETeleportType Teleport = ETeleportType::None) override;
	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;
	virtual FVector ComputeBounceResult(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta) override;

	virtual FVector ComputeAcceleration(const FVector& InVelocity, float DeltaTime) const;


	UPROPERTY(BlueprintReadWrite, Category = Spline)
	FSoSplinePoint SplineLocation;

	UPROPERTY(BlueprintReadWrite, Category = Spline)
	FVector OffsetFromSplineLocation = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(BlueprintReadWrite, Category = Spline)
	float InterpSpeed = 1.0f;


	UPROPERTY(BlueprintReadWrite, Category = Spline)
	FVector ExtraAcceleration = FVector(0.0f, 0.0f, 0.0f);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	bool bOrientRotationAlongSpline = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	bool bKeepZFromSpline = false;


	UPROPERTY(BlueprintAssignable, Category = Spline)
	FSoProjectileMovementNotify OnSplineLeft;
};

