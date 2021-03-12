// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEACombat.h"

#include "Basic/SoCollisionChannels.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Enemy/SoEnemy.h"
#include "Enemy/EPreconditions/SoEPrecondition.h"
#include "Components/CapsuleComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEAStrike::USoEAStrike()
{
	// most likely
	bForceSplineDirection = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStrike::OnEnter(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEAStrike::OnEnter (%s)"), *Name.ToString());

	ActivePhase = 0;
	Owner->Strike(Name);
	Counter = 0.0f;
	CachedMaxWalkSpeed = Owner->GetSoMovement()->MaxWalkSpeed;
	bLeave = false;

	if (const FSoStrike* Strike = Owner->GetStrike(Name))
	{
		if (Strike->bLookAtPlayer)
			bForceSplineDirection = false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAStrike::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	const FSoStrike* Strike = Owner->GetStrike(Name);

	if (Strike == nullptr)
	{
		UE_LOG(LogSoEnemyAI, Warning, TEXT("Failed to find strike (%s)"), *Name.ToString());
		return false;
	}

	if (bLeave || !Strike->Phases.IsValidIndex(ActivePhase))
		return false;

	const float NewTime = Counter + DeltaSeconds;

	Update(Owner, Strike->Phases[ActivePhase], Counter, NewTime);

	if (NewTime > Strike->Phases[ActivePhase].Duration)
	{
		Counter = NewTime - Strike->Phases[ActivePhase].Duration;

		ActivePhase += 1;
		if (ActivePhase == Strike->Phases.Num())
			return false;

		Owner->OnStrikePhaseSwitch(Strike->Phases[ActivePhase], ActivePhase);
		Update(Owner, Strike->Phases[ActivePhase], 0.0f, Counter);
	}
	else
		Counter = NewTime;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStrike::Update(ASoEnemy* Owner, const FSoStrikePhase& Phase, float OldTime, float NewTime)
{
	// Root motion:
	// step 1: turn off old ones
	for (const FSoRootMotionEntry& RootM : Phase.RootMotion)
		if (RootM.To >= OldTime && RootM.To < NewTime)
			Owner->ClearRootMotionDesc();
	// step 2: activate new ones
	for (const FSoRootMotionEntry& RootM : Phase.RootMotion)
		if (RootM.From >= OldTime && RootM.From < NewTime)
			Owner->SetRootMotionDesc(RootM.RootMotion);

	if (bAreaLimitedRootMotion &&
		!Owner->IsBetweenMarkers(0, 1, true) &&
		!Owner->IsFacingMarker(0, true))
	{
		Owner->ClearRootMotionDesc();
		Owner->GetSoMovement()->Velocity.X = 0.0f;
		Owner->GetSoMovement()->Velocity.Y = 0.0f;
	}


	// controlled motion:

	// step 1: turn off old ones if there is any
	if (ControlledMotionCounter > 0)
	{
		for (const FSoControlledMotionEntry& Motion : Phase.ControlledMotion)
			if (Motion.To >= OldTime && Motion.To < NewTime)
				ControlledMotionCounter -= 1;

		if (ControlledMotionCounter <= 0)
			Owner->GetSoMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;

#if WITH_EDITOR
		if (ControlledMotionCounter < 0)
			UE_LOG(LogSoEnemyAI, Warning, TEXT("Invalid ControlledMotionCounter value %d: check the intervals!"), ControlledMotionCounter);
#endif
	}

	// step 2: activate new ones
	for (const FSoControlledMotionEntry& Motion : Phase.ControlledMotion)
		if (Motion.From >= OldTime && Motion.From < NewTime)
		{
			SetStopOnHit(Motion.bStopIfStuck, Owner);
			ControlledMotionCounter += 1;
			Owner->GetSoMovement()->MaxWalkSpeed = Motion.Speed;
		}

	// apply active
	// TODO: handle vertical movement?!
	//		 handle negative speed value?!
	if (ControlledMotionCounter > 0)
		Owner->AddMovementInput(Owner->GetActorForwardVector(), 1.0f, true);


	// Weapon collision:
	// step 1: inactivate old
	for (const FSoWeaponCollisionEntry& Collision : Phase.WeaponCollision)
		if (Collision.To >= OldTime && Collision.To < NewTime)
			Owner->ActivateWeaponCollision(false, Collision.Slot);

	// step 2: activate new
	for (const FSoWeaponCollisionEntry& Collision : Phase.WeaponCollision)
		if (Collision.From >= OldTime && Collision.From < NewTime)
			Owner->ActivateWeaponCollision(true, Collision.Slot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStrike::OnLeave(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEAStrike::OnLeave (%s)"), *Name.ToString());
	Owner->StrikeEnd();

	Owner->GetSoMovement()->MaxWalkSpeed = CachedMaxWalkSpeed;
	ControlledMotionCounter = 0;
	SetStopOnHit(false, Owner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStrike::OnHitCallback(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bStopOnHit && OtherComp->GetCollisionObjectType() != ECC_Projectile)
		bLeave = true;

	ASoEnemy* Owner = Cast<ASoEnemy>(HitComp->GetOwner());
	if (Owner != nullptr)
		Owner->OnCapsuleComponentHitWhileStriking(OtherActor, OtherComp, NormalImpulse, Hit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAStrike::SetStopOnHit(bool bNewValue, ASoEnemy* Owner)
{
	if (bStopOnHit != bNewValue)
	{
		bStopOnHit = bNewValue;

		if (bStopOnHit)
			Owner->GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &USoEAStrike::OnHitCallback);
		else
			Owner->GetCapsuleComponent()->OnComponentHit.RemoveDynamic(this, &USoEAStrike::OnHitCallback);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEABlocked::OnEnter(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEABlocked::OnEnter"));
	Owner->SetActivity(ESoEnemyActivity::EEA_Blocked);
	Counter = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEABlocked::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Counter += DeltaSeconds;
	return Duration > Counter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEABlocked::OnLeave(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEABlocked::OnLeave"));
	Owner->SetActivity(ESoEnemyActivity::EEA_Default);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEABlock::OnEnter(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEABlock::OnEnter (%s)"), *Name.ToString());
	Owner->Block(Name, Duration);
	Counter = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEABlock::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Counter += DeltaSeconds;
	return Duration > Counter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEABlock::OnLeave(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEABlock::OnLeave (%s)"), *Name.ToString());
	Owner->SetActivity(ESoEnemyActivity::EEA_Default);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAHitReact::OnEnter(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEAHitReact::OnEnter (%s)"), *Name.ToString());
	Owner->HitReact(Name, Duration);
	Counter = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEAHitReact::OnTick(float DeltaSeconds, ASoEnemy* Owner)
{
	Counter += DeltaSeconds;
	return Duration > Counter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEAHitReact::OnLeave(ASoEnemy* Owner)
{
	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEAHitReact::OnLeave (%s)"), *Name.ToString());
	Owner->SetActivity(ESoEnemyActivity::EEA_Default);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void USoEARangeAttack::OnEnter(ASoEnemy* Owner)
//{
//	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEARangeAttack::OnEnter (%s)"), *Name.ToString());
//	Counter = 0.0f;
//	bProjectileCreated = false;
//
//	const FSoERangeAttackDesc* Desc = Owner->GetRangeAttackDesc(Name);
//	if (Desc != nullptr)
//		Owner->OnRangeAttackStarted(Name, Desc->TimeBeforeSpawn + Desc->TimeAfterSpawn);
//}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bool USoEARangeAttack::OnTick(float DeltaSeconds, ASoEnemy* Owner)
//{
//	Counter += DeltaSeconds;
//
//	const FSoERangeAttackDesc* Desc = Owner->GetRangeAttackDesc(Name);
//	if (Desc == nullptr)
//		return false;
//
//	if (bProjectileCreated)
//		return Counter < Desc->TimeAfterSpawn;
//
//	if (Counter > Desc->TimeBeforeSpawn)
//	{
//		bProjectileCreated = true;
//		const AActor* Target = Owner->GetTarget();
//		if (Target != nullptr)
//		{
//			Owner->OnRangeAttackFired();
//
//			const FVector StartPos = Owner->GetRangeAttackLocation(Name);
//			const FVector TargetPos = Target->GetActorLocation();
//
//			//if (bOnSpline)
//			//{
//			//	const FSoSplinePoint OwnerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Owner);
//			//	const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(Owner);
//			//	const FVector Delta = StartPos - FVector(OwnerSplineLocation);
//			//	const FSoSplinePoint StartSplineLocation = OwnerSplineLocation + OwnerSplineLocation.GetDirectionModifierFromVector(Delta) * Delta.Size2D();
//			//	FVector Velocity = USoMathHelper::CalcRangeAttackVelocity(StartPos,
//			//																StartSplineLocation,
//			//																TargetPos,
//			//																PlayerSplineLocation,
//			//																Desc->BulletSpeed,
//			//																Desc->BulletGravityScale);
//			//	if (Velocity.Size() < KINDA_SMALL_NUMBER)
//			//		Velocity = Owner->GetActorForwardVector() * Desc->BulletSpeed;
//			//
//			//	ASoBullet* Projectile = Owner->GetWorld()->SpawnActor<ASoBullet>(Desc->ProjectileClass, StartPos, Velocity.Rotation());
//			//	Projectile->SetGravityScale(Desc->BulletGravityScale);
//			//
//			//	const FVector Offset = (FVector(StartSplineLocation) - StartPos);
//			//	Projectile->FireOnSplineWithOffset(StartSplineLocation, FVector(Offset.X, Offset.Y, 0.0f), Velocity);
//			//	Projectile->SetSpawnerActor(Owner);
//			//}
//			//else
//			//{
//			//	FVector Velocity = USoMathHelper::CalcRangeAttackVelocity(StartPos,
//			//																TargetPos,
//			//																Desc->BulletSpeed,
//			//																Desc->BulletGravityScale);
//			//	if (Velocity.Size() < KINDA_SMALL_NUMBER)
//			//		Velocity = (TargetPos - StartPos).GetSafeNormal() * Desc->BulletSpeed;
//			//
//			//	ASoBullet* Projectile = Owner->GetWorld()->SpawnActor<ASoBullet>(Desc->ProjectileClass, StartPos, Velocity.Rotation());
//			//	Projectile->SetGravityScale(Desc->BulletGravityScale);
//			//	Projectile->FireOnSplineNew(FSoSplinePoint{ nullptr }, Velocity);
//			//	Projectile->SetSpawnerActor(Owner);
//			//}
//		}
//	}
//
//	return true;
//}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//void USoEARangeAttack::OnLeave(ASoEnemy* Owner)
//{
//	UE_LOG(LogSoEnemyAI, Display, TEXT("USoEARangeAttack::OnLeave (%s)"), *Name.ToString());
//	Owner->OnRangeAttackEnded();
//}
