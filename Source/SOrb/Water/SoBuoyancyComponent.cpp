// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoBuoyancyComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "SoOceanManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoBuoyancyComponent::USoBuoyancyComponent()
{
	// tick every frame
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TickGroup;

	// call InitializeComponent
	bWantsInitializeComponent = true;

	// auto activate at creation
	bAutoActivate = true;

	// set default values
	MeshDensity = 600;
	TestPointRadius = 10;
	FluidDensity = 1030;
	FluidLinearDamping = 1;
	FluidAngularDamping = 1;

	VelocityDamper = FVector(0.1, 0.1, 0.1);
	MaxUnderwaterVelocity = 1000;
	bEnableWaveForces = true;
	bClampMaxVelocity = true;
	WaveForceMultiplier = 2;

	// no physics defaults
	bSnapToSurfaceIfNoPhysics = true;
	NoPhysicsDisplacement = FVector::ZeroVector;

	// Debug defaults
	ColorOverWater = FColor(0, 255, 0, 255); // full green
	ColorUnderWater = FColor(255, 255, 0, 255); // full yellow
	SegmentThickness = 1;
	NumberSegments = 8;
	OverrideTestPointRadius = 0;

	// upright constraints
	bEnableStayUprightConstraint = false;
	UprightStiffness = 50;
	UprightDamping = 5;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts
void USoBuoyancyComponent::BeginPlay() { Super::BeginPlay(); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoBuoyancyComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Store the world ref.
	World = GetWorld();
	check(World);

	// If no OceanManager is defined, auto-detect
	if (!OceanManager)
	{
		for (TActorIterator<ASoOceanManager> ActorItr(World); ActorItr; ++ActorItr)
		{
			OceanManager = Cast<ASoOceanManager>(*ActorItr);
			break;
		}
	}

	// no negative point radius
	TestPointRadius = FMath::Abs(TestPointRadius);

	// Store the initial values.
	UPrimitiveComponent* const BasePrimitiveComponent = Cast<UPrimitiveComponent>(GetAttachParent());
	if (BasePrimitiveComponent)
	{
		ApplyUprightConstraint(BasePrimitiveComponent);

		InitialLinearDamping = BasePrimitiveComponent->GetLinearDamping();
		InitialAngularDamping = BasePrimitiveComponent->GetAngularDamping();
		InitialLocation = BasePrimitiveComponent->GetComponentLocation();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void USoBuoyancyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// not active, no parent and no ocean, what life is this?
	USceneComponent* const Parent = GetAttachParent();
	if (!bIsActive || !Parent || !OceanManager) return;

	// upcast the parent to a primitve
	UPrimitiveComponent* const BasePrimitiveComponent = Cast<UPrimitiveComponent>(Parent);
	check(BasePrimitiveComponent);

	// weird world of no physics, snap to water surface
	if (!BasePrimitiveComponent->IsSimulatingPhysics())
	{
		if (!bSnapToSurfaceIfNoPhysics) return;

		// UE_LOG(LogSoWater, Warning, TEXT("Running in no physics mode."));
		const FVector CurrentLocation = BasePrimitiveComponent->GetComponentLocation();
		const FVector WaveHeight = OceanManager->GetWaveHeight(CurrentLocation, World);

		// just a smidge away from the initial location
		BasePrimitiveComponent->SetWorldLocation(NoPhysicsDisplacement + FVector(InitialLocation.X + WaveHeight.X, InitialLocation.Y + WaveHeight.Y, WaveHeight.Z));
		//BasePrimitiveComponent->SetWorldLocation(NoPhysicsDisplacement + FVector(CurrentLocation.X + WaveHeight.X, CurrentLocation.Y + WaveHeight.Y, WaveHeight.Z));
		//BasePrimitiveComponent->SetWorldLocation(FVector(CurrentLocation.X, CurrentLocation.Y, WaveHeight.Z));
		return;
	}

	// world with physics
	const int32 NumPoints = TestPoints.Num();
	if (!NumPoints) // no points, ups
	{
		UE_LOG(LogSoWater, Error, TEXT("Physics is enabled for `%s` but it has no TestPoints"), *BasePrimitiveComponent->GetName());
		return;
	}

	// constants used by every point
	const float BodyMass = BasePrimitiveComponent->GetMass();
	const float Gravity = BasePrimitiveComponent->GetPhysicsVolume()->GetGravityZ();

	// direction of the radius, if the world is upside down
	const float SignedRadius = FMath::Sign(Gravity) * TestPointRadius;

	// test all points
	int32 NumPointsUnderWater = 0; // number of points underwater
	for (int32 PointIndex = 0; PointIndex < NumPoints; PointIndex++)
	{
		// you like to live dangerously, array size changed during runtime
		if (!TestPoints.IsValidIndex(PointIndex)) return;

		bool bIsUnderwater = false;
		// the point in local position
		const FVector Point = TestPoints[PointIndex];

		// the point in world position
		const FVector PointWorldLocation = BasePrimitiveComponent->GetComponentTransform().TransformPosition(Point);
		const FVector WaveHeight = OceanManager->GetWaveHeight(PointWorldLocation, World);

		// test if point radius is under the water surface, add force if it is
		const float TestPointZ = PointWorldLocation.Z + SignedRadius;
		if (BasePrimitiveComponent->IsGravityEnabled() &&
			WaveHeight.Z > TestPointZ)
		{
			NumPointsUnderWater++;
			bIsUnderwater = true;

			// the deeper the point, the higher the force to bring it up
			const float DepthMultiplier = FMath::Clamp((WaveHeight.Z - TestPointZ) / (TestPointRadius * 2), 0.0f, 1.0f);

			// use the point density override if there is one, instead of the MeshDensity
			const float PointDensity = TestPointsDensityOverride.IsValidIndex(PointIndex) ? TestPointsDensityOverride[PointIndex] : MeshDensity;

			// Buoyancy force formula: (Volume(Mass / Density) * Fluid Density * -Gravity) * Depth Multiplier / Num Points
			const float BuoyancyForceZ = (BodyMass / PointDensity) * FluidDensity * -Gravity * DepthMultiplier / NumPoints;

			// velocity damping using velocity at point on the physics body
			FVector DampingForce = -GetUnrealVelocityAtPoint(BasePrimitiveComponent, PointWorldLocation) * VelocityDamper * BodyMass * DepthMultiplier;
			//UE_LOG(LogSoWater, Log, TEXT("DampingForce = %s"), *DampingForce.ToString());

			// push the XY wave force from the ocean
			if (bEnableWaveForces)
			{
				const FVector2D& WaveDirection = OceanManager->GetGlobalWaveDirection();
				// Damping force formula: Mass * Length(wave XY) * Wave direction XY * Wave Force Multiplier / Num Points
				DampingForce += BodyMass * FVector2D(WaveHeight.X, WaveHeight.Y).Size() * FVector(WaveDirection.X, WaveDirection.Y, 0) * WaveForceMultiplier / NumPoints;
			}

			// add force to this point
			BasePrimitiveComponent->AddForceAtLocation(FVector(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyancyForceZ), PointWorldLocation);
		}

		// draw the debug points
		if (bDrawDebugPoints)
		{
			float Radius = TestPointRadius;
			FColor DebugColor = ColorOverWater;
			if (bIsUnderwater) DebugColor = ColorUnderWater;
			if (OverrideTestPointRadius > 0) Radius = OverrideTestPointRadius;

			DrawDebugSphere(World, PointWorldLocation, Radius, /* segments */ NumberSegments, DebugColor,
							/* persistent lines */ false, /* duration */ -1, /* depth priority */ 0, SegmentThickness);
		}
	} // end test all points

	// clamp the velocity to MaxUnderwaterVelocity if there are any points underwater and over the max velocity
	if (bClampMaxVelocity && NumPoints > 0
		&& BasePrimitiveComponent->GetPhysicsLinearVelocity().Size() > MaxUnderwaterVelocity)
	{
		const FVector Velocity = BasePrimitiveComponent->GetPhysicsLinearVelocity().GetSafeNormal() * MaxUnderwaterVelocity;
		BasePrimitiveComponent->SetPhysicsLinearVelocity(Velocity);
	}

	// update damping (resistance) based on number of underwater test points
	BasePrimitiveComponent->SetLinearDamping(InitialLinearDamping + FluidLinearDamping / NumPoints * NumPointsUnderWater);
	BasePrimitiveComponent->SetAngularDamping(InitialAngularDamping + FluidAngularDamping / NumPoints * NumPointsUnderWater);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoBuoyancyComponent::GetUnrealVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName)
{
	if (!Target) return FVector::ZeroVector;

	FBodyInstance* BodyInstance = Target->GetBodyInstance(BoneName);
	if (BodyInstance->IsValidBodyInstance()) { return BodyInstance->GetUnrealWorldVelocityAtPoint(Point); }

	return FVector::ZeroVector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Inspired by UDK's StayUprightSpring (https://wiki.beyondunreal.com/UE3:KActor_(UDK)#bEnableStayUprightSpring)
// Try to keep the Z axis of the actor pointing along the world Z axis.
void USoBuoyancyComponent::ApplyUprightConstraint(UPrimitiveComponent* BasePrimitiveComponent)
{
	if (!bEnableStayUprightConstraint || !BasePrimitiveComponent->IsSimulatingPhysics()) return;

	// settings for the constraint
	FConstraintInstance ConstraintInstance;

	// no constraints against any linear motion
	ConstraintInstance.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
	ConstraintInstance.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
	ConstraintInstance.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);

	// limit the motion and use the limit below
	// limit rotation around the Y axis
	ConstraintInstance.SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Limited);

	// limit rotation round the X axis, I <3 the naming of these functions, very concise on what they do
	ConstraintInstance.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Limited);

	// enable
	ConstraintInstance.SetOrientationDriveTwistAndSwing(true, true);

	// set the rotation limits on XY, lock to 0 angle
	ConstraintInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
	ConstraintInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);

	// set the spring params
	ConstraintInstance.SetAngularDriveParams(UprightStiffness, UprightDamping, 0);
	ConstraintInstance.AngularRotationOffset = BasePrimitiveComponent->GetComponentRotation().GetInverse() + UprightDesiredRotation;

	// set the instance and attach
	UPhysicsConstraintComponent* Constraint = NewObject<UPhysicsConstraintComponent>(BasePrimitiveComponent);
	if (Constraint != nullptr)
	{
		Constraint->ConstraintInstance = ConstraintInstance;
		Constraint->SetWorldLocation(BasePrimitiveComponent->GetComponentLocation());

		// attach
		Constraint->AttachToComponent(BasePrimitiveComponent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
		Constraint->SetConstrainedComponents(BasePrimitiveComponent, NAME_None, nullptr, NAME_None);
	}
}
