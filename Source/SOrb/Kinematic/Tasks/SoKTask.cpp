// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoKTask.h"

#include "DestructibleComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Components/PrimitiveComponent.h"
#include "Curves/CurveFloat.h"
#include "CollisionQueryParams.h"
#include "SplineLogic/SoSplineHelper.h"
#include "SplineLogic/SoSplineWalker.h"
#include "Kinematic/SoKProcess.h"
#include "CharacterBase/SoMortal.h"
#include "Basic/SoAudioManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTask::Initialize(USceneComponent* Target, AActor* InRematerializeLocation, int32 InCrushDamage)
{
	RematerializeLocation = InRematerializeLocation;
	CrushDamage = InCrushDamage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoTaskExecutionResults USoKTWait::Execute(float DeltaSeconds, bool bForward)
{
	CurrentTime += DeltaSeconds;
	return {CurrentTime - TimeToWait};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTCallFunction::Initialize(USceneComponent* Target, AActor* InRematerializeLocation, int32 InCrushDamage)
{
	if (AActor* OwnerActor = Target->GetOwner())
	{
		if (FunctionNameForward != NAME_None)
			BindDelegate(OwnerActor, FunctionNameForward, ForwardFunction);
		if (FunctionNameBackwards != NAME_None)
			BindDelegate(OwnerActor, FunctionNameBackwards, BackwardsFunction);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTCallFunction::BindDelegate(AActor* OwnerActor, FName FunctionName, FSoKNotify& FunctionDelegate)
{
	if (UFunction* Func = OwnerActor->FindFunction(FunctionName))
	{
		if (Func->ParmsSize > 0)
		{
			UE_LOG(LogSoKinematicSystem, Warning, TEXT("USoKTCallFunction inside %s has a function (%s) that expects parameters."), *OwnerActor->GetName(), *FunctionName.ToString());
		}
		else
		{
			FunctionDelegate.BindUFunction(OwnerActor, FunctionName);
		}
	}
	else
	{
		UE_LOG(LogSoKinematicSystem, Error, TEXT("USoKTCallFunction inside %s does NOT have function with name = %s"), *OwnerActor->GetName(), *FunctionName.ToString());
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTCallFunction::Start(USceneComponent* Target, bool bForward)
{
	if (bForward)
		ForwardFunction.ExecuteIfBound();
	else
		BackwardsFunction.ExecuteIfBound();
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTActivateComponent::Start(USceneComponent* Target, bool bForward)
{
	if (bForward)
	{
		Target->Activate(bReset);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTPlaySound::Start(USceneComponent* Target, bool bForward)
{
	if (bForward)
	{
		if (bAttached)
			USoAudioManager::PlaySoundAttached(SFX, Target, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget);
		else
			USoAudioManager::PlaySoundAtLocation(Target->GetOwner(), SFX, Target->GetComponentTransform());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTPlaySoundForReversePlay::Start(USceneComponent* Target, bool bForward)
{
	if (!bForward)
	{
		if (bAttached)
			USoAudioManager::PlaySoundAttached(SFX, Target, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget);
		else
			USoAudioManager::PlaySoundAtLocation(Target->GetOwner(), SFX, Target->GetComponentTransform());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTPlayLoopedSound::Start(USceneComponent* Target, bool bForward)
{
	if (bForward)
	{
		if (FMODComponent == nullptr)
			FMODComponent = USoAudioManager::PlaySoundAttached(SFX, Target, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget);
		else
			USoAudioManager::PlayComponentIfNotPlaying(FMODComponent);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTPlayLoopedSound::StartStopSFX(bool bStop)
{
	if (FMODComponent != nullptr)
	{
		if (bStop)
			FMODComponent->Stop();
		else
			FMODComponent->Play();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTTimeBasedTransform::Start(USceneComponent* Target, bool bForward)
{
	CurrentTime = bForward ? 0.0f : TimeInSec;
	TargetComponent = Target;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoKTTimeBasedTransform::GetOffset(float StartPercent, float EndPercent) const
{
	UE_LOG(LogSoKinematicSystem, Error, TEXT("USoKTimeBasedTransformer::GetOffset() called, but it should be overriden and should not be called directly!"));
	return FVector(0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoTaskExecutionResults USoKTTimeBasedTransform::Execute(float DeltaSeconds, bool bForward)
{
	const float LastTime = CurrentTime;
	float RestTime = UpdateCurrentTime(TimeInSec, DeltaSeconds * (bForward ? 1 : -1), CurrentTime);

	// calc offset based on time
	const float ActualPercent = CurrentTime / TimeInSec;
	const float LastPercent = LastTime / TimeInSec;

	// get offset from child class
	const FVector Offset = GetOffset(LastPercent, ActualPercent);

	if (IsRotation())
	{
		AddDeltaRotation(Offset, IsLocalAxis(), CollisionType, DeltaSeconds);
		return { RestTime };
	}

	const float AppliedPercent = AddDeltaOffset(Offset, IsLocalAxis(), CollisionType, DeltaSeconds);
	bool bChangeDirection = false;
	const bool bNotFinished = (AppliedPercent < 1.0f - KINDA_SMALL_NUMBER);
	if (bNotFinished)
	{
		CurrentTime = LastTime + (CurrentTime - LastTime) * AppliedPercent;
		switch (CollisionType)
		{
			case ESoTaskCollisionResponse::ETCR_TurnBack:
				bChangeDirection = true;
				break;

			case ESoTaskCollisionResponse::ETCR_Skip:
				RestTime = KINDA_SMALL_NUMBER * 2.0f;
				break;

			default:
				break;
		}
	}

	// notify child class
	OnMoved(Offset, AppliedPercent);

	return { RestTime, bChangeDirection };
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoKTTransformBasedOnOffsetCurve* USoKTTransformBasedOnOffsetCurve::ConstructTransformBasedOnOffsetCurve(UObject* WorldContextObject,
																										 float InTimeInSec,
																										 UCurveFloat* InCurve,
																										 const FVector& InOffset,
																										 bool bInRotation,
																										 ESoTaskCollisionResponse InCollisionType)
{
	USoKTTransformBasedOnOffsetCurve* Task = NewObject<USoKTTransformBasedOnOffsetCurve>();

	Task->CollisionType = InCollisionType;
	Task->TimeInSec = InTimeInSec;
	Task->OffsetCurve = InCurve;
	Task->Offset = InOffset;
	Task->bRotation = bInRotation;

	return Task;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoKTTransformBasedOnOffsetCurve::GetOffset(float StartPercent, float EndPercent) const
{
	if (OffsetCurve == nullptr)
	{
		UE_LOG(LogSoKinematicSystem, Error, TEXT("Curve based offset kinmematic task without curve!"));
		return FVector(0.0f, 0.0f, 0.0f);
	}

	const FVector ValueBefore = Offset * OffsetCurve->GetFloatValue(StartPercent);
	const FVector ValueAfter = Offset * OffsetCurve->GetFloatValue(EndPercent);
	return ValueAfter - ValueBefore;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTTransformBasedOnOffset::Initialize(USceneComponent* Target, AActor* InRematerializeLocation, int32 InCrushDamage)
{
	if (Target != nullptr)
		SavedStartPos = Target->GetComponentLocation();
	FullTime = TimeInSec;

	RealOffset = Offset;

	Super::Initialize(Target, InRematerializeLocation, InCrushDamage);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTTransformBasedOnOffset::Start(USceneComponent* Target, bool bForward)
{
	if (bAlwaysRelativeToStartPosition)
	{
		const FVector TargetPos = SavedStartPos + Offset;
		const FVector StartPos = Target->GetComponentLocation();

		const float Distance = (StartPos - TargetPos).Size();
		const float NormalDistance = Offset.Size();

		TimeInSec = FullTime * Distance / NormalDistance;
		RealOffset = TargetPos - StartPos;
	}

	// has to be called after bAlwaysRelativeToStartPosition setup is finished cause it needs the modified TimeInSec
	Super::Start(Target, bForward);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoKTTransformBasedOnOffset::GetOffset(float StartPercent, float EndPercent) const
{
	return RealOffset * (EndPercent - StartPercent);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoKTTransformBasedOnVelocity::GetOffset(float StartPercent, float EndPercent) const
{
	return Velocity * (EndPercent - StartPercent) * TimeInSec;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool USoKTask::ShouldUseSweep(ESoTaskCollisionResponse CollisionType) const
{
	return CollisionType != ESoTaskCollisionResponse::ETCR_Ignore;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTask::AddDeltaRotation(const FVector& InDeltaRot, bool bLocal, ESoTaskCollisionResponse CollisionType, float DeltaSeconds)
{
	const FQuat OldTargetQuat = TargetComponent->GetComponentQuat();
	const FVector OldTargetLocation = TargetComponent->GetComponentLocation();

	if (bLocal)
		TargetComponent->AddLocalRotation(FRotator(InDeltaRot.Y, InDeltaRot.Z, InDeltaRot.X));
	else
		TargetComponent->AddWorldRotation(FRotator(InDeltaRot.Y, InDeltaRot.Z, InDeltaRot.X));

#if WITH_EDITOR
	if (CollisionType == ESoTaskCollisionResponse::ETCR_Notify ||
		CollisionType == ESoTaskCollisionResponse::ETCR_Wait ||
		CollisionType == ESoTaskCollisionResponse::ETCR_TurnBack ||
		CollisionType == ESoTaskCollisionResponse::ETCR_PushOrWait)
		UE_LOG(LogSoKinematicSystem, Error, TEXT("USoKTask::AddDeltaRotation: invalid collision response! Rotation supports Ignore and Push only atm!"));
#endif

	if (CollisionType != ESoTaskCollisionResponse::ETCR_Push && CollisionType != ESoTaskCollisionResponse::ETCR_PushAggressive)
		return;

	UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(TargetComponent);

	if (Target == nullptr) // should not happen
		return;

	const FQuat NewTargetQuat = TargetComponent->GetComponentQuat();
	const FVector NewTargetLocation = TargetComponent->GetComponentLocation();
	const FQuatRotationTranslationMatrix OldTargetLocalToWorld(OldTargetQuat, OldTargetLocation);
	const FQuatRotationTranslationMatrix NewTargetLocalToWorld(NewTargetQuat, NewTargetLocation);

	const ECollisionChannel TargetCollisionChannel = Target->GetCollisionObjectType();

	TSet<AActor*> ActorsToPush;
	//TArray<UPrimitiveComponent*> OverlapingComponents;
	// Target->UpdateOverlaps(nullptr, false, nullptr);
	// Target->GetOverlappingComponents(OverlapingComponents);

	TArray<FOverlapResult> OutOverlaps;
	FComponentQueryParams Params;
	Params.MobilityType = EQueryMobilityType::Dynamic;

	FCollisionObjectQueryParams ObjectQueryParams;
	const auto& Responses = Target->GetCollisionResponseToChannels();
	for (int32 i = 0; i < ECollisionChannel::ECC_MAX; ++i)
		if (Responses.GetResponse(static_cast<ECollisionChannel>(i)) == ECollisionResponse::ECR_Block)
			ObjectQueryParams.AddObjectTypesToQuery(static_cast<ECollisionChannel>(i));

	Target->ComponentOverlapMulti(OutOverlaps, Target->GetOwner()->GetWorld(), Target->GetComponentLocation(), Target->GetComponentRotation(), Target->GetCollisionObjectType(), Params, ObjectQueryParams);

	// gather actors to push
	for (const auto& OverlapResult : OutOverlaps)
	{
		auto* Component = OverlapResult.GetComponent();
		if (Component == nullptr)
			continue;

		// have to check both way, because the weaker collision response should be considered
		const ECollisionResponse Resp1 = Component->GetCollisionResponseToChannel(TargetCollisionChannel);
		const ECollisionResponse Resp2 = TargetComponent->GetCollisionResponseToChannel(Component->GetCollisionObjectType());

		if (Resp1 == ECollisionResponse::ECR_Block && Resp2 == ECollisionResponse::ECR_Block)
			ActorsToPush.Add(Component->GetOwner());
		// maybe trigger overlap events otherwise? - we can add it later if we need it
		// also maybe triggering hit events here would be cool too, they are triggered when the actor is moved back, maybe it can cause some trouble?!
		// but then they would be triggered twice, so I am not sure
	}

	for (auto* Actor : ActorsToPush)
	{
		const FVector LocalBasePos = OldTargetLocalToWorld.InverseTransformPosition(Actor->GetActorLocation());
		const FVector NewWorldPos = NewTargetLocalToWorld.TransformPosition(LocalBasePos);
		const FVector DeltaPosition = (NewWorldPos - Actor->GetActorLocation());

		OnPushObject(Actor, DeltaPosition, DeltaSeconds, true, CollisionType == ESoTaskCollisionResponse::ETCR_PushAggressive);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoKTask::AddDeltaOffset(const FVector& InDeltaOffset, bool bLocal, ESoTaskCollisionResponse CollisionType, float DeltaSeconds)
{
	const bool bSweep = ShouldUseSweep(CollisionType);
	FHitResult HitResult;

	if (bLocal)
		TargetComponent->AddLocalOffset(InDeltaOffset, bSweep, &HitResult);
	else
		TargetComponent->AddWorldOffset(InDeltaOffset, bSweep, &HitResult);

	// react collision
	if (bSweep && HitResult.bBlockingHit)
	{
		switch (CollisionType)
		{
			case ESoTaskCollisionResponse::ETCR_Ignore:
				check(false);
				break;
			case ESoTaskCollisionResponse::ETCR_Wait:
			case ESoTaskCollisionResponse::ETCR_TurnBack:
			case ESoTaskCollisionResponse::ETCR_Skip:
				return HitResult.Time;
			case ESoTaskCollisionResponse::ETCR_Notify:
			{
				// atm only the first collision is handled
				// maybe all could be handled properly - but how?
				const FVector RestOffset = InDeltaOffset * (1.0f - HitResult.Time);
				if (bLocal)
					TargetComponent->AddLocalOffset(RestOffset);
				else
					TargetComponent->AddWorldOffset(RestOffset);

				return 1.0f;
			}

			case ESoTaskCollisionResponse::ETCR_PushOrWait:
			{
				UClass* HitClass = HitResult.GetActor()->GetClass();
				if ((!HitClass->ImplementsInterface(USoSplineWalker::StaticClass()) ||
					!HitClass->ImplementsInterface(USoMortal::StaticClass())) &&
					Cast<UDestructibleComponent>(HitResult.GetComponent()) == nullptr &&
					HitResult.GetComponent()->GetCollisionObjectType() != ECollisionChannel::ECC_PhysicsBody)
					return HitResult.Time;
			}
			// no break for a reason!!
			case ESoTaskCollisionResponse::ETCR_Push:
			case ESoTaskCollisionResponse::ETCR_PushAggressive:
			{
				FVector RestWorldOffset;

				if (bLocal)
				{
					// has to remove scale from the transform
					FTransform Transform = TargetComponent->GetComponentTransform();
					Transform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
					RestWorldOffset = Transform.TransformVector(InDeltaOffset);
				}
				else
					RestWorldOffset = InDeltaOffset;

				TArray<AActor*> ActorsAlreadyHit;

				const float SumLength = RestWorldOffset.Size();

				while (HitResult.bBlockingHit && !ActorsAlreadyHit.Contains(HitResult.GetActor()) && RestWorldOffset.SizeSquared() > KINDA_SMALL_NUMBER)
				{
					if (HitResult.GetActor() && HitResult.GetActor()->GetRootComponent()->Mobility != EComponentMobility::Movable)
						break; // can't have proper collision, just skip everything


					OnPushObject(HitResult.GetActor(), RestWorldOffset * (1.0f - HitResult.Time), DeltaSeconds, false, CollisionType == ESoTaskCollisionResponse::ETCR_PushAggressive);

					RestWorldOffset *= (1.0f - HitResult.Time);
					ActorsAlreadyHit.Add(HitResult.GetActor());

					TargetComponent->AddWorldOffset(RestWorldOffset, true, &HitResult);
				}
				// could not push everything out of the way, but we don't care
				RestWorldOffset *= (1.0f - HitResult.Time);
				if (RestWorldOffset.Size() > KINDA_SMALL_NUMBER)
					TargetComponent->AddWorldOffset(RestWorldOffset);

				// TODO: maybe after it is moved, it should check all overlapping actors the way rotation does, for a final correction, if there was a blocking hit which was not resolved

				return 1.0f;
			}
			default:
				check(false);
				break;
		}
	}
	return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTask::OnPushObject(AActor* ActorToPush, const FVector& DeltaMovement, float DeltaSeconds, bool bTeleportAndSweepBack, bool bForceDamage) const
{
	if (ActorToPush == nullptr)
		return;

	ISoSplineWalker* SplineWalker = Cast<ISoSplineWalker>(ActorToPush);
	// try to handle it as spline object, but maybe it's e.g. a bullet with invalid spline location - then normal push
	// FIXMYSELF: bullets shouldn't even be considered, should they? :O
	bool bHandledAsSplineWalker = false;
	if (SplineWalker != nullptr)
	{
		// push along spline
		FHitResult HitResult;
		bHandledAsSplineWalker = USoSplineHelper::PushSplineWalker(ActorToPush,
																   DeltaMovement,
																   DeltaSeconds,
																   bTeleportAndSweepBack ? nullptr : &HitResult,
																   RematerializeLocation,
																   CrushDamage, bForceDamage);

		if (bTeleportAndSweepBack && bHandledAsSplineWalker)
		{
			USoSplineHelper::PushSplineWalker(ActorToPush, -DeltaMovement, DeltaSeconds, &HitResult, RematerializeLocation, CrushDamage, bForceDamage);
			const int32 MaxIterNum = 6;
			int32 IterNum = 0;
			while (HitResult.bBlockingHit && HitResult.Time < KINDA_SMALL_NUMBER && HitResult.GetComponent() == TargetComponent && IterNum < MaxIterNum)
			{
				// some more so we don't get stuck
				USoSplineHelper::PushSplineWalker(ActorToPush, DeltaMovement, DeltaSeconds, nullptr, RematerializeLocation, CrushDamage, bForceDamage);
				USoSplineHelper::PushSplineWalker(ActorToPush, -DeltaMovement, DeltaSeconds, &HitResult, RematerializeLocation, CrushDamage, bForceDamage);
				IterNum += 1;
			}
			// UE_LOG(LogSoKinematicSystem, Display, TEXT("Iter num: %d"), IterNum);
		}
	}

	if (!bHandledAsSplineWalker)
	{
		// simple case
		ActorToPush->AddActorWorldOffset(DeltaMovement, !bTeleportAndSweepBack);
		if (bTeleportAndSweepBack)
			ActorToPush->AddActorWorldOffset(-DeltaMovement, true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoKTask::UpdateCurrentTime(float MaxCurrentTime, float DeltaSeconds, float& CurrentTime) const
{
	// update current time
	const float UnClampedNewTime = CurrentTime + DeltaSeconds;
	CurrentTime = UnClampedNewTime;
	// rest time means "unused time" in this context: if we reached our goals in less than DeltaSeconds, we give the "not used" time back for the next task
	float RestTime = -1.0f;
	if (UnClampedNewTime < 0.0f)
	{
		CurrentTime = 0.0f;
		RestTime = -UnClampedNewTime;
	}
	else if (UnClampedNewTime > MaxCurrentTime)
	{
		CurrentTime = MaxCurrentTime;
		RestTime = UnClampedNewTime - MaxCurrentTime;
	}

	return RestTime;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTTranslateOnSpline::Start(USceneComponent* Target, bool bForward)
{
	Super::Start(Target, bForward);

	// setup spline location
	SplineLocation.SetSpline(SplinePtr.Get());
	SplineLocation.SetDistanceFromWorldLocation(Target->GetComponentLocation());

	if (bFreezeRotationRelativeToSplineDirection)
		RotationDelta = Target->GetComponentRotation() - SplineLocation.GetDirectionFromVector(Target->GetForwardVector()).Rotation();

	if (bForceDistanceFromSpline)
	{
		DistanceFromSplinePoint = Target->GetComponentLocation() - FVector(SplineLocation);
		DistanceFromSplinePoint.Z = 0.0f;

		DistanceFromSplinePoint = Target->GetComponentTransform().InverseTransformVector(DistanceFromSplinePoint);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTTranslateOnSpline::Start(USceneComponent* Target, const bool bForward, const FSoSplinePoint& SplinePoint)
{
	Super::Start(Target, bForward);
	SplineLocation = SplinePoint;

	if (bFreezeRotationRelativeToSplineDirection)
		RotationDelta = Target->GetComponentRotation() - SplineLocation.GetDirectionFromVector(Target->GetForwardVector()).Rotation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoKTTranslateOnSpline::GetOffset(float StartPercent, float EndPercent) const
{
	if (!SplineLocation.IsValid())
		return FVector(0, 0, 0);

	const float Multiplier = (EndPercent - StartPercent) * (bVelocityBased ? TimeInSec : 1.0f);

	const FVector OldLocation = SplineLocation;
	const FVector NewLocation = (SplineLocation + Value.X * Multiplier);
	const FVector DeltaLocation = NewLocation - OldLocation;

	return FVector(DeltaLocation.X, DeltaLocation.Y, Value.Y * Multiplier);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoKTTranslateOnSpline::OnMoved(const FVector& Offset, float AppliedPercent)
{
	// TODO: better splinelocation update if AppliedPercent < 1 (if it is necessary)
	SplineLocation += Offset.Size2D() * AppliedPercent * SplineLocation.GetDirectionModifierFromVector(Offset);

	if (bFreezeRotationRelativeToSplineDirection && TargetComponent != nullptr)
		TargetComponent->SetWorldRotation(SplineLocation.GetDirectionFromVector(TargetComponent->GetForwardVector()).Rotation() + RotationDelta);

	if (bForceDistanceFromSpline && TargetComponent != nullptr)
	{
		const FVector WorldDelta = TargetComponent->GetComponentTransform().TransformVector(DistanceFromSplinePoint);
		const float ZValue = TargetComponent->GetComponentLocation().Z;
		const FVector NewLocation = FVector(SplineLocation) + WorldDelta;
		TargetComponent->SetWorldLocation(FVector(NewLocation.X, NewLocation.Y, ZValue));
	}
}
