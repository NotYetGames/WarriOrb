// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEARangeAttack.h"

#include "Components/CapsuleComponent.h"

#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Enemy/SoEnemy.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Projectiles/SoProjectileSpawnerComponent.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEARangeAttack::OnEnter(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEARangeAttack::OnEnter (%s)"), *Name.ToString());

	Counter = 0.0f;

	if (AnimationType == ESoRangeAttackAnimType::ERAT_SingleAnimBasedTiming)
		if (UAnimSequenceBase* const* AnimPtr = Owner->GetAnimationMap().Find(Name))
			Duration = USoStaticHelper::GetScaledAnimLength(*AnimPtr);

	Owner->OnRangeAttackStarted(Name, Duration, AnimationType, bInterruptible);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEARangeAttack::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	const float NewCounter = Counter + DeltaSeconds;

	for (const FSoRangeAttackSpawn& Spawn : AttackList)
		if (Spawn.Time >= Counter && Spawn.Time < NewCounter)
		{
			const FSoERangeAttackProfile* Profile = Owner->InitializeAndGetRangeAttackProfile(Spawn.Index, Spawn.Variant);
			if (Profile == nullptr)
			{
				UE_LOG(LogSoEnemyAI, Error, TEXT("%s Failed to spawn projectile: invalid index %d"), *Owner->GetName(), Spawn.Index);
				continue;
			}

			UE_LOG(LogSoEnemyAI, Display, TEXT("%s spawns projectile!"), *Owner->GetName());

			USoProjectileSpawnerComponent* ProjectileSpawner = Owner->GetProjectileSpawner();

			if (Profile->bUseThisInsteadOfComponentDefaults)
				ProjectileSpawner->SpawnProjectileFromClass(Profile->ProjectileClass,
															Profile->Location,
															Profile->Orientation,
															Profile->bUseProjectileInitDataOverride ? &Profile->InitDataOverride : nullptr);
			else
				ProjectileSpawner->SpawnProjectile();
		}

	for (const FSoAnimationOverride& Animation : AnimationOverrideList)
		if (Animation.Time >= Counter && Animation.Time < NewCounter)
			Owner->OverrideAnimation(Animation.bInterruptible, Animation.AnimationName, Animation.Duration, Animation.bLoop);

	Counter = NewCounter;

	bool bStillAlive = Counter < Duration;
	if (bForceDoneOnDurationOver && !bStillAlive)
		return false;

	if (bDoneIfProjectilesDied && AttackList.Num() > 0 && AttackList.Last(0).Time < Counter)
		bStillAlive = Owner->GetProjectileSpawner()->HasActiveProjectile();

	return bStillAlive;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEARangeAttack::OnLeave(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEARangeAttack::OnLeave (%s)"), *Name.ToString());
	Owner->OnRangeAttackEnded();
}
