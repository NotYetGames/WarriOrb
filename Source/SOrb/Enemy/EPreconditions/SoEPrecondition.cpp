// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEPrecondition.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Engine/World.h"

#include "Enemy/SoEnemy.h"
#include "Enemy/SoEnemyHelper.h"
#include "Enemy/EActions/SoEAction.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameMode.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/SoGameInstance.h"
#include "Projectiles/SoProjectile.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPrecondition::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	check(false);
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPSplineDistanceFromPlayer::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const FSoSplinePoint OwnerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(OwnerCharacter);
	const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(OwnerCharacter);

	if (!PlayerSplineLocation.IsValid(false))
		return 0.0f;

	if (bCheckAgainstAlert)
	{
		const float AlertDistance = OwnerCharacter->GetAlertDistance();
		const float Distance = fabs(OwnerSplineLocation.GetDistanceFromSplinePoint(PlayerSplineLocation, AlertDistance + 10.0f));
		return Distance < AlertDistance;
	}

	const float Distance = fabs(OwnerSplineLocation.GetDistanceFromSplinePoint(PlayerSplineLocation, MaxDistance + 10.0f));
	return Distance > MinDistance && Distance < MaxDistance ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPSplineDistanceFromClosestAlly::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const float ClosestDistance = FMath::Abs(USoEnemyHelper::GetSignedSplineDistanceToClosestEnemy(OwnerCharacter, bIgnoreFloatings, bOnlySameClass));
	return (ClosestDistance > MinDistance && ClosestDistance < MaxDistance) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPZDistanceFromPlayer::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const float OwnerZ = OwnerCharacter->GetActorLocation().Z;

	AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(OwnerCharacter);
	if (Player == nullptr)
		return 0.0f;

	const float Distance = OwnerZ - Player->GetActorLocation().Z;
	return ((Distance > MinDistance && Distance < MaxDistance) != bInverse) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPDistanceFromPlayer::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(OwnerCharacter);
	if (Player == nullptr)
		return 0.0f;

	const FVector OwnerLoc = OwnerCharacter->GetActorLocation();
	const FVector PlayerLoc = Player->GetActorLocation();

	const float DistanceSquared = (OwnerLoc - PlayerLoc).SizeSquared();
	return (DistanceSquared > MinDistance*MinDistance && DistanceSquared < MaxDistance*MaxDistance) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPZDistanceFromStart::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const float OwnerZ = OwnerCharacter->GetActorLocation().Z;
	const float StartZ = OwnerCharacter->GetInitialLocation().Z;
	const float Distance = OwnerZ - StartZ;

	return ((Distance > MinDistance && Distance < MaxDistance) != bInverse) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPNotFacingPlayer::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const FSoSplinePoint OwnerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(OwnerCharacter);
	const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(OwnerCharacter);
	if (!PlayerSplineLocation.IsValid(false))
		return 0.0f;

	const int32 FaceDir = FMath::Sign(OwnerSplineLocation.GetDirectionModifierFromVector(OwnerCharacter->GetActorForwardVector()));
	const int32 TargetDir = static_cast<int32>(FMath::Sign((PlayerSplineLocation - OwnerSplineLocation)));

	if (bInverse)
		return FaceDir == TargetDir ? TrueValue : 0.0f;

	return FaceDir != TargetDir ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPPlayerNotInDialogue::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	if (ASoCharacter* Player = USoStaticHelper::GetPlayerCharacterAsSoCharacter(OwnerCharacter))
	{
		return Player->IsInDialogue() ? 0.0f : TrueValue;
	}

	return TrueValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPWasNotUsedRecently::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const float CurrentTime = OwnerCharacter->GetWorld()->GetTimeSeconds();
	const float LastUsageTime = OwnerAction->GetLastUsageTime();
	const float TimeSinceUsage = OwnerCharacter->GetWorld()->GetTimeSeconds() - OwnerAction->GetLastUsageTime();

	if (TimeSinceUsage < SoftTimeDelta)
		return 0.0f;

	if (TimeSinceUsage > HardTimeDelta)
		return HardTimeValue;

	const float Ratio = (TimeSinceUsage - SoftTimeDelta) / (HardTimeDelta - SoftTimeDelta);
	return SoftTimeValue + (HardTimeValue - SoftTimeValue) * Ratio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPIsStandingOnGround::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	if (bInverse)
		return OwnerCharacter->GetMovementComponent()->IsMovingOnGround() ? 0.0f : TrueValue;

	return OwnerCharacter->GetMovementComponent()->IsMovingOnGround() ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPRandom::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	return Percent >= FMath::FRandRange(0.0f, 1.0f) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPBlockedRecently::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const float Delta = OwnerCharacter->GetWorld()->GetTimeSeconds() - OwnerCharacter->GetLastBlockTime();
	const bool bBlockedRecently = Delta <= MaxTimeDelta;
	return bInverse ? !bBlockedRecently : bBlockedRecently;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPLastID::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	FName LastName;
	switch (IDType)
	{
		case ESoEnemyIDType::EEIT_Strike:
			LastName = OwnerCharacter->GetStrikeName();
			break;

		case ESoEnemyIDType::EEIT_Block:
			LastName = OwnerCharacter->GetBlockName();
			break;

		case ESoEnemyIDType::EEIT_HitReact:
			LastName = OwnerCharacter->GetHitReactName();
			break;

		default:
			break;
	}
	const bool bEquals = LastName == Name;
	return bInverse ? !bEquals : bEquals;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPRangeAttackTestHit::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	const AActor* Target = OwnerCharacter->GetTarget();
	if (Target == nullptr)
		return 0.0f;

	const FVector StartPos = OwnerCharacter->GetRangeAttackLocation(Index);
	const FVector TargetPos = Target->GetActorLocation();
	TArray<const AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);
	ActorsToIgnore.Add(Target);

	bool bSuccess = false;

	bool bSplineProjectile = false;
	const FSoProjectileInitData* InitData = OwnerCharacter->GetRangeAttackInitData(Index, bSplineProjectile);
	if (InitData == nullptr)
		return 0.0f;

	if (bSplineProjectile)
	{
		const FSoSplinePoint OwnerSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(OwnerCharacter);
		const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(OwnerCharacter);
		const FVector Delta = StartPos - FVector(OwnerSplineLocation);
		const FSoSplinePoint StartSplineLocation = OwnerSplineLocation + OwnerSplineLocation.GetDirectionModifierFromVector(Delta) * Delta.Size2D();

		bSuccess = USoMathHelper::CheckRangeAttack(OwnerCharacter->GetWorld(),
													 StartPos,
													 StartSplineLocation,
													 TargetPos,
													 PlayerSplineLocation,
													 ActorsToIgnore,
													 InitData->Velocity.Size(),
													 InitData->GravityScale,
													 Precision,
													 OwnerCharacter->ShouldRangeAttackPreferLargeArc(Index) ? 1 : -1);
	}
	else
		bSuccess = USoMathHelper::CheckRangeAttack(OwnerCharacter->GetWorld(),
												   StartPos,
												   TargetPos,
												   ActorsToIgnore,
												   InitData->Velocity.Size(),
												   InitData->GravityScale,
												   Precision,
												   OwnerCharacter->ShouldRangeAttackPreferLargeArc(Index) ? 1 : -1);

	return bSuccess ? TrueValue : 0.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPFloatValueIsGreater::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	return (OwnerCharacter->GetFloatValue(Value) > CompareTo) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPFloatValueIsSmaller::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	return (OwnerCharacter->GetFloatValue(Value) < CompareTo) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPIsPlayerAlive::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(OwnerCharacter);
	return (ISoMortal::Execute_IsAlive(Player) == bShouldBeAlive) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPBlueprintCondition::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	return OwnerCharacter->CheckBlueprintConditionBP(Name) ? TrueValue : 0.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPIsPlayerSurrounded::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	ASoEnemySpawner* Spawner = OwnerCharacter->GetSoSpawner();
	if (Spawner == nullptr)
		return bInverse ? TrueValue : 0.0f;

	const TArray<ASoEnemy*> EnemyList = ASoGameMode::Get(OwnerCharacter).GetEnemiesFromGroup(OwnerCharacter->GetSoGroupName());

	if (EnemyList.Num() < 2)
		return bInverse ? TrueValue : 0.0f;

	bool bDir0 = false;
	bool bDir1 = false;

	const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(OwnerCharacter);
	for (ASoEnemy* Enemy : EnemyList)
	{
		const FSoSplinePoint& SplinePoint = Enemy->GetSoMovement()->GetSplineLocation();

		if ((SplinePoint - PlayerSplineLocation) > 0)
			bDir0 = true;
		else
			bDir1 = true;

		if (bDir0 && bDir1)
			return bInverse ? 0.0f : TrueValue;
	}

	return bInverse ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPEnemyDistanceFromPlayer::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	if (MinValue < 0 && MaxValue < 0)
		return TrueValue;

	const FSoSplinePoint PlayerSplineLocation = USoStaticHelper::GetPlayerSplineLocation(OwnerCharacter);

	const float OwnerDistance = PlayerSplineLocation - OwnerCharacter->GetSoMovement()->GetSplineLocation();
	int32 EnemyDistance = 0;
	for (ASoEnemy* Enemy : ASoGameMode::Get(OwnerCharacter).GetEnemiesFromGroup(OwnerCharacter->GetSoGroupName()))
	{
		if (Enemy != OwnerCharacter &&
			Enemy != nullptr &&
			(!bIgnoreFloatings || Enemy->GetSoMovement()->DefaultLandMovementMode != EMovementMode::MOVE_Flying) &&
			(!bOnlySameClass || Enemy->GetClass() == OwnerCharacter->GetClass()))
		{
			const float Distance = PlayerSplineLocation - Enemy->GetSoMovement()->GetSplineLocation();
			if (FMath::Sign(Distance) == FMath::Sign(OwnerDistance) &&
				fabs(Distance) < fabs(OwnerDistance))
				EnemyDistance += 1;
		}
	}


	return ((MinValue < 0 || EnemyDistance >= MinValue) && (MaxValue < 0 || EnemyDistance <= MaxValue)) ? TrueValue : 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPAorB::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	if (A == nullptr || B == nullptr)
		return 0.0f;

	return TrueValue * (FMath::Max(A->Evaluate(OwnerCharacter, OwnerAction), B->Evaluate(OwnerCharacter, OwnerAction)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEPCheckDynamicName::Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const
{
	return OwnerCharacter->GetDynamicName() == ExpectedValue ? TrueValue : 0.0f;
}
