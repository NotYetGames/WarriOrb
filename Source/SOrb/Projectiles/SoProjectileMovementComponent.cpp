// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoProjectileMovementComponent.h"

#include "Components/SplineComponent.h"

#include "Basic/Helpers/SoMathHelper.h"
#include "SplineLogic/SoSpline.h"
#include "SoProjectile.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	SplineLocation.SetReferenceActor(GetOwner());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - TickComponent"), STAT_Movement_TickComponent, STATGROUP_SoProjectile);

	float ZDistanceFromSpline = 0.0f;
	if (bKeepZFromSpline && SplineLocation.GetSpline() != nullptr)
		ZDistanceFromSpline = UpdatedComponent->GetComponentLocation().Z - FVector(SplineLocation).Z;

	if (OffsetFromSplineLocation.Size() > KINDA_SMALL_NUMBER)
		OffsetFromSplineLocation = FMath::VInterpTo(OffsetFromSplineLocation, FVector(0.0f, 0.0f, 0.0f), DeltaTime, InterpSpeed);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ASoSpline* Spline = SplineLocation.GetSpline())
	{
		const FVector VelDir = SplineLocation.GetDirectionFromVector(Velocity) * FVector2D(Velocity).Size();
		Velocity.X = VelDir.X;
		Velocity.Y = VelDir.Y;

		if (bOrientRotationAlongSpline)
		{
			const FVector Tangent = Spline->GetSplineComponent()->GetTangentAtDistanceAlongSpline(SplineLocation.GetDistance(), ESplineCoordinateSpace::World);
			UpdatedComponent->SetWorldRotation(Tangent.Rotation());
		}

		if (bKeepZFromSpline)
		{
			FVector Location = UpdatedComponent->GetComponentLocation();
			Location.Z = FVector(SplineLocation).Z + ZDistanceFromSpline;
			UpdatedComponent->SetWorldLocation(Location);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoProjectileMovementComponent::ComputeMoveDelta(const FVector& InVelocity, float DeltaTime) const
{
	return UProjectileMovementComponent::ComputeMoveDelta(InVelocity, DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoProjectileMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit, ETeleportType Teleport)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - MoveUpdatedComponentImpl"), STAT_Movement_MoveUpdatedComponentImpl, STATGROUP_SoProjectile);

	if (SplineLocation.GetSpline() == nullptr)
		return Super::MoveUpdatedComponentImpl(Delta, NewRotation, bSweep, OutHit, Teleport);


	if (UpdatedComponent)
	{
		const int32 DirModifier = ((FVector2D(SplineLocation.GetDirection()) | FVector2D(Delta)) > 0) ? 1 : -1;
		const float DeltaX = Delta.Size2D() * DirModifier;

		const FVector2D OldLocation = FVector2D(UpdatedComponent->GetComponentLocation()) + FVector2D(OffsetFromSplineLocation);
		const FVector2D NewLocation = SplineLocation + DeltaX;

		const FVector DeltaVector = FVector(NewLocation - OldLocation, Delta.Z);
		const bool bMoved = UpdatedComponent->MoveComponent(DeltaVector, NewRotation, bSweep, OutHit, MoveComponentFlags, Teleport);

		if (bMoved)
		{
			float MovedPercent = 1.0f;
			if (OutHit && OutHit->bBlockingHit)
				MovedPercent = OutHit->Time;

			bool bLeftSpline = false;
			float RestTime = 0.0f;
			SplineLocation.ProjectAndAddToDistance(DeltaX * MovedPercent, bLeftSpline, RestTime);

			if (bLeftSpline)
			{
				SplineLocation.SetSpline(nullptr);
				MoveUpdatedComponentImpl(DeltaVector * RestTime, NewRotation, bSweep, OutHit, Teleport);

				OnSplineLeft.Broadcast();
			}
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoProjectileMovementComponent::ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation)
{
	//	if (SplineLocation.GetSpline() == nullptr)
	return Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoProjectileMovementComponent::ComputeBounceResult(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - ComputeBounceResult"), STAT_Movement_ComputeBounceResult, STATGROUP_SoProjectile);

	if (SplineLocation.GetSpline() == nullptr)
		return Super::ComputeBounceResult(Hit, TimeSlice, MoveDelta);


	FVector TempVelocity = Velocity;
	const FVector Normal = USoMathHelper::GetProjectedNormal(TempVelocity, Hit.Normal);
	const float VDotNormal = (TempVelocity | Normal);

	// Only if velocity is opposed by normal
	// if (VDotNormal < 0.f)
	{
		// Project velocity onto normal in reflected direction.
		const FVector ProjectedNormal = Normal * -VDotNormal;

		// Point velocity in direction parallel to surface
		TempVelocity += ProjectedNormal;

		// Only tangential velocity should be affected by friction.
		const float ScaledFriction = (bBounceAngleAffectsFriction || bIsSliding) ? FMath::Clamp(-VDotNormal / TempVelocity.Size(), 0.f, 1.f) * Friction : Friction;
		TempVelocity *= FMath::Clamp(1.f - ScaledFriction, 0.f, 1.f);

		// Coefficient of restitution only applies perpendicular to impact.
		TempVelocity += (ProjectedNormal * FMath::Max(Bounciness, 0.f));

		// Bounciness could cause us to exceed max speed.
		TempVelocity = LimitVelocity(TempVelocity);
	}

	return TempVelocity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoProjectileMovementComponent::ComputeAcceleration(const FVector& InVelocity, float DeltaTime) const
{
	return Super::ComputeAcceleration(InVelocity, DeltaTime) + ExtraAcceleration;
}
