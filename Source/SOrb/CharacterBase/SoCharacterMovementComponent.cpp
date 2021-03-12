// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoCharacterMovementComponent.h"

#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

#include "Basic/Helpers/SoMathHelper.h"
#include "CharacterBase/SoMortal.h"
#include "SplineLogic/SoSpline.h"
#include "CharacterBase/SoCharacterBase.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

#include "Logic/SoSimulated.h"

/////////////////////////////////////////////////
// STUFF from charactermovementcomponent, cause something is required for the phys**** functions
DEFINE_LOG_CATEGORY_STATIC(LogCharacterMovement, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogNavMeshMovement, Log, All);

/**
* Character stats
*/
// DECLARE_STATS_GROUP(TEXT("Character"), STATGROUP_Character, STATCAT_Advanced);

//DECLARE_CYCLE_STAT(TEXT("Char Movement Tick"), STAT_CharacterMovementTick, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char Movement Authority Time"), STAT_CharacterMovementAuthority, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char Movement Simulated Time"), STAT_CharacterMovementSimulated, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char CombineNetMove"), STAT_CharacterMovementCombineNetMove, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char SmoothClientPosition"), STAT_CharacterMovementSmoothClientPosition, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char Physics Interation"), STAT_CharPhysicsInteraction, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char FindFloor"), STAT_CharFindFloor, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char AdjustFloorHeight"), STAT_CharAdjustFloorHeight, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char Update Acceleration"), STAT_CharUpdateAcceleration, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char MoveUpdateDelegate"), STAT_CharMoveUpdateDelegate, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char PhysWalking"), STAT_CharPhysWalking, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char PhysFalling"), STAT_CharPhysFalling, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char PhysNavWalking"), STAT_CharPhysNavWalking, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char NavProjectPoint"), STAT_CharNavProjectPoint, STATGROUP_Character);
//DECLARE_CYCLE_STAT(TEXT("Char NavProjectLocation"), STAT_CharNavProjectLocation, STATGROUP_Character);

// MAGIC NUMBERS
const float MAX_STEP_SIDE_Z = 0.08f;	// maximum z value for the normal on the vertical side of steps
const float SWIMBOBSPEED = -80.f;
const float VERTICAL_SLOPE_NORMAL_Z = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle normals slightly off horizontal for vertical surface.




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoCharacterMovementComponent::USoCharacterMovementComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	SoOwner = Cast<ASoCharacterBase>(GetOwner());
	check(SoOwner);

	SplineLocation.SetReferenceActor(GetOwner());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SetDefaultMovementMode()
{
	if (!bOppressSetDefaultMovementMode)
		Super::SetDefaultMovementMode();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - TickComponent"), STAT_Movement_TickComponent, STATGROUP_SoCharacter);

	const FSoSplinePoint OldSplineLocation = SplineLocation;

	bAlrearyRollHitThisFrame = false;

	if (MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == ECustomMovementMode::ECM_Swing)
		SwingInput = ConsumeInputVector();

	// if (SoRootMotionDesc.bVerticalRootMotionEnabled && !SoRootMotionDesc.bHorizontalRootMotionEnabled)
	// {
	// 	CalcVelocity(DeltaTime, GroundFriction, false, BrakingDecelerationWalking);
	// 	// Acceleration.X = 0;
	// 	// Acceleration.Y = 0;
	// }

	bWallJumpedThisFrame = false;
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementMode == EMovementMode::MOVE_Walking)
	{
		if (Velocity.Size2D() > MaxWalkSpeed)
		{
			const FVector Vel = Velocity.GetSafeNormal2D() * MaxWalkSpeed;
			Velocity.X = Vel.X;
			Velocity.Y = Vel.Y;
		}
	}

	// force orientation along spline
	// used to fix orientation in air
	// also is a must, otherwise there are wrong frames in the middle of a turn which leads to camera glitch for the player character
	if (Velocity.SizeSquared2D() > 1.0f)
	{
		if (bOrientRotationToMovement)
			SoOwner->SetActorRotation(SplineLocation.GetDirectionFromVector(Velocity).Rotation());
		else
			if (bOrientRotationToSpline)
				SoOwner->SetActorRotation(SplineLocation.GetDirectionFromVector(SoOwner->GetActorForwardVector()).Rotation());
	}

	if (OldSplineLocation.GetSpline() != SplineLocation.GetSpline())
		SoOwner->OnSplineChanged(OldSplineLocation, SplineLocation);

	// maybe modified if we landed last frame, we reset here to 2.0f
	// BrakingFrictionFactor = 6.0f;

	// let's try to fix issues caused by moving platforms and stuff
	if (bWasPushedAndStuckLastFrame)
	{
		// check if we are inside something
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(UpdatedComponent);
		if (Primitive != nullptr)
		{
			// test with a slightly smaller component so the base doesn't cause trouble if we are just standing on it
			if (GetOwner()->GetWorld()->OverlapBlockingTestByProfile(Primitive->GetComponentLocation(),
																	 Primitive->GetComponentTransform().GetRotation(),
																	 FName("Pawn"), // Primitive->GetCollisionProfileName() returns with Custom?!
																	 Primitive->GetCollisionShape(-5.0f)))
			{
				// try to move out
				bool bFixed = false;
				FHitResult Hit;
				const FQuat Rot = UpdatedComponent->GetComponentTransform().GetRotation();

				const FVector Dir = GetOwner()->GetActorForwardVector();

				const FVector Delta[] = { FVector(0, 0,  0.1f),
										  FVector(0, 0, -0.1f),
											 Dir,
										   - Dir };
				// const FVector OldLoc = UpdatedComponent->GetComponentLocation();
				for (int32 i = 0; i < 4; ++i)
				{
					// if any direction says stuck we wanna fix
					UpdatedComponent->MoveComponent(Delta[i], Rot, true, &Hit, MoveComponentFlags | MOVECOMP_NeverIgnoreBlockingOverlaps);
					// Handle initial penetrations
					if (Hit.bStartPenetrating)
					{
						const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
						const bool bDirFix = ResolvePenetration(RequestedAdjustment, Hit, Rot);

						if (bDirFix)
						{
							bFixed = true;
							// UE_LOG(LogTemp, Error, TEXT("Moved: %f"), (OldLoc - UpdatedComponent->GetComponentLocation()).Size());
						}
					}
					else
						bFixed = true;
				}
				// there is only one way out now:
				if (!bFixed)
					StuckedAndShouldBeKilled();
			}
		}
		bWasPushedAndStuckLastFrame = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SetSplineLocation(ASoSpline* NewSpline, float Distance /*= 0.f*/)
{
	SetSplineLocation(FSoSplinePoint(NewSpline, Distance));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SetSplineLocation(const FSoSplinePoint& InSplineLocation)
{
	SplineLocation.CopySplineLocation(InSplineLocation);
#if WITH_EDITOR
	if (CharacterOwner != nullptr)
	{
		const float Distance = (SplineLocation.ToVector2D() - FVector2D(CharacterOwner->GetActorLocation())).Size();
		if (Distance > 1.0f)
			UE_LOG(LogSoSplineSys, Warning, TEXT("DistanceFromSplineStart isn't 0 in USoCharacterMovementComponent::SetSpline (%f)"), Distance);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::MoveAlongFloor(const FVector& InVelocity, float DeltaSeconds, FStepDownResult* OutStepDownResult)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - MoveAlongFloor"), STAT_Movement_MoveAlongFloor, STATGROUP_SoCharacter);

#if WITH_EDITOR
	if (!SplineLocation.IsValid())
	{
		UE_LOG(LogSoSplineSys, Warning, TEXT("Standard MoveAlongFloor used cause SplineLocation is invalid (check is only in editor atm)"));
		Super::MoveAlongFloor(InVelocity, DeltaSeconds, OutStepDownResult);
		return;
	}
#endif

	if (!CurrentFloor.IsWalkableFloor() || InVelocity.Size2D() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector OldPos = UpdatedComponent->GetComponentLocation();

	// sanity check
	{
		const float Error = (OldPos - SplineLocation).Size2D();
		if (Error > 0.5f)
		{
			// too much error, let's fix it
			SplineLocation.SetDistanceFromWorldLocation(OldPos);
			// UE_LOG(LogSoSplineSys, Warning, TEXT("Too much error in spline (%f) - spline location is reinitialized from actor location!"), Error);
		}
	}


	// Move along the current floor

	FVector Direction = SplineLocation.GetDirection();
	Direction.Z = 0;
	bool bForwardMovement = true;
	if ((FVector2D(Direction) | FVector2D(Velocity)) <= 0)
	{
		Direction *= -1;
		bForwardMovement = false;
	}


	const float SplineDeltaSign = (bForwardMovement ? 1 : -1);
	const float SplineDelta = InVelocity.Size2D() * DeltaSeconds;
	float DeltaOnSpline = SplineDelta * SplineDeltaSign;

	// First attempt to fix the spline distance problem
	// maybe it could be moved inside SplineLocation += ?!
	// performance vs movement accuracy question
	for (int32 i = 0; i < 4; ++i)
	{
		const float RealDelta = (FVector(SplineLocation + DeltaOnSpline) - OldPos).Size2D();
		const float Rest = FMath::Max(SplineDelta - RealDelta, 0.0f);
		DeltaOnSpline += Rest * SplineDeltaSign;
	}

	FVector NewPos = (SplineLocation + DeltaOnSpline);
	FVector Delta = NewPos - OldPos;
	//const float Error = fabs(Delta.Size2D() - InVelocity.Size2D() * DeltaSeconds);
	//if (Error > 0.6f)
	//	UE_LOG(LogCharacterMovement, Warning, TEXT("MoveAlongFloor Error %f"), Error);

	Delta.Z = 0.f;
	FHitResult Hit(1.f);
	FVector RampVector = ComputeGroundMovementDelta(Delta, CurrentFloor.HitResult, CurrentFloor.bLineTrace);
	SafeMoveUpdatedComponent(RampVector, UpdatedComponent->GetComponentQuat(), true, Hit);
	FSoSplinePoint OldSplineLocation = SplineLocation;
	SplineLocation += DeltaOnSpline;
	float LastMoveTimeSlice = DeltaSeconds;

	if (Hit.bStartPenetrating)
	{
		// Allow this hit to be used as an impact we can deflect off, otherwise we do nothing the rest of the update and appear to hitch.
		HandleImpact(Hit);
		float PercentDistanceApplied = SlideAlongSurface(Delta, 1.f, Hit.Normal, Hit, true);
		SplineLocation = OldSplineLocation + DeltaOnSpline * PercentDistanceApplied;

		if (Hit.bStartPenetrating)
		{
			OnCharacterStuckInGeometry(&Hit);
		}

		// UE_LOG(LogTemp, Warning, TEXT("bStartPenetrating"));
	}
	else if (Hit.IsValidBlockingHit())
	{
		// We impacted something (most likely another ramp, but possibly a barrier).
		float PercentTimeApplied = Hit.Time;
		if ((Hit.Time > 0.f) && (Hit.Normal.Z > KINDA_SMALL_NUMBER) && IsWalkable(Hit))
		{
			// Another walkable ramp.
			const float InitialPercentRemaining = 1.f - PercentTimeApplied;
			RampVector = ComputeGroundMovementDelta(Delta * InitialPercentRemaining, Hit, false);
			LastMoveTimeSlice = InitialPercentRemaining * LastMoveTimeSlice;
			SafeMoveUpdatedComponent(RampVector, UpdatedComponent->GetComponentQuat(), true, Hit);

			const float SecondHitPercent = Hit.Time * InitialPercentRemaining;
			PercentTimeApplied = FMath::Clamp(PercentTimeApplied + SecondHitPercent, 0.f, 1.f);

			// UE_LOG(LogTemp, Warning, TEXT("first if"));
		}

		const float DeltaDistance = SplineLocation - OldSplineLocation;
		SplineLocation = OldSplineLocation + DeltaDistance * PercentTimeApplied;
		const float RestDistance = (1.f - PercentTimeApplied) * DeltaDistance;

		if (Hit.IsValidBlockingHit())
		{
			if (CanStepUp(Hit) || (CharacterOwner->GetMovementBase() != NULL && CharacterOwner->GetMovementBase()->GetOwner() == Hit.GetActor()))
			{
				// hit a barrier, try to step up
				const FVector GravDir(0.f, 0.f, -1.f);
				auto SavedHid = Hit;
				const FVector LocationBeforeStepUp = UpdatedComponent->GetComponentLocation();
				bool bCouldStepUp = StepUp(GravDir, Delta * (1.f - PercentTimeApplied), Hit, OutStepDownResult);
				if (!bCouldStepUp)
				{
					UE_LOG(LogCharacterMovement, Verbose, TEXT("- StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					HandleImpact(Hit, LastMoveTimeSlice, RampVector);

					float PercentDistanceApplied = SlideAlongSurface(Delta, 1.f - PercentTimeApplied, Hit.Normal, Hit, true);
					SplineLocation += RestDistance * PercentDistanceApplied;
					// UE_LOG(LogTemp, Warning, TEXT("Second if / true / true"));

					float StepUpHitVelocity = 0.0f;
					float DefaultBounce = 0.0f;
					static const FName NoFloorBounceTag = FName("NoFloorBounce");
					if (SoOwner->ShouldBounceOnHit(StepUpHitVelocity, DefaultBounce, &Hit) &&
						Hit.GetActor() != nullptr &&
						!Hit.GetActor()->ActorHasTag(NoFloorBounceTag))
					{
						const FVector OldVelocity = Velocity;
						if (bCouldStepUp)
							Velocity.Z = Velocity.Z + StepUpHitVelocity;
						else Velocity = Velocity - SavedHid.ImpactNormal * (SavedHid.ImpactNormal | Velocity) * 2;

						float RollStoredValue = SplineLocation.GetDirectionModifierFromVector(Velocity);
						SoOwner->OnBounce(false, RollStoredValue, SavedHid.ImpactPoint, SavedHid.ImpactNormal);
						if (Velocity.Z > KINDA_SMALL_NUMBER)
						{
							SetMovementMode(EMovementMode::MOVE_Falling);
							return;
						}
						// const FVector NewInVelocity = InVelocity - SavedHid.ImpactNormal * (SavedHid.ImpactNormal | InVelocity) * 2;
						// TODO: pray that it solves everything and doesn't ruin anything
						// UPDATE: I am not good at praying, we should hire some religious developers

						if (!bAlrearyRollHitThisFrame)
						{
							bAlrearyRollHitThisFrame = true;
							MoveAlongFloor(-InVelocity, DeltaSeconds * (1.f - PercentTimeApplied), OutStepDownResult);
						}
					}
				}
				else
				{
					// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
					UE_LOG(LogCharacterMovement, Verbose, TEXT("+ StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					bJustTeleported |= !bMaintainHorizontalGroundVelocity;

					// I guess StepUp did the movement after all
					SplineLocation += FMath::Sign(RestDistance) * (UpdatedComponent->GetComponentLocation() - LocationBeforeStepUp).Size2D();
					// UE_LOG(LogTemp, Warning, TEXT("Second if / true / false"));
				}
			}
			else if (Hit.Component.IsValid() && !Hit.Component.Get()->CanCharacterStepUp(CharacterOwner))
			{
				HandleImpact(Hit, LastMoveTimeSlice, RampVector);

				float PercentDistanceApplied = SlideAlongSurface(Delta, 1.f - PercentTimeApplied, Hit.Normal, Hit, true);
				SplineLocation += RestDistance * PercentDistanceApplied;

				// static int i = 0;
				// ++i;
				// UE_LOG(LogTemp, Warning, TEXT("Second if / else %d"), i);

			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCharacterMovementComponent::ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation)
{
	if (!SplineLocation.IsValid(false))
		return Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotation);

	return  ResolvePenetrationOnSpline(Adjustment, Hit, NewRotation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCharacterMovementComponent::ResolvePenetrationOnSpline(const FVector& ProposedAdjustment, const FHitResult& Hit, const FQuat& NewRotationQuat)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - ResolvePenetrationOnSpline"), STAT_Movement_ResolvePenetrationOnSpline, STATGROUP_SoCharacter);

	// SceneComponent can't be in penetration, so this function really only applies to PrimitiveComponent.
	const FVector Adjustment = FVector::VectorPlaneProject(ProposedAdjustment, SplineLocation.GetPlaneNormal());
	if (Adjustment.IsZero() || !UpdatedPrimitive)
		return false;

	// See if we can fit at the adjusted location without overlapping anything.
	AActor* ActorOwner = UpdatedComponent->GetOwner();
	if (!ActorOwner)
		return false;

	UE_LOG(LogSoSplineSys, Verbose, TEXT("ResolvePenetration: %s.%s at location %s inside %s.%s at location %s by %.3f (netmode: %d)"),
		   *ActorOwner->GetName(),
		   *UpdatedComponent->GetName(),
		   *UpdatedComponent->GetComponentLocation().ToString(),
		   *GetNameSafe(Hit.GetActor()),
		   *GetNameSafe(Hit.GetComponent()),
		   Hit.Component.IsValid() ? *Hit.GetComponent()->GetComponentLocation().ToString() : TEXT("<unknown>"),
		   Hit.PenetrationDepth,
		   (uint32)GetNetMode());

	// We really want to make sure that precision differences or differences between the overlap test and sweep tests don't put us into another overlap,
	// so make the overlap test a bit more restrictive.
	const float OverlapInflation = 0.1f;
	const bool bEncroached = OverlapTest(Hit.TraceStart + Adjustment, NewRotationQuat, UpdatedPrimitive->GetCollisionObjectType(), UpdatedPrimitive->GetCollisionShape(OverlapInflation), ActorOwner);
	if (!bEncroached)
	{
		// Move without sweeping.
		MoveUpdatedComponent(Adjustment, NewRotationQuat, false, nullptr, ETeleportType::TeleportPhysics);
		SplineLocation += Adjustment.Size2D() * SplineLocation.GetDirectionModifierFromVector(Adjustment);

		UE_LOG(LogSoSplineSys, Verbose, TEXT("ResolvePenetration:   teleport by %s"), *Adjustment.ToString());
		return true;
	}

	// Disable MOVECOMP_NeverIgnoreBlockingOverlaps if it is enabled, otherwise we wouldn't be able to sweep out of the object to fix the penetration.
	TGuardValue<EMoveComponentFlags> ScopedFlagRestore(MoveComponentFlags, EMoveComponentFlags(MoveComponentFlags & (~MOVECOMP_NeverIgnoreBlockingOverlaps)));

	// Try sweeping as far as possible...
	FHitResult SweepOutHit(1.f);
	bool bMoved = MoveUpdatedComponent(Adjustment, NewRotationQuat, true, &SweepOutHit, ETeleportType::TeleportPhysics);
	if (bMoved)
		SplineLocation += Adjustment.Size2D() * SplineLocation.GetDirectionModifierFromVector(Adjustment) * SweepOutHit.Time;

	UE_LOG(LogSoSplineSys, Verbose, TEXT("ResolvePenetration:   sweep by %s (success = %d)"), *Adjustment.ToString(), bMoved);

	// Still stuck?
	if (!bMoved && SweepOutHit.bStartPenetrating)
	{
		// Combine two MTD results to get a new direction that gets out of multiple surfaces.
		const FVector SecondMTD = FVector::VectorPlaneProject(GetPenetrationAdjustment(SweepOutHit), SplineLocation.GetPlaneNormal());
		const FVector CombinedMTD = Adjustment + SecondMTD;
		if (SecondMTD != Adjustment && !CombinedMTD.IsZero())
		{
			bMoved = MoveUpdatedComponent(CombinedMTD, NewRotationQuat, true, &SweepOutHit, ETeleportType::TeleportPhysics);
			if (bMoved)
				SplineLocation += CombinedMTD.Size2D() * SplineLocation.GetDirectionModifierFromVector(CombinedMTD) * SweepOutHit.Time;
			UE_LOG(LogSoSplineSys, Verbose, TEXT("ResolvePenetration:   sweep by %s (MTD combo success = %d)"), *CombinedMTD.ToString(), bMoved);
		}
	}

	// Still stuck?
	if (!bMoved)
	{
		// Try moving the proposed adjustment plus the attempted move direction. This can sometimes get out of penetrations with multiple objects
		const FVector MoveDelta = ConstrainDirectionToPlane(Hit.TraceEnd - Hit.TraceStart);
		if (!MoveDelta.IsZero())
		{
			const FVector LastTryDelta = Adjustment + MoveDelta;
			bMoved = MoveUpdatedComponent(LastTryDelta, NewRotationQuat, true, &SweepOutHit, ETeleportType::TeleportPhysics);
			if (bMoved)
				SplineLocation += LastTryDelta.Size2D() * SplineLocation.GetDirectionModifierFromVector(LastTryDelta) * SweepOutHit.Time;
			UE_LOG(LogSoSplineSys, Verbose, TEXT("ResolvePenetration:   sweep by %s (adjusted attempt success = %d)"), *(Adjustment + MoveDelta).ToString(), bMoved);
		}
	}

	return bMoved;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	if (SoOwner->OnPreLanded(Hit))
	{
		// value modified to minimize horizontal movement after landing
		// BrakingFrictionFactor = 6.0f;
		UCharacterMovementComponent::ProcessLanded(Hit, remainingTime, Iterations);
	}
	else
		StartNewPhysics(remainingTime, Iterations);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoCharacterMovementComponent::ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	FVector PlaneNormal = Delta.CrossProduct(Delta, FVector(0.f, 0.f, 1.f));
	PlaneNormal.Normalize();
	const FVector ProjectedNormal = FVector::VectorPlaneProject(Normal, PlaneNormal).GetSafeNormal();
	return Super::ComputeSlideVector(Delta, Time, ProjectedNormal, Hit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - PhysWalking"), STAT_Movement_PhysWalking, STATGROUP_SoCharacter);

	//if (bControlledMovement)
	//	PhysCustomForcedWalking(deltaTime, Iterations);
	//else
	{
		if (HasAnimRootMotion() && SoRootMotionDesc.bVerticalRootMotionEnabled)
		{
			if (Velocity.Z > KINDA_SMALL_NUMBER)
			{
				SetMovementMode(MOVE_Falling);
				StartNewPhysics(deltaTime, 0);
				return;
			}

			if (!SoRootMotionDesc.bHorizontalRootMotionEnabled)
			{
				const FVector OldVelocity = Velocity;
				MaintainHorizontalGroundVelocity();
				Acceleration.Z = 0.f;
				// autch
				RootMotionParams.bHasRootMotion = false;
				CalcVelocity(deltaTime, GroundFriction, false, BrakingDecelerationWalking);
				RootMotionParams.bHasRootMotion = true;
				checkCode(ensureMsgf(!Velocity.ContainsNaN(), TEXT("PhysWalking: Velocity contains NaN after CalcVelocity (%s)\n%s"), *GetPathNameSafe(this), *Velocity.ToString()));
				Velocity.Z = OldVelocity.Z;
			}
		}

		UCharacterMovementComponent::PhysWalking(deltaTime, Iterations);
	}

	// Root Motion has been used, clear - cause animation should not ruin the rotation, that's why
	if (!bAllowRootMotionRotation)
		RootMotionParams.Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - PhysFalling"), STAT_Movement_PhysFalling, STATGROUP_SoCharacter);

	if (SplineLocation.GetSpline() != nullptr)
		SplinePhysFalling(deltaTime, Iterations);
	else
		UCharacterMovementComponent::PhysFalling(deltaTime, Iterations);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::PhysFlying(float deltaTime, int32 Iterations)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - PhysFlying"), STAT_Movement_PhysFlying, STATGROUP_SoCharacter);

	if (SplineLocation.GetSpline() != nullptr)
		SplinePhysFlying(deltaTime, Iterations);
	else
		UCharacterMovementComponent::PhysFlying(deltaTime, Iterations);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	switch (CustomMovementMode)
	{
		case ECustomMovementMode::ECM_Swing:
			PhysCustomSwinging(deltaTime, Iterations);
			break;

		//case ECustomMovementMode::ECM_Controlled:
		//	PhysControlledMovement(deltaTime, Iterations);
		//	break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void USoCharacterMovementComponent::PhysCustomForcedWalking(float deltaTime, int32 Iterations)
//{
//	if (deltaTime < MIN_TICK_TIME) { return; }
//
//	float remainingTime = deltaTime;
//
//	// Perform the move
//	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner)
//	{
//		Iterations++;
//		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
//		remainingTime -= timeTick;
//
//		// Save current values
//		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
//
//		// Ensure velocity is horizontal.
//		MaintainHorizontalGroundVelocity();
//		const FVector OldVelocity = Velocity;
//		Acceleration.Z = 0.f;
//
//		// Compute move parameters
//		const FVector MoveVelocity = Velocity;
//		const FVector Delta = timeTick * MoveVelocity;
//		const bool bZeroDelta = Delta.IsNearlyZero();
//		FStepDownResult StepDownResult;
//
//		const FFindFloorResult OldFloor = CurrentFloor;
//		if (!CurrentFloor.IsWalkableFloor())
//		{
//			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
//		}
//
//		if (bZeroDelta) remainingTime = 0.f;
//		else MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);
//
//		// Update floor.
//		// StepUp might have already done it for us.
//		if (StepDownResult.bComputedFloor)
//		{
//			CurrentFloor = StepDownResult.FloorResult;
//		}
//		else
//		{
//			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
//		}
//
//		bool bWalkableFloor = CurrentFloor.IsWalkableFloor();
//		if ( !bWalkableFloor || (bWalkableFloor && ShouldCatchAir(OldFloor, CurrentFloor)))
//		{
//			CharacterOwner->OnWalkingOffLedge(OldFloor.HitResult.ImpactNormal, OldFloor.HitResult.Normal, OldLocation, timeTick);
//			SetMovementMode(EMovementMode::MOVE_Falling);
//			StartFalling(Iterations, remainingTime, timeTick, Delta, OldLocation);
//			return;
//		}
//
//		// Make velocity reflect actual move
//		if (timeTick >= MIN_TICK_TIME)
//		{
//			Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
//		}
//
//		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
//		if (UpdatedComponent->GetComponentLocation() == OldLocation)
//		{
//			remainingTime = 0.f;
//			break;
//		}
//	}
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::PhysCustomSwinging(float deltaTime, int32 Iterations)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - PhysCustomSwinging"), STAT_Movement_PhysCustomSwinging, STATGROUP_SoCharacter);

	if (deltaTime < MIN_TICK_TIME) { return; }

	float remainingTime = deltaTime;

	// Perform the move
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner)
	{
		Iterations++;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();

		// calculate acceleration
		const FVector SwingCenterToCharacter = OldLocation - SwingCenter;
		const float SwingArm = FMath::Clamp(SwingCenterToCharacter.Size() + SwingArmDelta, SwingArmMin, SwingArmMax);
		SwingArmDelta = 0.0f;

		FVector2D Direction2D = FVector2D(SplineLocation.GetDirection());

		// calculate Acceleration and set the direction to match the spline's
		Direction2D.Normalize();
		const FVector Gravity(0.f, 0.f, GetGravityZ());
		FVector SwingAcceleration = (Gravity - Gravity.ProjectOnTo(SwingCenterToCharacter));

		// user tried to move
		if (SwingInput.Size2D() > 0)
		{
			// user input is added only if the angle between Gravity and (CharacterLocation - SwingCenter) is small enough
			if (((Gravity - SwingAcceleration).GetSafeNormal() | Gravity.GetSafeNormal()) > 0.7f)
				SwingAcceleration += SwingAcceleration.GetSafeNormal() * ((FVector2D(SwingInput) | FVector2D(SwingAcceleration)) > 0.f ? 1.f : -1.f) * SwingInputForce;
		}

		int32 DirModifier = (Direction2D | FVector2D(SwingAcceleration)) > 0.f ? 1 : -1;
		FVector2D SwingAccel2D = Direction2D * DirModifier * SwingAcceleration.Size2D();
		SwingAcceleration.X = SwingAccel2D.X;
		SwingAcceleration.Y = SwingAccel2D.Y;

		// update velocity
		Velocity += SwingAcceleration * timeTick;

		// align velocity
		DirModifier = (Direction2D | FVector2D(Velocity)) > 0.f ? 1 : -1;
		const FVector2D SwingVelocity = Direction2D * DirModifier * Velocity.Size2D();
		Velocity.X = SwingVelocity.X;
		Velocity.Y = SwingVelocity.Y;

		if (Velocity.Size2D() > KINDA_SMALL_NUMBER)
		{
			bIsSwingForward = (GetOwner()->GetActorForwardVector() | Velocity.GetSafeNormal()) > 0;
			if (GetOwner()->GetActorLocation().Z > SwingCenter.Z)
				bIsSwingForward = !bIsSwingForward;
		}


		// DEBUG
		//FVector LinkEnd = OldLocation + SwingAcceleration;
		//DrawDebugLine(GetWorld(), OldLocation, LinkEnd, FColor(255, 0, 0), false, -1, 0, 12.333);
		//
		//LinkEnd = OldLocation + Velocity;
		//DrawDebugLine(GetWorld(), OldLocation, LinkEnd, FColor(0, 0, 255), false, -1, 0, 12.333);

		FVector PointWithoutRope = OldLocation + Velocity * timeTick;
		FVector NewLocation = SwingCenter + (PointWithoutRope - SwingCenter).GetSafeNormal() * SwingArm;

		const FVector Delta = NewLocation - OldLocation;
		DirModifier = (Direction2D | FVector2D(Delta)) > 0.f ? 1 : -1;

		FSoSplinePoint OldSplinePoint = SplineLocation;
		SplineLocation += (Delta.Size2D() * DirModifier);

		FHitResult Hit;
		UpdatedComponent->SetWorldLocation(NewLocation, true, &Hit);

		if (Hit.IsValidBlockingHit())
		{
			SplineLocation = OldSplinePoint + Delta.Size2D() * DirModifier * Hit.Time;
		}

		// Make velocity reflect actual move
		if (timeTick >= MIN_TICK_TIME)
		{
			Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}
}

// Old offset based controlled movement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void USoCharacterMovementComponent::PhysControlledMovement(float deltaTime, int32 Iterations)
//{
//	if (deltaTime < MIN_TICK_TIME) { return; }
//
//	float remainingTime = deltaTime;
//
//	const FVector OldLocation = GetActorLocation();
//	float RestTime = 0.f;
//	// Perform the move
//	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner)
//	{
//		Iterations++;
//		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
//		remainingTime -= timeTick;
//
//		const float BeforeTime = ControlledMovementDesc.CurrentTime;
//		float AfterTime = BeforeTime + timeTick;
//
//		if (AfterTime > ControlledMovementDesc.Duration)
//		{
//			RestTime = AfterTime - ControlledMovementDesc.Duration;
//			bControlledMovement = false;
//			AfterTime = ControlledMovementDesc.Duration;
//		}
//		ControlledMovementDesc.CurrentTime = AfterTime;
//
//		// recalculate velocity
//		const float BeforePercent = BeforeTime / ControlledMovementDesc.Duration;
//		const float AfterPercent = AfterTime / ControlledMovementDesc.Duration;
//
//		float BeforeXDelta = ControlledMovementDesc.BaseOffset.X;
//		if (ControlledMovementDesc.XCurve)
//			BeforeXDelta *= ControlledMovementDesc.XCurve->GetFloatValue(BeforePercent);
//
//		float AfterXDelta = ControlledMovementDesc.BaseOffset.X;
//		if (ControlledMovementDesc.XCurve)
//			AfterXDelta *= ControlledMovementDesc.XCurve->GetFloatValue(AfterPercent);
//
//		float BeforeZDelta = ControlledMovementDesc.BaseOffset.Y;
//		if (ControlledMovementDesc.ZCurve)
//			BeforeZDelta *= ControlledMovementDesc.ZCurve->GetFloatValue(BeforePercent);
//
//		float AfterZDelta = ControlledMovementDesc.BaseOffset.Y;
//		if (ControlledMovementDesc.ZCurve)
//			AfterZDelta *= ControlledMovementDesc.ZCurve->GetFloatValue(AfterPercent);
//
//		const int DirModifier = (FVector2D(SplineLocation.GetDirection()) | FVector2D(GetOwner()->GetActorForwardVector())) > 0 ? 1 : -1;
//		const float DeltaZ = AfterZDelta - BeforeZDelta;
//		const float DeltaX = (AfterXDelta - BeforeXDelta) * DirModifier;
//
//		if (fabs(DeltaX) < KINDA_SMALL_NUMBER && fabs(DeltaZ) < KINDA_SMALL_NUMBER)
//			continue;
//
//		FVector Delta = FVector(SplineLocation + DeltaX) - FVector(SplineLocation);
//		Delta.Z = DeltaZ;
//
//		FHitResult Hit(1.f);
//		SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
//		float MovedPercent = 1.0f;
//		if (Hit.bBlockingHit)
//			MovedPercent = Hit.Time;
//		SplineLocation += DeltaX * MovedPercent;
//	}
//
//	const FVector Location = GetActorLocation();
//
//	if (deltaTime > KINDA_SMALL_NUMBER)
//		Velocity = (Location - OldLocation) / deltaTime;
//
//	if (!bControlledMovement)
//	{
//		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false, NULL);
//
//		if (CurrentFloor.IsWalkableFloor())
//			SetMovementMode(EMovementMode::MOVE_Walking);
//		else
//		{
//			SetMovementMode(EMovementMode::MOVE_Falling);
//			StartFalling(Iterations, RestTime, deltaTime, Location - OldLocation, OldLocation);
//		}
//
//		ControlledMovementDesc = FSoControlledMovementDesc();
//	}
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SplinePhysFlying(float deltaTime, int32 Iterations)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - SplinePhysFlying"), STAT_Movement_SplinePhysFlying, STATGROUP_SoCharacter);

	if (deltaTime < MIN_TICK_TIME)
		return;

	RestorePreAdditiveRootMotionVelocity();

	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		CalcVelocity(deltaTime, GroundFriction, false, GetMaxBrakingDeceleration());

	ApplyRootMotionToVelocity(deltaTime);

	Iterations++;
	bJustTeleported = false;

	if (Velocity.IsNearlyZero())
	{
		return;
	}

	// Move
	FHitResult Hit(1.f);
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FSoSplinePoint OldSplineLocation = SplineLocation;

	{
		const int32 DirModifier = (FVector2D(SplineLocation.GetDirection()) | FVector2D(Velocity)) > 0.f ? 1 : -1;
		const float DeltaMovement = Velocity.Size2D() * deltaTime * DirModifier;
		SplineLocation += DeltaMovement;
	}

	FVector NewPos = SplineLocation;
	NewPos.Z = OldLocation.Z + Velocity.Z * deltaTime;
	const FVector Adjusted = NewPos - OldLocation;

	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.Time < 1.f)
	{
		const FVector GravDir = FVector(0.f, 0.f, -1.f);
		const FVector VelDir = Velocity.GetSafeNormal();
		const float UpDown = GravDir | VelDir;

		//bool bSteppedUp = false;
		//if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
		//{
		//	float stepZ = UpdatedComponent->GetComponentLocation().Z;
		//	bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
		//	if (bSteppedUp)
		//	{
		//		OldLocation.Z = UpdatedComponent->GetComponentLocation().Z + (OldLocation.Z - stepZ);
		//	}
		//}
		//
		//if (!bSteppedUp)
		{
			//adjust and try again
			HandleImpact(Hit, deltaTime, Adjusted);
			const float PercentDistanceApplied = SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
			SplineLocation = OldSplineLocation + (SplineLocation - OldSplineLocation) * PercentDistanceApplied;
		}
	}

	//if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	//{
	//	Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	//}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SplinePhysFalling(float deltaTime, int32 Iterations)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Movement - SplinePhysFalling"), STAT_Movement_SplinePhysFalling, STATGROUP_SoCharacter);

	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	FVector FallAcceleration = GetFallingLateralAcceleration(deltaTime);
	FallAcceleration.Z = 0.f;
	const bool bHasAirControl = (FallAcceleration.SizeSquared2D() > 0.f);

	float remainingTime = deltaTime;
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
	{
		Iterations++;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
		bJustTeleported = false;

		FVector OldVelocity = Velocity;
		FVector VelocityNoAirControl = Velocity;

		// Apply input

		// Compute VelocityNoAirControl
		if (bHasAirControl)
		{
			// Find velocity *without* acceleration.
			TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
			TGuardValue<FVector> RestoreVelocity(Velocity, Velocity);
			Velocity.Z = 0.f;
			const bool bWayTooFast = bSlowDownToMaxWalkSpeedInFall && Velocity.Size2D() > MaxWalkSpeed;
			// const bool bWayTooFast = false;
			CalcVelocity(timeTick, FallingLateralFriction, false, bWayTooFast ? BrakingDecelerationFallingOverMaxWalkSpeed : BrakingDecelerationFalling);
			VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
		}

		// Compute Velocity
		{
			// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
			TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
			Velocity.Z = 0.f;
			// oh well...
			const bool bSavedHasRootMotion = RootMotionParams.bHasRootMotion;
			RootMotionParams.bHasRootMotion = false;
			//const bool bWayTooFast = false;
			const bool bWayTooFast = bSlowDownToMaxWalkSpeedInFall && Velocity.Size2D() > MaxWalkSpeed;
			CalcVelocity(timeTick, FallingLateralFriction, false, bWayTooFast ? BrakingDecelerationFallingOverMaxWalkSpeed : BrakingDecelerationFalling);
			RootMotionParams.bHasRootMotion = bSavedHasRootMotion;
			Velocity.Z = OldVelocity.Z;
		}

		// Just copy Velocity to VelocityNoAirControl if they are the same (ie no acceleration).
		if (!bHasAirControl)
		{
			VelocityNoAirControl = Velocity;
		}

		// force velocity to follow the spline
		{
			FVector Direction = SplineLocation.GetDirection();
			Direction.Z = 0;
			Direction.Normalize();

			if ((FVector2D(Direction) | FVector2D(Velocity)) <= 0)
			{
				Direction *= -1;
			}

			FVector2D Velocity2D = FVector2D(Direction.X, Direction.Y) * Velocity.Size2D();
			Velocity = FVector(Velocity2D.X, Velocity2D.Y, Velocity.Z);

			FVector2D VelocityNoAirControl2D = FVector2D(Direction.X, Direction.Y) * VelocityNoAirControl.Size2D();
			VelocityNoAirControl = FVector(VelocityNoAirControl2D.X, VelocityNoAirControl2D.Y, VelocityNoAirControl.Z);
		}

		if (!HasAnimRootMotion() || !SoRootMotionDesc.bVerticalRootMotionEnabled)
		{
			// Apply gravity
			if (bFallVelocityOverride)
			{
				Velocity.Z = FallVelocityOverrideValue;
				VelocityNoAirControl.Z = FallVelocityOverrideValue;
			}
			else
			{
				const FVector Gravity(0.f, 0.f, GetGravityZ());
				Velocity = NewFallVelocity(Velocity, Gravity, timeTick);
				VelocityNoAirControl = NewFallVelocity(VelocityNoAirControl, Gravity, timeTick);
			}
		}

		const FVector AirControlAccel = (Velocity - VelocityNoAirControl) / timeTick;

		if (bNotifyApex && CharacterOwner->Controller && (Velocity.Z <= 0.f))
		{
			// Just passed jump apex since now going down
			bNotifyApex = false;
			NotifyJumpApex();
		}


		// Move
		FHitResult Hit(1.f);
		const FVector TempVelocity = (OldVelocity + Velocity) * 0.5f;
		int32 DirModifier = (FVector2D(SplineLocation.GetDirection()) | FVector2D(TempVelocity)) > 0.f ? 1 : -1;
		const float DeltaMovement = TempVelocity.Size2D() * timeTick * DirModifier;

		FVector NewPos = (SplineLocation + DeltaMovement);
		NewPos.Z = OldLocation.Z + 0.5f * (OldVelocity.Z + Velocity.Z) * timeTick;
		FVector Adjusted = NewPos - OldLocation;
		// WARNING: SplineLocation might change during this call!!!
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

		if (!HasValidData())
		{
			return;
		}

		float LastMoveTimeSlice = timeTick;
		float subTimeTickRemaining = timeTick * (1.f - Hit.Time);

		if (IsSwimming()) //just entered water
		{
			SplineLocation = SplineLocation + (DeltaMovement) * Hit.Time;

			remainingTime += subTimeTickRemaining;
			StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
			return;
		}

		if (!Hit.bBlockingHit)
		{
			SplineLocation += DeltaMovement;
		}
		else
		{
			// DrawDebugLine(SoOwner->GetWorld(), OldLocation, OldLocation + Adjusted.GetSafeNormal() * 500, FColor::Blue);

   			SplineLocation = SplineLocation + DeltaMovement * Hit.Time;

			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += subTimeTickRemaining;
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}

			// Compute impact deflection based on final velocity, not integration step.
			// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
			Adjusted = Velocity * timeTick;

			// See if we can convert a normally invalid landing spot (based on the hit result) to a usable one.
			if (!Hit.bStartPenetrating && ShouldCheckForValidLandingSpot(timeTick, Adjusted, Hit))
			{
				const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
				FFindFloorResult FloorResult;
				FindFloor(PawnLocation, FloorResult, false);
				if (FloorResult.IsWalkableFloor() && IsValidLandingSpot(PawnLocation, FloorResult.HitResult))
				{
					remainingTime += subTimeTickRemaining;
					ProcessLanded(FloorResult.HitResult, remainingTime, Iterations);
					return;
				}
			}

			const FVector VelocityBeforeRoll = Velocity;
			float StepUpHitVelocity = 0.0f;
			float DefaultBounce = 0.0f;
			bool bParallel = false;
			if (SoOwner->ShouldBounceOnHit(StepUpHitVelocity, DefaultBounce, &Hit))
			{
				if (UPrimitiveComponent* Component = Hit.GetComponent())
				{
					static FName ApplyForceTag = FName("ApplyForce");
					if (Component->ComponentHasTag(ApplyForceTag))
					{
						ISoSimulated::Execute_OnPlayerBounce(Hit.GetActor(), Velocity, Hit);
					}
				}

				const FVector ProjectedHitNormal = USoMathHelper::GetProjectedNormal(Velocity, Hit.ImpactNormal);

				// DrawDebugLine(SoOwner->GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + ProjectedHitNormal.GetSafeNormal() * 500, FColor::Green, false, 1.0f);

				const FVector VecNormal = Velocity.GetSafeNormal();
				// UE_LOG(LogTemp, Warning, TEXT("CosAlpha: %f"), CosAlpha);
				bool bWallJump = false;

				// no wall jump from e.g. ceiling
				const float CosAWithZ = ProjectedHitNormal | FVector(0.0f, 0.0f, -1.0f);
				const bool bMightWallJump = !bWallJumpedThisFrame && (CosAWithZ < 0.5f);

				FVector WallJumpVelocity = ProjectedHitNormal.GetSafeNormal2D() * 200.0f;
				WallJumpVelocity.Z = 950;

				if (bMightWallJump && SoOwner->CanPerformWallJump(Hit))
				{
					bWallJump = true;
					Velocity = WallJumpVelocity;
					bWallJumpedThisFrame = true;
				}
				else
				{
					// just bouncing
					float CosAlpha = ProjectedHitNormal | VecNormal;
					bParallel = fabs(CosAlpha) + KINDA_SMALL_NUMBER > 1.0f;
					Velocity = (!bParallel) ? (Velocity - ProjectedHitNormal * (ProjectedHitNormal | Velocity) * 2) : -Velocity;
					Velocity *= DefaultBounce;

					if (bMightWallJump)
					{
						MissedWallJumpTime = GetWorld()->GetTimeSeconds();
						MissedWallJumpVelocity = WallJumpVelocity;
						MissedImpactPointOffset = Hit.ImpactPoint - UpdatedComponent->GetComponentLocation();
						MissedImpactNormal = Hit.ImpactNormal;
					}
				}

				Acceleration.X = 0.f;
				Acceleration.Y = 0.f;

				float RollStoredValue = (!bParallel) && (CosAWithZ < 0.5f) ? ((FVector2D(SplineLocation.GetDirection()) | FVector2D(Velocity)) > 0.f ? 1 : -1)
																		   : 0.f;

				if (CosAWithZ > 0.5f)
					RollStoredValue = 0.0f;
				MissedRollStoredValue = RollStoredValue;

				SoOwner->OnBounce(bWallJump, RollStoredValue, Hit.ImpactPoint, Hit.ImpactNormal);

				// DrawDebugLine(SoOwner->GetWorld(), SoOwner->GetActorLocation(), SoOwner->GetActorLocation() + Velocity.GetSafeNormal() * 500, FColor::Blue, false, 1.0f);
				// static int i = 0;
				// ++i;
				// UE_LOG(LogTemp, Warning, TEXT("Roll dir: %d"), i);

				remainingTime += subTimeTickRemaining;
				SplinePhysFalling(remainingTime, Iterations);
				return;
			}

			HandleImpact(Hit, LastMoveTimeSlice, Adjusted);

			// If we've changed physics mode, abort.
			if (!HasValidData() || !IsFalling())
			{
				return;
			}

			// Limit air control based on what we hit.
			// We moved to the impact point using air control, but may want to deflect from there based on a limited air control acceleration.
			if (bHasAirControl)
			{
				const bool bCheckLandingSpot = false; // we already checked above.
				const FVector AirControlDeltaV = LimitAirControl(LastMoveTimeSlice, AirControlAccel, Hit, bCheckLandingSpot) * LastMoveTimeSlice;
				Adjusted = (VelocityNoAirControl + AirControlDeltaV) * LastMoveTimeSlice;
			}

			const FVector OldHitNormal = USoMathHelper::GetProjectedNormal(VelocityBeforeRoll, Hit.Normal);
			const FVector OldHitImpactNormal = USoMathHelper::GetProjectedNormal(VelocityBeforeRoll, Hit.ImpactNormal);
			FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

			// Compute velocity after deflection (only gravity component for RootMotion)
			if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
			{
				Velocity = (Delta / subTimeTickRemaining);
			}

			if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
			{
				// UE_LOG(LogTemp, Warning, TEXT("Spline Location DELTA: %f %f %f"), Delta.GetSafeNormal().X, Delta.GetSafeNormal().Y, Delta.GetSafeNormal().Z);
				// UE_LOG(LogTemp, Warning, TEXT("Spline Location ADJUSTED: %f %f %f"), Adjusted.GetSafeNormal().X, Adjusted.GetSafeNormal().Y, Adjusted.GetSafeNormal().Z);

				// Move in deflected direction.
				SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

				DirModifier = (FVector2D(SplineLocation.GetDirection()) | FVector2D(Delta)) > 0.f ? 1 : -1;
				FSoSplinePoint OldSplineLocation = SplineLocation;
				SplineLocation += Delta.Size2D() * DirModifier; /** (bMoveForwardOnSpline ? 1.f : -1.f)*/;

				if (Hit.bBlockingHit)
				{
					// hit second wall
					LastMoveTimeSlice = subTimeTickRemaining;
					subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

					SplineLocation = OldSplineLocation + (SplineLocation - OldSplineLocation) * Hit.Time;

					if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
					{
						remainingTime += subTimeTickRemaining;
						ProcessLanded(Hit, remainingTime, Iterations);
						return;
					}

					HandleImpact(Hit, LastMoveTimeSlice, Delta);

					// If we've changed physics mode, abort.
					if (!HasValidData() || !IsFalling())
					{
						return;
					}

					// Act as if there was no air control on the last move when computing new deflection.
					if (bHasAirControl && Hit.Normal.Z > VERTICAL_SLOPE_NORMAL_Z)
					{
						const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
						Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);
					}

					FVector PreTwoWallDelta = Delta;
					TwoWallAdjust(Delta, Hit, OldHitNormal);

					// Limit air control, but allow a slide along the second wall.
					if (bHasAirControl)
					{
						const bool bCheckLandingSpot = false; // we already checked above.
						const FVector AirControlDeltaV = LimitAirControl(subTimeTickRemaining, AirControlAccel, Hit, bCheckLandingSpot) * subTimeTickRemaining;

						// Only allow if not back in to first wall
						if (FVector::DotProduct(AirControlDeltaV, OldHitNormal) > 0.f)
						{
							Delta += (AirControlDeltaV * subTimeTickRemaining);
						}
					}

					// Compute velocity after deflection (only gravity component for RootMotion)
					if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
					{
						Velocity = Delta / subTimeTickRemaining;
					}

					const FVector NewImpactNormal = USoMathHelper::GetProjectedNormal(Delta, Hit.ImpactNormal);

					// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
					bool bDitch = ((OldHitImpactNormal.Z > 0.f) && (NewImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((NewImpactNormal | OldHitImpactNormal) < 0.f));
					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

					DirModifier = (FVector2D(SplineLocation.GetDirection()) | FVector2D(Delta)) > 0.f ? 1 : -1;
					OldSplineLocation = SplineLocation;
					SplineLocation += Delta.Size2D() * DirModifier;

					if (Hit.Time == 0.f)
					{
						SplineLocation = OldSplineLocation;

						// if we are stuck then try to side step
						FVector SideDelta = (OldHitNormal + NewImpactNormal).GetSafeNormal2D();
						if (SideDelta.IsNearlyZero())
						{
							SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).GetSafeNormal();
						}
						SafeMoveUpdatedComponent(SideDelta, PawnRotation, true, Hit);

						DirModifier = (FVector2D(SplineLocation.GetDirection()) | FVector2D(SideDelta)) > 0.f ? 1 : -1;
						SplineLocation += SideDelta.Size2D() * DirModifier * (Hit.bBlockingHit ? Hit.Time : 1.f);

					}

					if (bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0.f)
					{
						remainingTime = 0.f;
						ProcessLanded(Hit, remainingTime, Iterations);
						return;
					}
					if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= GetWalkableFloorZ())
					{
						// We might be in a virtual 'ditch' within our perch radius. This is rare.
						const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
						const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
						const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
						if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
						{
							Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
							Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
							Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
							Delta = Velocity * timeTick;
							SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

							DirModifier = (FVector2D(SplineLocation.GetDirection()) | FVector2D(Delta)) > 0.f ? 1 : -1;
							SplineLocation += Delta.Size2D() * DirModifier * (Hit.bBlockingHit ? Hit.Time : 1.f);
						}
					}
				}
			}
		}

		if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}


		// UE_LOG(LogTemp, Warning, TEXT("Spline Location thingy: %f"), SplineLocation.GetDistanceFromSplineStart());

	}

	// Root Motion has been used, clear - cause animation should not ruin the rotation, that's why
	if (!bAllowRootMotionRotation)
		RootMotionParams.Clear();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const USplineComponent* USoCharacterMovementComponent::GetActiveSplineComponent() const
{
	return SplineLocation.GetSpline()->GetSplineComponent();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoCharacterMovementComponent::ComputeFixedNormalForDelta(const FVector& Delta, const FVector& InNormal, const FHitResult& Hit)
{
	FVector Normal(InNormal);
	if (IsMovingOnGround())
	{
		// We don't want to be pushed up an unwalkable surface.
		if (Normal.Z > 0.f)
		{
			if (!IsWalkable(Hit))
			{
				Normal = Normal.GetSafeNormal2D();
			}
		}
		else if (Normal.Z < -KINDA_SMALL_NUMBER)
		{
			// Don't push down into the floor when the impact is on the upper portion of the capsule.
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				const FVector FloorNormal = CurrentFloor.HitResult.Normal;
				const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Z < 1.f - DELTA);
				if (bFloorOpposedToMovement)
				{
					Normal = FloorNormal;
				}

				Normal = Normal.GetSafeNormal2D();
			}
		}
	}
	return Normal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoCharacterMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact)
{
	if (Delta.IsNearlyZero())
	{
		return 0.0f;
	}
	const FVector SlideVector = ComputeSlideVector(Delta, Time, ComputeFixedNormalForDelta(Delta, Hit.Normal, Hit), Hit);

	if (SlideVector.IsNearlyZero())
	{
		return 0.0f;
	}

	const float ProjectionPercent = SlideVector.ProjectOnTo(Delta).Size2D() / Delta.Size2D();
	return Super::SlideAlongSurface(Delta, Time, Hit.Normal, Hit, true) * ProjectionPercent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::OnCharacterStuckInGeometry(const FHitResult* Hit)
{
	Super::OnCharacterStuckInGeometry(Hit);
	if (bWasPushedAndStuckLastFrame)
		StuckedAndShouldBeKilled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::UpdateBasedMovement(float DeltaSeconds)
{
	const FVector OldPostion = UpdatedComponent->GetComponentLocation();
	Super::UpdateBasedMovement(DeltaSeconds);

	const FVector NewPosition = UpdatedComponent->GetComponentLocation();
	FVector Delta = NewPosition - OldPostion;
	const float Offset = Delta.Size2D();

	if (Offset <= KINDA_SMALL_NUMBER) return;

	FVector PositiveDirection = SplineLocation.GetDirection();
	PositiveDirection.Normalize();
	Delta.Normalize();

	SplineLocation += FMath::Sign(FVector::DotProduct(Delta, PositiveDirection)) * Offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoCharacterMovementComponent::CalcAnimRootMotionVelocity(const FVector& RootMotionDeltaMove, float DeltaSeconds, const FVector& CurrentVelocity) const
{
	FVector Vel = DeltaSeconds > KINDA_SMALL_NUMBER ? RootMotionDeltaMove / DeltaSeconds * SoRootMotionDesc.Multiplier * SkeletonIsExportedTerriblyModifier : Velocity;
	FVector2D VelHorizontal = FVector2D(SplineLocation.GetDirection()) * Vel.Size2D();

	if (!SoRootMotionDesc.bVerticalRootMotionEnabled)
		Vel.Z = Velocity.Z;

	if (!SoRootMotionDesc.bHorizontalRootMotionEnabled)
	{
		const FVector TempVel = Velocity;
		// Apply acceleration
		// const float MaxSpeed = GetMaxSpeed();
		// const float NewMaxSpeed = (IsExceedingMaxSpeed(MaxSpeed)) ? TempVel.Size() : MaxSpeed;
		// TempVel += Acceleration * DeltaSeconds;
		// TempVel = TempVel.GetClampedToMaxSize(NewMaxSpeed);


		Vel.X = TempVel.X;
		Vel.Y = TempVel.Y;
	}
	return Vel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoCharacterMovementComponent::GetImpartedMovementBaseVelocity() const
{
	if (CharacterOwner)
	{
		if (UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase())
		{
			if (AActor* BaseActor = MovementBase->GetOwner())
			{
				static const FName ActorTagNoImpartedVel = FName("NoImpartedVel");
				if (BaseActor->ActorHasTag(ActorTagNoImpartedVel))
				{
					return FVector::ZeroVector;
				}
			}
		}
	}

	return Super::GetImpartedMovementBaseVelocity();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoCharacterMovementComponent::ConstrainLocationToPlane(FVector Location) const
{
	if (SplineLocation.GetSpline() != nullptr)
	{
		// the spline is horizontal, Z must not be projected
		const FVector L = SplineLocation.GetSpline()->GetSplineComponent()->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::World);
		return FVector(L.X, L.Y, Location.Z);
	}

	return Location;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SetFallVelocityOverride(const bool bInFallVelocityOverride, const float InFallVelocityOverride)
{
	bFallVelocityOverride = bInFallVelocityOverride;
	FallVelocityOverrideValue = InFallVelocityOverride;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::Dash(const FVector& Dir, const float Distance)
{
	const float SplineDelta = Dir.Size2D() * Distance;
	const float ZDelta = Dir.Z * Distance;
	SplineLocation += SplineDelta * SplineLocation.GetDirectionModifierFromVector(Dir);

	FVector Delta = FVector(SplineLocation) - UpdatedComponent->GetComponentLocation();
	Delta.Z = ZDelta;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::StartSwinging(const FVector& InSwingCenter, const float InSwingInputForce)
{
	MovementMode = EMovementMode::MOVE_Custom;
	CustomMovementMode = ECustomMovementMode::ECM_Swing;
	SwingCenter = InSwingCenter;
	SwingArmDelta = 0.0f;
	SwingInput = FVector(0.0f, 0.0f, 0.0f);
	SwingInputForce = InSwingInputForce;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCharacterMovementComponent::IsPointInSwingReach(const FVector& InSwingCenter) const
{
	return (InSwingCenter - CharacterOwner->GetActorLocation()).Size() < SwingArmMax;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::StartSliding(const FVector& SurfaceNormal, float SurfaceSlidingModifier)
{
	// TODO
	MovementMode = EMovementMode::MOVE_Custom;

	FVector Direction = SplineLocation.GetDirection();
	Direction.Z = 0;
	Direction.Normalize();

	if ((FVector2D(Direction) | FVector2D(SurfaceNormal)) <= 0)
	{
		Direction *= -1.f;
	}
	Velocity = Direction * MaxWalkSpeed * SurfaceSlidingModifier;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::AddOffsetOnSpline(float Offset, const FVector& Direction, bool bSweep /*= false*/, FHitResult* HitResult /*= nullptr*/)
{
	const FVector DirectionOnSpline = SplineLocation.GetDirection();
	Offset *= FMath::Sign(DirectionOnSpline | Direction);

	FHitResult Hit;
	UpdatedComponent->AddWorldOffset(DirectionOnSpline * Offset, bSweep, &Hit);
	if (bSweep)
	{
		if (HitResult) *HitResult = Hit;
		if (Hit.bBlockingHit) Offset = Offset * Hit.Time;
	}
	SplineLocation += Offset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::Push(const FVector2D& PushVelocity)
{
	const FVector Vel = SplineLocation.GetDirection() * PushVelocity.X;
	// Velocity += FVector(Vel.X, Vel.Y, PushedDesc.Velocity.Y); ??
	Velocity = FVector(Vel.X, Vel.Y, PushVelocity.Y);

	if (MovementMode == EMovementMode::MOVE_Walking)
	{
		if (Velocity.Z < -KINDA_SMALL_NUMBER)
		{
			float StepUpHitVelocity = 0.0f;
			float DefaultBounce = 0.0f;
			if (SoOwner->ShouldBounceOnHit(StepUpHitVelocity, DefaultBounce, nullptr))
				Velocity.Z *= -DefaultBounce;
		}

		if (Velocity.Z > KINDA_SMALL_NUMBER)
			SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::StuckedAndShouldBeKilled()
{
	if (LastPushRematerializeLocation != nullptr)
	{
		FSoHitReactDesc HitReact;
		HitReact.AssociatedActor = LastPushRematerializeLocation;
		HitReact.HitReact = ESoHitReactType::EHR_BreakIntoPieces;
		ISoMortal::Execute_CauseDmg(GetOwner(), FSoDmg{ LastPushCrushDamage * 10.0f, 0.0f }, HitReact);
	}
	else // RIP
		ISoMortal::Execute_Kill(GetOwner(), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void USoCharacterMovementComponent::StartControlledMovementBasedOnOffset(const FSoControlledMovementDesc& MovementDesc)
//{
//	if (bControlledMovement) return;
//
//	bControlledMovement = true;
//	ControlledMovementDesc = MovementDesc;
//
//	MovementMode = EMovementMode::MOVE_Custom;
//	CustomMovementMode = ECustomMovementMode::ECM_Controlled;
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SetPushedAndStuck(AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage)
{
	bWasPushedAndStuckLastFrame = true;
	LastPushRematerializeLocation = RematerializeLocation;
	LastPushCrushDamage = DamageAmountIfStuck;
	if (bForceDamage)
		StuckedAndShouldBeKilled();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::ForceUpdateFloor()
{
	// check floor again
	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);

	if (AActor* Floor = CurrentFloor.HitResult.GetActor())
	{
		const float FloorZ = Floor->GetVelocity().Z;
		if (FloorZ > Velocity.Z)
		{
			AdjustFloorHeight();
			SetBaseFromFloor(CurrentFloor);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoCharacterMovementComponent::TryLateWallJump()
{
	if (!bAllowLateWallJump)
		return false;

	const float TimeSinceMissedWallJump = GetWorld()->GetTimeSeconds() - MissedWallJumpTime;
	if (TimeSinceMissedWallJump < GetLateWallJumpInterval())
	{
		Velocity = MissedWallJumpVelocity;
		SoOwner->OnBounce(true, MissedRollStoredValue, MissedImpactPointOffset + UpdatedComponent->GetComponentLocation(), MissedImpactNormal);
		MissedWallJumpTime = 0.0f;
		// UE_LOG(LogTemp, Warning, TEXT("Late WallJump!"));
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::ToggleAllowLateWallJump()
{
	bAllowLateWallJump = !bAllowLateWallJump;
	if (bAllowLateWallJump)
		UE_LOG(LogTemp, Warning, TEXT("Late WallJump Enabled"))
	else
		UE_LOG(LogTemp, Warning, TEXT("Late WallJump Disabled"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCharacterMovementComponent::SetLateWallJumpInterval(float Time)
{
	LateWallJumpInterval = Time;
	UE_LOG(LogTemp, Warning, TEXT("LateWallJumpInterval set to %f"), Time);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoCharacterMovementComponent::GetLateWallJumpInterval() const
{
	return USoDateTimeHelper::NormalizeTime(LateWallJumpInterval);
}
