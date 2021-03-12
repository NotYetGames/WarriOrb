// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEnemySpawnHandler.h"

#include "SplineLogic/SoSplineHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoEnemySpawnHandler::ASoEnemySpawnHandler(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemySpawnHandler::CanSpawn_Implementation() const
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoEnemy* ASoEnemySpawnHandler::SpawnCreature(TSubclassOf<ASoEnemy> Class)
{
	ASoEnemy* Enemy = SpawnCreatureBP(Class);
	if (Enemy != nullptr)
	{
		Enemy->SetTargetMarkers(MarkerList);

		if (DefaultScriptOverride != NAME_None)
			Enemy->SetActiveActionList(DefaultScriptOverride);

		if (bAllowSpawnMaterialAnimation)
		{
			const int32 MatAnimIndex = Enemy->GetSpawnMaterialAnimationIndex();
			if (MatAnimIndex > -1)
			{
				Enemy->PlayMaterialAnimation(MatAnimIndex);
				Enemy->UpdateMaterialAnimations(0.0f);
			}
		}
	}

	return Enemy;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoEnemySpawnHandler::GetPointNearTarget(UPARAM(Ref)FSoSplinePointPtr& PointOnSpline,
											  const FSoSplinePoint& Min,
											  const FSoSplinePoint& Max,
											  const FSoSplinePoint& Target,
											  float DistanceFromTarget,
											  bool bMoveActorToPoint,
											  bool bLookAtTarget,
											  int32 PreferredDirection)
{
	if (!Min.IsValid(true) || !Max.IsValid(true) || !Target.IsValid(true))
		return false;

	if (!USoSplineHelper::IsSplinepointBetweenPoints(Target, Min, Max))
		return false;


	const FSoSplinePoint First = Target + DistanceFromTarget;
	const FSoSplinePoint Second = Target - DistanceFromTarget;
	const bool bFirstGood = USoSplineHelper::IsSplinepointBetweenPoints(First, Min, Max);
	const bool bSecondGood = USoSplineHelper::IsSplinepointBetweenPoints(Second, Min, Max);

	if (!bFirstGood && !bSecondGood)
		return false;

	bool bFirstSelected = bFirstGood;
	if (bFirstGood == bSecondGood)
		bFirstSelected = FMath::RandRange(0.0f, 1.0f) < (0.5 + PreferredDirection);

	const FSoSplinePoint Selected = bFirstSelected ? First : Second;
	PointOnSpline = Selected;

	if (bMoveActorToPoint)
		SetActorLocation(Selected.GetWorldLocation(GetActorLocation().Z));

	if (bLookAtTarget)
		SetActorRotation(USoSplineHelper::GetSplineLookAtRotation(Selected, Target));

	return true;
}
