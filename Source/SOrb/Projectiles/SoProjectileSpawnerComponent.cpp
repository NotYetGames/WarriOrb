// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoProjectileSpawnerComponent.h"

#include "GameFramework/Actor.h"

#include "Basic/SoGameInstance.h"
#include "SplineLogic/SoSplineWalker.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoProjectileSpawnerComponent::USoProjectileSpawnerComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileSpawnerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	check(GetOwner());

	if (bSpawnProjectileOnSpline && !GetOwner()->GetClass()->ImplementsInterface(USoSplineWalker::StaticClass()))
	{
		bSpawnProjectileOnSpline = false;
		UE_LOG(LogTemp, Warning, TEXT("USoProjectileSpawnerComponent is marked bSpawnProjectileOnSpline but owner %s does not implement ISoSplineWalker!"), *GetOwner()->GetClass()->GetName());
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
	HandleProjectileSpawnerBeginPlay();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	HandleProjectileSpawnerEndPlay();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileSpawnerComponent::HandleProjectileSpawnerBeginPlay()
{
	if (bClearParticlesOnReload)
		USoEventHandlerHelper::SubscribeToSoPostLoad(this);
	if (bClearParticlesOnPlayerRematerialize)
		USoEventHandlerHelper::SubscribeToPlayerRematerialize(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileSpawnerComponent::HandleProjectileSpawnerEndPlay()
{
	ClearSpawnedProjectiles();

	if (bClearParticlesOnReload)
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);
	if (bClearParticlesOnPlayerRematerialize)
		USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileSpawnerComponent::ClearSpawnedProjectiles()
{
	for (auto* Projectile : SpawnedProjectiles)
	{
		if (Projectile != nullptr)
		{
			Projectile->DeactivateProjectile();
			Projectile->OnDeathNotify.RemoveDynamic(this, &USoProjectileSpawnerComponent::OnProjectileDied);
			USoGameInstance::Get(GetOwner()).ReturnProjectile(Projectile);
		}
	}

	SpawnedProjectiles.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile* USoProjectileSpawnerComponent::SpawnProjectile()
{
	if (bSpawnProjectileOnSpline)
	{
		FSoSplinePoint OwnerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(GetOwner());
		if (OwnerSplineLocation.GetSpline() == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn projectile: invalid spline location! (%s)"), *GetOwner()->GetClass()->GetName());
			return nullptr;
		}
		const FVector ComponentLocation = GetComponentLocation();
		const FVector Delta = ComponentLocation - FVector(OwnerSplineLocation);
		const FSoSplinePoint StartSplineLocation = OwnerSplineLocation + OwnerSplineLocation.GetDirectionModifierFromVector(Delta) * Delta.Size2D();
		const FVector Offset = FVector(StartSplineLocation) - ComponentLocation;
		return SpawnProjectile_Internal(StartSplineLocation, ComponentLocation.Z, GetComponentRotation(), FVector(Offset.X, Offset.Y, 0.0f));
	}

	return SpawnProjectile_Internal(GetComponentLocation(), GetComponentRotation());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile* USoProjectileSpawnerComponent::SpawnProjectileFromClass(const TSubclassOf<ASoProjectile>& ClassToSpawn,
																	   const FVector& Location,
																	   const FRotator& Orientation,
																	   const FSoProjectileInitData* InitDataOverride)
{
	const FSoProjectileInitData* InitDataPtr = InitDataOverride;
	if (InitDataPtr == nullptr)
		InitDataPtr = bOverrideProjectileInitData ? &ProjectileInitData : nullptr;

	ASoProjectile* Projectile = USoGameInstance::Get(GetOwner()).ClaimProjectile(ClassToSpawn);
	Projectile->ActivateProjectile( Location,
									Orientation,
									InitDataPtr,
									bOverrideProjectileRuntimeData ? &ProjectileRuntimeData : nullptr,
									GetOwner());

	Projectile->OnDeathNotify.AddDynamic(this, &USoProjectileSpawnerComponent::OnProjectileDied);
	SpawnedProjectiles.Add(Projectile);
	return Projectile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile* USoProjectileSpawnerComponent::SpawnFromProjectile(const TSubclassOf<ASoProjectile> ClassToSpawn,
																  const FVector& Location,
																  const FRotator& Orientation)
{
	if (ASoProjectile* Projectile = USoGameInstance::Get(GetOwner()).ClaimProjectile(ClassToSpawn))
	{
		Projectile->ActivateProjectile(Location, Orientation, nullptr, nullptr, GetOwner());
		Projectile->OnDeathNotify.AddDynamic(this, &USoProjectileSpawnerComponent::OnProjectileDied);
		SpawnedProjectiles.Add(Projectile);
		return Projectile;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile* USoProjectileSpawnerComponent::SpawnFromProjectileOnSpline(const TSubclassOf<ASoProjectile> ClassToSpawn,
																		  const FVector& Location,
																		  const FRotator& Orientation,
																		  const FSoSplinePoint& SplineLocation)
{
	ASoProjectile* Projectile = USoGameInstance::Get(GetOwner()).ClaimProjectile(ClassToSpawn);

	if (Projectile->IsSplineProjectile())
		Projectile->ActivateProjectileOnSpline(SplineLocation, Location.Z, FVector(0.0f, 0.0f, 0.0f), Orientation, nullptr, nullptr, GetOwner());
	else
		Projectile->ActivateProjectile(Location, Orientation, nullptr, nullptr, GetOwner());

	Projectile->OnDeathNotify.AddDynamic(this, &USoProjectileSpawnerComponent::OnProjectileDied);
	SpawnedProjectiles.Add(Projectile);
	return Projectile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile*  USoProjectileSpawnerComponent::SpawnProjectile_Internal(const FVector& Location, const FRotator& Rotation)
{
	ASoProjectile* Projectile = USoGameInstance::Get(GetOwner()).ClaimProjectile(ProjectileClass);
	if (Projectile != nullptr)
	{
		Projectile->ActivateProjectile( Location,
										Rotation,
										bOverrideProjectileInitData ? &ProjectileInitData : nullptr,
										bOverrideProjectileRuntimeData ? &ProjectileRuntimeData : nullptr,
										GetOwner());

		Projectile->OnDeathNotify.AddDynamic(this, &USoProjectileSpawnerComponent::OnProjectileDied);
		SpawnedProjectiles.Add(Projectile);
	}
	return Projectile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectile* USoProjectileSpawnerComponent::SpawnProjectile_Internal(const FSoSplinePoint& SplineLocation,
																	   float ZValue,
																	   const FRotator& Rotation,
																	   const FVector& OffsetFromSpline)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Spawner - SpawnProjectile_Internal"), STAT_Spawner_SpawnProjectile_Internal, STATGROUP_SoProjectile);

	ASoProjectile* Projectile = USoGameInstance::Get(GetOwner()).ClaimProjectile(ProjectileClass);
	if (SplineLocation.GetSpline() == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn projectile: invalid spline location! (%s)"), *GetOwner()->GetClass()->GetName());
		return nullptr;
	}
	if (Projectile != nullptr)
	{
		Projectile->ActivateProjectileOnSpline(SplineLocation,
												ZValue,
												OffsetFromSpline,
												Rotation,
												bOverrideProjectileInitData ? &ProjectileInitData : nullptr,
												bOverrideProjectileRuntimeData ? &ProjectileRuntimeData : nullptr,
												GetOwner());

		Projectile->OnDeathNotify.AddDynamic(this, &USoProjectileSpawnerComponent::OnProjectileDied);
		SpawnedProjectiles.Add(Projectile);
	}
	return Projectile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoProjectileSpawnerComponent::OnProjectileDied(ASoProjectile* Projectile)
{
	Projectile->OnDeathNotify.RemoveDynamic(this, &USoProjectileSpawnerComponent::OnProjectileDied);
	SpawnedProjectiles.RemoveSwap(Projectile);
	USoGameInstance::Get(GetOwner()).ReturnProjectile(Projectile);
}
