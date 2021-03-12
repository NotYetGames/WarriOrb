// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoProjectileSpawnerAutomata.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/ArrowComponent.h"

#include "Kismet/GameplayStatics.h"

#include "SaveFiles/SoWorldState.h"
#include "Basic/SoGameInstance.h"
#include "SplineLogic/SoSplineHelper.h"
#include "SplineLogic/SoSpline.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoTimerManager.h"
#include "Basic/SoGameMode.h"
#include "SaveFiles/SoWorldStateBlueprint.h"
#include "Basic/SoAudioManager.h"
#include "SoProjectile.h"
#include "Basic/Helpers/SoPlatformHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoProjectileSpawnerAutomata::ASoProjectileSpawnerAutomata()
{
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("SoArrow"));
	SetRootComponent(ArrowComponent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bUpdateSplineLocation)
	{
		if (bSpawnProjectileOnSpline)
		{
			USoSplineHelper::UpdateSplineLocationRef(this, SplineLocationPtr, false, false);

			// store reference if they are on the same level - some child may work in editor
			if (SplineLocationPtr.IsValid())
			{
				FSoSplinePoint Point = SplineLocationPtr.Extract();
				if (Point.GetSpline()->GetLevel() == GetLevel())
					SplineLocation = Point;
			}
		}
		else
		{
			SplineLocationPtr = {};
			SplineLocation = {};
		}
	}
	ArrowComponent->SetArrowColor(SplineLocationPtr.IsValid() ? SplineColor : NonSplineColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::BeginPlay()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SpawnerAutomata - BeginPlay"), STAT_SpawnerAutomata_BeginPlay, STATGROUP_SoProjectile);

	if (PreSimulationTime > KINDA_SMALL_NUMBER)
		StartPreSimulation();

	if (bSpawnProjectileOnSpline)
	{
		SplineLocation = SplineLocationPtr.Extract();
		SplineLocation.SetReferenceZ(GetActorLocation().Z);

		if (!SplineLocation.IsValid(false))
		{
			UE_LOG(LogTemp, Warning, TEXT("ASoProjectileSpawnerAutomata %s was supposed to spawn stuff on spline but the spline isn't setup correctly!"), *GetName());
			bSpawnProjectileOnSpline = false;
		}
	}

	Super::BeginPlay();

	// build spline list from spline ptr-s
	for (TAssetPtr<ASoSpline>& SplinePtr : WhiteListedSplinePtrList)
	{
		ASoSpline* Spline = SplinePtr.Get();
		if (Spline != nullptr)
			WhiteListedSplineList.Add(Spline);
	}

	if (bSerializeState)
		USoEventHandlerHelper::SubscribeToSoPostLoad(this);
	else
	{
		if (bActiveByDefault &&
			(!ShouldUseWhitelistedSplines() || CharIsOnWhitelistedSplines()))
			StartExecution();
	}

	if (ShouldUseWhitelistedSplines())
	{
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (Character != nullptr)
		{
			Character->OnPlayerSplineChanged.AddDynamic(this, &ASoProjectileSpawnerAutomata::OnPlayerSplineChanged);
		}
	}

	bPreSimulationPhase = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ShouldUseWhitelistedSplines())
	{
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (Character != nullptr)
			Character->OnPlayerSplineChanged.RemoveDynamic(this, &ASoProjectileSpawnerAutomata::OnPlayerSplineChanged);
		WhiteListedSplineList.Empty();
	}

	ClearSpawnedProjectiles();
	StopExecution(false);

	if (bSerializeState)
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);

	ClearTimer();

	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::HandleSoPostLoad_Implementation()
{
	check(bSerializeState);

	ClearSpawnedProjectiles();
	StopExecution(false);
	const bool bChanged = USoWorldState::IsActorNameInSet(this);
	if (bChanged != bActiveByDefault &&
		(!ShouldUseWhitelistedSplines() || CharIsOnWhitelistedSplines()))
		StartExecution();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::HandlePlayerRematerialize_Implementation()
{
	check(bResetOnPlayerRematerialize);

	ClearSpawnedProjectiles();
	StopExecution(true);
	if (bActiveByDefault)
		StartExecution();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::Trigger_Implementation(const FSoTriggerData& InTriggerData)
{
	if (InTriggerData.SourceIdentifier > 0)
	{
		if (MaxSpawnedProjectileNum <= 0 || SpawnedProjectiles.Num() < MaxSpawnedProjectileNum)
		StartExecution();
	}
	else
		StopExecution(true);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::StartExecution()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SpawnerAutomata - StartExecution"), STAT_SpawnerAutomata_StartExecution, STATGROUP_SoProjectile);

	if (ShouldUseWhitelistedSplines())
	{
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (Character != nullptr)
		{
			if (!WhiteListedSplineList.Contains(ISoSplineWalker::Execute_GetSplineLocationI(Character).GetSpline()))
			{
				return;
			}
		}
	}

	if (!bWorking && (TimeTable.Num() > 0 || !bStartWithWait))
	{
		bWorking = true;
		UpdateSerializedState();

		if (FirstTimeOffset > KINDA_SMALL_NUMBER)
		{
			ActiveIndex = -1;
			ClearTimer();
			TimerID = USoStaticHelper::GetSoTimerManager(this).SetTimer(this, &ASoProjectileSpawnerAutomata::Fire, FirstTimeOffset);
		}
		else
		{
			if (bStartWithWait)
			{
				ActiveIndex = 0;
				ClearTimer();
				TimerID = USoStaticHelper::GetSoTimerManager(this).SetTimer(this, &ASoProjectileSpawnerAutomata::Fire, TimeTable[0]);
			}
			else
			{
				ActiveIndex = -1;
				Fire();
			}
		}


		if (bResetOnPlayerRematerialize)
			USoEventHandlerHelper::SubscribeToPlayerRematerialize(this);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::StopExecution(bool bCanUpdateSerializedState)
{
	// outside of if because this stops the end trigger timer as well which happens when bWorking is false
	ClearTimer();
	if (bWorking || bCanUpdateSerializedState)
	{
		ActiveIndex = -1;
		bWorking = false;

		if (bResetOnPlayerRematerialize && SpawnedProjectiles.Num() == 0)
			USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(this);

		if (bCanUpdateSerializedState)
			UpdateSerializedState();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::Fire()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SpawnerAutomata - Fire"), STAT_SpawnerAutomata_Fire, STATGROUP_SoProjectile);

	ClearTimer();
	TimerID = -1;
	if (bWorking)
	{
		bool bFired = true;
		if (bTeleportAbovePlayer)
		{
			bFired = false;
			const float Z = GetActorLocation().Z;
			if (AActor* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
			{
				FVector Transform = Player->GetActorLocation();
				Transform.Z = TeleportAbovePlayerZDist > 0.0f ? Transform.Z + TeleportAbovePlayerZDist : Z;
				SetActorLocation(Transform);

				if (ISoMortal::Execute_IsAlive(Player))
				{
					OnFireProjectiles();
					bFired = true;
				}
			}
		}
		else
			OnFireProjectiles();

		if (TriggerOnFire != nullptr && bFired)
			USoTriggerHelper::TriggerActor(TriggerOnFire, TriggerOnFireValue);

		OnProjectileSpawned.Broadcast();

		if (SFXOnFire != nullptr && bFired)
			USoAudioManager::PlaySoundAtLocation(this, SFXOnFire, GetActorTransform());

		ActiveIndex += 1;

		if (!TimeTable.IsValidIndex(ActiveIndex))
		{
			if (!bLoop)
			{
				StopExecution(true);
				if (TriggerTargetList.Num() > 0)
				{
					if (TriggerDelay > KINDA_SMALL_NUMBER)
						TimerID = USoStaticHelper::GetSoTimerManager(this).SetTimer(this, &ASoProjectileSpawnerAutomata::TriggerTargets, TriggerDelay);
					else
						TriggerTargets();
				}
				return;
			}
			ActiveIndex = 0;
		}
		TimerID = USoStaticHelper::GetSoTimerManager(this).SetTimer(this, &ASoProjectileSpawnerAutomata::Fire, TimeTable[ActiveIndex]);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::TriggerTargets()
{
	for (AActor* Target : TriggerTargetList)
		ISoTriggerable::Execute_Trigger(Target, TriggerData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::OnProjectileDied(ASoProjectile* Projectile)
{
	ClearSpawnedProjectile(Projectile);
	SpawnedProjectiles.RemoveSwap(Projectile);

	if (SpawnedProjectiles.Num() == 0)
		OnAllSpawnedProjectileDied.Broadcast();

	if (!bWorking && bResetOnPlayerRematerialize && SpawnedProjectiles.Num() == 0)
		USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::OnPlayerSplineChanged(const ASoSpline* OldSpline, const ASoSpline* NewSpline)
{
	const bool bPlayerInWhitelisted = WhiteListedSplineList.Contains(NewSpline);
	if (!bPlayerInWhitelisted)
	{
		StopExecution(false);

		if (bKillProjectilesOnWhitelistedSplinesLeft)
			ClearSpawnedProjectiles();

		if (TriggerOnStateChange != nullptr)
			USoTriggerHelper::TriggerActor(TriggerOnStateChange, 0);
	}
	else
	{
		bool bStartExecution = bActiveByDefault;
		if (bSerializeState)
		{
			const bool bChanged = USoWorldState::IsActorNameInSet(this);
			bStartExecution = (bChanged != bActiveByDefault);
		}

		if (bStartExecution && !bWorking)
		{
			if (PreSimulationTime > KINDA_SMALL_NUMBER && bPreSimulateOnWhitelistedSplineEnterred)
				StartPreSimulation();

			StartExecution();
			bPreSimulationPhase = false;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::OnFireProjectiles_Implementation()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("SpawnerAutomata - OnFireProjectiles_Implementation"), STAT_SpawnerAutomata_OnFireProjectiles_Implementation, STATGROUP_SoProjectile);

	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn projectile: class is nullptr!"));
		return;
	}

	ASoProjectile* Projectile = USoGameInstance::Get(this).ClaimProjectile(ProjectileClass);
	if (Projectile == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn projectile %s!"), *ProjectileClass->GetName());
		return;
	}

	if (bSpawnProjectileOnSpline)
	{
		if (SplineLocation.GetSpline() == nullptr)
		{

			UE_LOG(LogTemp, Warning, TEXT("Failed to spawn projectile: invalid spline location! (%s)"), *GetOwner()->GetClass()->GetName());
			return;
		}

		const FVector ActorLocation = GetActorLocation();
		const FVector Offset = SplineLocation.ToVector(ActorLocation.Z) - ActorLocation;
		Projectile->ActivateProjectileOnSpline(SplineLocation,
											   ActorLocation.Z,
											   Offset,
											   GetActorRotation(),
											   bOverrideProjectileInitData ? &ProjectileInitData : nullptr,
											   bOverrideProjectileRuntimeData ? &ProjectileRuntimeData : nullptr,
											   this);
	}
	else
		Projectile->ActivateProjectile(GetActorLocation(),
									   GetActorRotation(),
									   bOverrideProjectileInitData ? &ProjectileInitData : nullptr,
									   bOverrideProjectileRuntimeData ? &ProjectileRuntimeData : nullptr,
									   GetOwner());

	Projectile->OnDeathNotify.AddDynamic(this, &ASoProjectileSpawnerAutomata::OnProjectileDied);
	SpawnedProjectiles.Add(Projectile);
	if (SFXProjectileImpactOverride != nullptr)
		Projectile->SetSFXOverride(SFXProjectileImpactOverride);

	if (bPreSimulationPhase)
	{
		const float CurrentExtraTime = USoStaticHelper::GetSoTimerManager(this).GetExtraTime(this);
		if (CurrentExtraTime > KINDA_SMALL_NUMBER)
		{
			const float TimeToSimulate = PreSimulationTime - CurrentExtraTime;
			Projectile->ForceUpdate(TimeToSimulate);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::UpdateSerializedState()
{
	if (bSerializeState)
	{
		if (bActiveByDefault == bWorking)
			USoWorldState::RemoveActorNameFromSet(this);
		else
			USoWorldState::AddActorNameToSet(this);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::StartPreSimulation()
{
	bPreSimulationPhase = true;
	USoStaticHelper::GetSoTimerManager(this).SetExtraTime(this, PreSimulationTime);
	if (TriggerOnStateChange != nullptr)
		USoTriggerHelper::TriggerActor(TriggerOnStateChange, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::ClearTimer()
{
	if (TimerID >= 0)
		TimerID = USoStaticHelper::GetSoTimerManager(this).ClearTimer(TimerID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::ClearSpawnedProjectiles()
{
	for (auto* Projectile : SpawnedProjectiles)
		ClearSpawnedProjectile(Projectile);

	SpawnedProjectiles.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoProjectileSpawnerAutomata::ClearSpawnedProjectile(ASoProjectile* Projectile)
{
	if (Projectile == nullptr)
		return;

	Projectile->OnDeathNotify.RemoveDynamic(this, &ASoProjectileSpawnerAutomata::OnProjectileDied);
	Projectile->DeactivateProjectile();

	USoGameInstance::Get(this).ReturnProjectile(Projectile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoProjectileSpawnerAutomata::ShouldUseWhitelistedSplines() const
{
	if (USoPlatformHelper::HasWeakHardware())
		return WhiteListedSplineList.Num() > 0;

	return WhiteListedSplineList.Num() > 0 && (!bUseWhitelistedSplinesOnlyOnSwitch);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoProjectileSpawnerAutomata::CharIsOnWhitelistedSplines() const
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
		return WhiteListedSplineList.Contains(USoStaticHelper::GetPlayerSplineLocation(this).GetSpline());

	return false;
}