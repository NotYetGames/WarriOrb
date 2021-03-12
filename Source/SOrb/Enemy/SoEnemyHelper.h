// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterBase/SoIMortalTypes.h"

#include "SoEnemyHelper.generated.h"


struct FSoSplinePoint;
class ASoEnemy;

/**
 *
 */
UCLASS()
class SORB_API USoEnemyHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = EnemyHelper, meta = (WorldContext = "WorldContextObject"))
	static void DisplayDamageText(UObject* WorldContextObject,
								  const FSoDmg& Damage,
								  const FVector& BaseLocation,
								  const FVector ForwardDirRef,
								  bool bWasHitThisFrame,
								  bool bCritical,
								  bool bOnPlayer = false);

	static void ForceDistanceOnFlyingEnemies(const UObject* WorldContextObject, TSubclassOf<ASoEnemy> Enemies, float MinDistance, float MaxDelta);

	static float GetSignedSplineDistanceToClosestEnemy(const ASoEnemy* Source, bool bIgnoreFloatings, bool bOnlySameClass);

private:
	static void PushActor(AActor* Actor,
						  const FSoSplinePoint& SplineLocation,
						  const FVector2D& Pos,
						  const FVector2D& OtherActorPos,
						  const FVector2D& PlayerPos,
						  float DesiredDistance,
						  float MaxDelta,
						  const FVector& OtherActorLocation);

public:

	static bool bDrawDebugModeOn;// = false;

};
