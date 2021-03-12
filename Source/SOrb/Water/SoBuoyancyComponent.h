// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "SoBuoyancyComponent.generated.h"

// forward declare
class ASoOceanManager;

UCLASS(ClassGroup=(Physics), HideCategories = (Transform, Object, Mobility, LOD), Meta=(BlueprintSpawnableComponent))
class SORB_API USoBuoyancyComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoBuoyancyComponent();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Gotta initialize stuff!
	virtual void InitializeComponent() override;

protected:
	// Density of mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	float MeshDensity;

	// Density of water. Typically you don't need to change this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	float FluidDensity;

	// Linear damping (drag) when object is in fluid.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	float FluidLinearDamping;

	// Angular damping (drag) when object is in fluid.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	float FluidAngularDamping;

	// Test point array (to test and apply forces). At least one point is required for buoyancy.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy", Meta = (MakeEditWidget = true))
	TArray<FVector> TestPoints;

	// Radius of the test points (actually a Z offset).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	float TestPointRadius;

	// Maximum velocity that can be achieved under water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	float MaxUnderwaterVelocity;

	// Flag indicating to clamp or not to clamp to MaxUnderwaterVelocity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy")
	bool bClampMaxVelocity;

	// OceanManager used by the component, if unassigned component will auto-detect */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyancy")
	ASoOceanManager* OceanManager;

	// Multiplier (drag force/resistance) that reduces the actual velocity
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyancy")
	FVector VelocityDamper;

	// Waves will push objects towards the wave direction (XY) set in the Ocean Manager.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy")
	bool bEnableWaveForces;

	// How much will the XY wave force damping factor be affected
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy")
	float WaveForceMultiplier;

	// Group where this component ticking is done
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy")
	TEnumAsByte<enum ETickingGroup> TickGroup;

	// Per-point mesh density override, can be used for half-sinking objects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy")
	TArray<float> TestPointsDensityOverride;

	// If object has no physics enabled, snap to water surface.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy No Physics")
	bool bSnapToSurfaceIfNoPhysics;

	// Add this to the actual location to move the center of the object so that it snaps to a different location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy No Physics", Meta = (MakeEditWidget = true))
	FVector NoPhysicsDisplacement;

	// Stay upright physics constraint (inspired by UDK's StayUprightSpring)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Stay Upright")
	bool bEnableStayUprightConstraint;

	// The stiffness of the spring
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Stay Upright")
	float UprightStiffness;

	// The drag force of the spring
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Stay Upright")
	float UprightDamping;

	// The desired upright rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Stay Upright")
	FRotator UprightDesiredRotation;

	// To Draw or not to draw debug points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDrawDebugPoints;

	// Color of the debug sphere when over water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FColor ColorOverWater;

	// Color of the debug sphere when under water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FColor ColorUnderWater;

	// Color of the debug sphere when over water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float SegmentThickness;

	// The number of segments for the debug sphere
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	uint8 NumberSegments;

	// If bigger than 0 it will use this as the radius of the debug sphere instead of the TestPointRadius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float OverrideTestPointRadius;

private:
	// Get current velocity of a point on this physics body, in world space.
	static FVector GetUnrealVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName = NAME_None);

	// Stay upright physics constraint inspired from Ocean Project which was also inspired by UDK's StayUprightSpring (https://wiki.beyondunreal.com/UE3:KActor_(UDK)#bEnableStayUprightSpring)
	void ApplyUprightConstraint(UPrimitiveComponent* BasePrimitiveComponent);

	// one true world
	UWorld* World;

	// initial values set in InitializeComponent
	float InitialAngularDamping;
	float InitialLinearDamping;
	FVector InitialLocation;
};
