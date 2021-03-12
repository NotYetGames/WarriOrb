// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

#include "CharacterBase/SoMortal.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoProjectileMovementComponent.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoAudioManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile::ASoProjectile()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::OnConstruction(const FTransform& Transform)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::BeginPlay()
{
	Super::BeginPlay();
	HitReactDesc.AssociatedActor = this;

	if (PrimitiveComponent != nullptr)
	{
		PrimitiveComponent->OnComponentBeginOverlap.AddDynamic(this, &ASoProjectile::OnOverlapBegin);
		PrimitiveComponent->OnComponentHit.AddDynamic(this, &ASoProjectile::OnHitCallback);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::ActivateProjectile(const FVector& InitialLocation,
									   const FRotator& InitialRotation,
									   const FSoProjectileInitData* InitDataOverride,
									   const FSoProjectileRuntimeData* RuntimeData,
									   AActor* Spawner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ActivateProjectile"), STAT_ActivateProjectile, STATGROUP_SoProjectile);

	SetActorLocationAndRotation(InitialLocation + FVector(0.0f, 0.0f, ZSpawnOffset), InitialRotation);
	const FSoProjectileInitData& InitDataRef = InitDataOverride == nullptr ? DefaultInitData : *InitDataOverride;
	ActivateProjectile_Internal(InitDataRef, RuntimeData == nullptr ? DefaultRuntimeData : *RuntimeData, Spawner);

	if (ProjectileMovementComponent != nullptr)
	{
		if (InitDataRef.bShouldVelocityInfluenceDirection)
		{
			FVector ForwardVector = InitialRotation.Vector();
			ForwardVector.Z = 0.0f;
			ForwardVector.Normalize();
			ProjectileMovementComponent->Velocity = ForwardVector * InitDataRef.Velocity.X + FVector(0.0f, 0.0f, InitDataRef.Velocity.Y);
		}
		else
			ProjectileMovementComponent->Velocity = GetActorForwardVector() * InitDataRef.Velocity.X;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::ActivateProjectileOnSpline(const struct FSoSplinePoint& SplineLocation,
											   float LocationZ,
											   const FVector& OffsetFromSpline,
											   const FRotator& InitialRotation,
											   const FSoProjectileInitData* InitDataOverride,
											   const FSoProjectileRuntimeData* RuntimeData,
											   AActor* Spawner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ActivateProjectileOnSpline"), STAT_ActivateProjectileOnSpline, STATGROUP_SoProjectile);

	USoProjectileMovementComponent* SoProjectileMovement = Cast<USoProjectileMovementComponent>(ProjectileMovementComponent);
	if (SoProjectileMovement == nullptr || !SplineLocation.IsValid(false))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to fire projectile %s: projectile does not support splines or the spline parameters are invalid!"), *GetClass()->GetName());
		return;
	}

	FVector Location = SplineLocation;
	Location.Z = LocationZ;
	SetActorLocationAndRotation(Location + OffsetFromSpline + FVector(0.0f, 0.0f, ZSpawnOffset), InitialRotation);

	SoProjectileMovement->SetSplineLocation(SplineLocation);
	SoProjectileMovement->SetOffsetFromSplineLocation(OffsetFromSpline);

	const FSoProjectileInitData& InitDataRef = InitDataOverride == nullptr ? DefaultInitData : *InitDataOverride;
	ActivateProjectile_Internal(InitDataRef, RuntimeData == nullptr ? DefaultRuntimeData : *RuntimeData, Spawner);

	if (InitDataRef.bShouldVelocityInfluenceDirection)
		ProjectileMovementComponent->Velocity = GetActorForwardVector() * InitDataRef.Velocity.X + FVector(0.0f, 0.0f, InitDataRef.Velocity.Y);
	else
	{
		const FVector ForwardVector = SplineLocation.GetDirectionFromVector(InitialRotation.Vector());
		ProjectileMovementComponent->Velocity = ForwardVector * InitDataRef.Velocity.X + FVector(0.0f, 0.0f, InitDataRef.Velocity.Y);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::ForceVelocityToZ()
{
	if (ProjectileMovementComponent != nullptr)
		ProjectileMovementComponent->Velocity = FVector(0.0f, 0.0f, 1.0f) * ProjectileMovementComponent->Velocity.Size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::ActivateProjectile_Internal(const FSoProjectileInitData& InitData,
												const FSoProjectileRuntimeData& RuntimeData,
												AActor* Spawner)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("ActivateProjectile_Internal"), STAT_ActivateProjectile_Internal, STATGROUP_SoProjectile);

	// override some data
	RuntimeDataPtr = &RuntimeData;
	SpawnerActor = Spawner;
	RemainingHitCount = InitData.HitCountBeforeDestroyed;
	const bool OptionList[] = { bKillProjectileOnTargetHit, true, false };
	bCurrentKillProjectileOnTargetHit = OptionList[static_cast<int32>(InitData.TargetHitBehaviourOverride)];
	ActorsAlreadyHit.Empty();

	if (ProjectileMovementComponent != nullptr)
	{
		ProjectileMovementComponent->ProjectileGravityScale = InitData.GravityScale;
		if (InitData.bSetVelocityAsMaxSpeed)
			ProjectileMovementComponent->MaxSpeed = InitData.Velocity.Size();
	}

	// Setup max lifetime
	GetWorld()->GetTimerManager().SetTimer(LifeTimer, this, &ASoProjectile::DestroyProjectile, InitData.MaxLifeTime, false);

	// Spawner immunity timer
	if (InitData.SpawnerProtectionTime > KINDA_SMALL_NUMBER)
	{
		bDoNotDamageSpawner = true;
		FTimerDelegate TimerCallback;
		TimerCallback.BindLambda([&] { bDoNotDamageSpawner = false; });
		GetWorld()->GetTimerManager().SetTimer(SpawnerImmunityTimer, TimerCallback, InitData.SpawnerProtectionTime, false);
	}
	else
		bDoNotDamageSpawner = false;

	SetActorScale3D(InitData.Scale);

	// activate components
	if (bEnableMovementOnActivate && ProjectileMovementComponent != nullptr)
	{
		if (PrimitiveComponent != nullptr)
			ProjectileMovementComponent->SetUpdatedComponent(PrimitiveComponent);
		ProjectileMovementComponent->SetComponentTickEnabled(true);
	}
	if (PrimitiveComponent != nullptr)
	{
		PrimitiveComponent->SetVisibility(true, true);
		if (bEnableCollisionOnActivate)
			PrimitiveComponent->SetCollisionEnabled(PrimitiveComponentCollisionType);
	}

	// reset dmg in case it was modified
	if (ASoProjectile* Proj = Cast<ASoProjectile>(GetClass()->GetDefaultObject()))
		DefaultRuntimeData.Damage = Proj->DefaultRuntimeData.Damage;

	OnProjectileActivatedBP();

	bCollisionActiveOnPrimitiveComponent = true;
	if (CollisionActiveDistanceFromPlayerSquered > 0.0f && CollisionRefreshFrequency > 0.0f)
	{
		UpdateCollision();
		GetWorld()->GetTimerManager().SetTimer(CollisionRefreshTimer, this, &ASoProjectile::UpdateCollision, CollisionRefreshFrequency, true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::DeactivateProjectile()
{
	OnProjectileDeactivatedBP();

	RuntimeDataPtr = nullptr;
	SpawnerActor = nullptr;

	GetWorld()->GetTimerManager().ClearTimer(LifeTimer);
	GetWorld()->GetTimerManager().ClearTimer(SpawnerImmunityTimer);
	GetWorld()->GetTimerManager().ClearTimer(CollisionRefreshTimer);

	if (ProjectileMovementComponent != nullptr)
		ProjectileMovementComponent->SetComponentTickEnabled(false);

	if (PrimitiveComponent != nullptr)
	{
		PrimitiveComponent->SetVisibility(false, true);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// to avoid 1 frame glitches on next activation?!
	SetActorLocation(FVector(0.0f, 0.0f, 0.0f));

	SFXOverride = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoProjectile::IsSplineProjectile() const
{
	return Cast<USoProjectileMovementComponent>(ProjectileMovementComponent) != nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::DestroyProjectile_Implementation()
{
	OnProjectileDestroyed();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::OnDestroyStartDisableTimers()
{
	GetWorld()->GetTimerManager().ClearTimer(LifeTimer);
	GetWorld()->GetTimerManager().ClearTimer(SpawnerImmunityTimer);
	GetWorld()->GetTimerManager().ClearTimer(CollisionRefreshTimer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::OnProjectileDestroyed()
{
	DeactivateProjectile();
	OnDeathNotify.Broadcast(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::OnTargetHit_Implementation(AActor* OtherActor, const FHitResult& SweepResult)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnTargetHit_Implementation"), STAT_OnTargetHit_Implementation, STATGROUP_SoProjectile);

	if (RuntimeDataPtr != nullptr && !ActorsAlreadyHit.Contains(OtherActor))
	{
		HitReactDesc.HitReact = RuntimeDataPtr->HitReactType;
		HitReactDesc.AssociatedActor = RuntimeDataPtr->AssociatedActor == nullptr ? this : RuntimeDataPtr->AssociatedActor;
		HitReactDesc.Irresistibility = RuntimeDataPtr->Irresistibility;
		HitReactDesc.AssociatedBones = { SweepResult.BoneName };

		OnTargetHitReallyHappened(OtherActor, SweepResult);

		if (VFXOnTargetHit != nullptr)
		{
			const FVector MyActorLocation = GetActorLocation();
			FVector SpawnLocation = MyActorLocation;

			if (ACharacter* Character = Cast<ACharacter>(OtherActor))
			{
				USkeletalMeshComponent* SkeletalMesh = Character->GetMesh();
				FHitResult Hit;
				if (SkeletalMesh->LineTraceComponent(Hit, MyActorLocation, Character->GetActorLocation(), FCollisionQueryParams::DefaultQueryParam))
				{
					if (Hit.BoneName != NAME_None)
					{
						FVector ImpactPoint;
						if (SkeletalMesh->GetClosestPointOnCollision(MyActorLocation, ImpactPoint, Hit.BoneName) > 0.0f)
							SpawnLocation = ImpactPoint;

						if (bAttachImpactToTarget)
							UGameplayStatics::SpawnEmitterAttached(VFXOnTargetHit, SkeletalMesh, Hit.BoneName, SpawnLocation, FRotator::ZeroRotator, EAttachLocation::KeepWorldPosition);
					}
				}
			}

			if (!bAttachImpactToTarget)
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), VFXOnTargetHit, SpawnLocation);
		}

		if (EffectToApplyOnTarget != ESoVisualEffect::EVE_MAX)
			ISoMortal::Execute_DisplayVisualEffect(OtherActor, EffectToApplyOnTarget);

		// Reduce friendly fire effect
		FSoDmg Dmg = bCanUseDamageOverride ? RuntimeDataPtr->Damage : DefaultRuntimeData.Damage;
		if (bReduceDmgAgainstNotPlayer && USoStaticHelper::GetPlayerCharacterAsActor(this) != OtherActor)
			Dmg *= 0.1f;

		ISoMortal::Execute_CauseDmg(OtherActor, Dmg, HitReactDesc);

		USoAudioManager::PlayHitReactSFX(
			this,
			OtherActor,
			GetActorTransform(),
			HitReactDesc,
			SFXOnTargetHit,
			SFXOnEnemyTargetHit2D,
			SFXOnTargetDeath,
			SFXOnEnemyTargetHit
		);

		if (bCurrentKillProjectileOnTargetHit)
			DestroyProjectile();
		else if (bHitOneActorOnlyOnce)
			ActorsAlreadyHit.Add(OtherActor);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoProjectile::HasDamage() const
{
	return (RuntimeDataPtr != nullptr &&
		(RuntimeDataPtr->Damage.Physical > 0.0f || RuntimeDataPtr->Damage.Magical > 0.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::ForceUpdate_Implementation(float DeltaTime)
{
	if (ProjectileMovementComponent != nullptr)
	{
		const float MaxDeltaPerCall = 0.2f;
		for (int32 i = 0; i < DeltaTime / MaxDeltaPerCall; ++i)
			ProjectileMovementComponent->TickComponent(MaxDeltaPerCall, ELevelTick::LEVELTICK_All, nullptr);

		ProjectileMovementComponent->TickComponent(FMath::Fmod(DeltaTime, MaxDeltaPerCall), ELevelTick::LEVELTICK_All, nullptr);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("OnOverlapBegin"), STAT_OnOverlapBegin, STATGROUP_SoProjectile);

	if (OtherActor->Implements<USoMortal>() &&
		((OtherActor != SpawnerActor || !bDoNotDamageSpawner) &&
			!(bDoNotDamageSpawner && SpawnerActor == nullptr)) &&
			((!bOnlyHitAliveTargets) || ISoMortal::Execute_IsAlive(OtherActor)))
	{
		OnTargetHit(OtherActor, SweepResult);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::OnHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (RemainingHitCount > 0)
	{
		RemainingHitCount -= 1;
		if (RemainingHitCount == 0)
		{
			OnHitBP(Hit, false);
			DestroyProjectile();
		}
		else
			OnHitBP(Hit, true);
	}
	else
		OnHitBP(Hit, true);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectile::UpdateCollision()
{
	if (AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(this))
	{
		const FVector Loc = Player->GetActorLocation();
		const bool bShouldCheckOverlaps = (Loc - GetActorLocation()).SizeSquared() < CollisionActiveDistanceFromPlayerSquered;
		if (bShouldCheckOverlaps != bCollisionActiveOnPrimitiveComponent)
		{
			ensure(PrimitiveComponent);

			if (bShouldCheckOverlaps)
				PrimitiveComponent->SetCollisionEnabled(PrimitiveComponentCollisionType);
			else
				PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			bCollisionActiveOnPrimitiveComponent = bShouldCheckOverlaps;
			OnCollisionUpdatedBP(bCollisionActiveOnPrimitiveComponent);
		}
	}
}
