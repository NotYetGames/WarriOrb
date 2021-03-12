// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEnemyHelper.h"

#include "Kismet/GameplayStatics.h"

#include "DrawDebugHelpers.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameMode.h"
#include "SoEnemy.h"
#include "SplineLogic/SoSplineWalker.h"
#include "SplineLogic/SoSpline.h"
#include "CharacterBase/SoCharacterMovementComponent.h"

bool USoEnemyHelper::bDrawDebugModeOn = false;

struct FSoActorCluster
{
	TArray<AActor*> Actors;
};

struct FSoActorWith2DPos
{
	AActor* Actor;
	FVector2D Pos;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEnemyHelper::DisplayDamageText(UObject* WorldContextObject,
										const FSoDmg& Damage,
										const FVector& BaseLocation,
										const FVector ForwardDirRef,
										bool bWasHitThisFrame,
										bool bCritical,
										bool bOnPlayer)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Helper - DisplayDamageText"), STAT_Helper_DisplayDamageText, STATGROUP_SoEnemy);

	if (bOnPlayer)
	{
		for (int32 i = 0; i < 2; ++i)
		{
			FVector Offset = FVector(0.0f, 0.0f, 0.0f);

			if (Damage.Magical > KINDA_SMALL_NUMBER && Damage.Physical > KINDA_SMALL_NUMBER)
			{
				Offset = ForwardDirRef * ((i % 2 == 0) ? 40 : -40);
				if (bWasHitThisFrame)
					Offset.Z += FMath::FRandRange(50.0f, 60.0f);
			}
			else
				Offset = FMath::VRand() * 40;

			if (Damage[i] > 0.0f)
				ASoGameMode::Get(WorldContextObject).DisplayDamageText(BaseLocation + Offset, Damage[i], i == 0, bOnPlayer, bCritical);

			bWasHitThisFrame = true;
		}
	}
	else
	{
		const FVector Offset = FMath::VRand() * 40;
		ASoGameMode::Get(WorldContextObject).DisplayDamageText(BaseLocation + Offset, Damage.Sum(), true, false, bCritical);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEnemyHelper::ForceDistanceOnFlyingEnemies(const UObject* WorldContextObject, TSubclassOf<ASoEnemy> Class, float MinDistance, float MaxDelta)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Helper - ForceDistanceOnFlyingEnemies"), STAT_Helper_ForceDistanceOnFlyingEnemies, STATGROUP_SoEnemy);

	TArray<ASoEnemy*> Enemies = ASoGameMode::Get(WorldContextObject).GetEnemies();
	// remove irrelevant ones
	for (int32 i = Enemies.Num() - 1; i >= 0; --i)
		if (!Enemies[i]->CanBeMovedByLogic() || !Enemies[i]->IsAlive())
			Enemies.RemoveAtSwap(i);

	// gather close to eachother ones in groups:

	TArray<FSoActorCluster> EnemyClusters;
	TMap<AActor*, FSoSplinePoint> PositionMap;
	for (AActor* Enemy : Enemies)
		PositionMap.Add(Enemy, ISoSplineWalker::Execute_GetSplineLocationI(Enemy));

	// first pass: check all enemy against all other and create initial "we are close to each other" groups
	const float MinDistanceSquered = MinDistance * MinDistance;
	for (int32 i = 0; i < Enemies.Num(); ++i)
		for (int32 j = 0; j < Enemies.Num(); ++j)
			if (i != j &&
				PositionMap[Enemies[i]].GetDistanceFromSplinePointWithZSquered(PositionMap[Enemies[j]]) < MinDistanceSquered &&
				PositionMap[Enemies[i]].GetSpline() == PositionMap[Enemies[j]].GetSpline())
			{
				bool bFound = false;
				for (FSoActorCluster& Cluster : EnemyClusters)
					if (Cluster.Actors.Contains(Enemies[i]) || Cluster.Actors.Contains(Enemies[j]))
					{
						bFound = true;
						Cluster.Actors.AddUnique(Enemies[i]);
						Cluster.Actors.AddUnique(Enemies[j]);
					}

				if (!bFound)
				{
					TArray<AActor*>& NewCluster = EnemyClusters.Add_GetRef({}).Actors;
					NewCluster.Add(Enemies[i]);
					NewCluster.Add(Enemies[j]);
				}
			}

	// 2 merge groups if there is a connection between them
	bool bFound = true;
	while (bFound)
	{
		bFound = false;
		for (int32 i = 0; !bFound && i < EnemyClusters.Num(); ++i)
			for (int32 j = i + 1; !bFound && j < EnemyClusters.Num(); ++j)
				for (int x = 0; !bFound && x < EnemyClusters[i].Actors.Num(); ++x)
					for (int y = 0; !bFound && y < EnemyClusters[j].Actors.Num(); ++y)
						if (PositionMap[EnemyClusters[i].Actors[x]].GetDistanceFromSplinePointWithZSquered(PositionMap[EnemyClusters[j].Actors[y]]) < MinDistanceSquered &&
							PositionMap[EnemyClusters[i].Actors[x]].GetSpline() == PositionMap[EnemyClusters[j].Actors[y]].GetSpline())
						{
							bFound = true;
							for (int32 k = 0; k < EnemyClusters[j].Actors.Num(); ++k)
								EnemyClusters[i].Actors.AddUnique(EnemyClusters[j].Actors[k]);
							EnemyClusters.RemoveAtSwap(j);
						}
	}

	// move enemies inside clusters:
	const FSoSplinePoint PlayerSplinePoint = USoStaticHelper::GetPlayerSplineLocation(WorldContextObject);
	for (FSoActorCluster& Cluster : EnemyClusters)
	{
		// order them based on angle from player (clockwise/counterclockwise, idk, does not matter)
		TArray<FSoActorWith2DPos> VectorList;
		for (AActor* Actor : Cluster.Actors)
			VectorList.Add({Actor, FVector2D(PlayerSplinePoint - PositionMap[Actor], PlayerSplinePoint.GetReferenceZ() - PositionMap[Actor].GetReferenceZ()).GetSafeNormal()});

		const FVector2D ReferenceVec = VectorList[0].Pos;
		VectorList.Sort([&ReferenceVec](const FSoActorWith2DPos& One, const FSoActorWith2DPos& Two)
		{
			return FVector2D::CrossProduct(One.Pos, ReferenceVec) > FVector2D::CrossProduct(Two.Pos, ReferenceVec);
		});

		// some debug helper for the ordering
		if (bDrawDebugModeOn)
			for (int32 i = 0; i < VectorList.Num(); ++i)
			{
				DrawDebugString(VectorList[0].Actor->GetWorld(), VectorList[i].Actor->GetActorLocation(),
					FString::FromInt(i) + " : " + FString::SanitizeFloat(FVector2D::CrossProduct(VectorList[i].Pos, ReferenceVec)),
					nullptr,
					FColor::White,
					0.1f);

				DrawDebugLine(VectorList[0].Actor->GetWorld(),
							  VectorList[i].Actor->GetActorLocation(),
							  USoStaticHelper::GetPlayerCharacterAsActor(WorldContextObject)->GetActorLocation(),
							  FColor::Blue, false, 0.1f);
			}


		auto GetPosFromSplineLoc = [](const FSoSplinePoint& SplineLoc) -> FVector2D
		{
			return FVector2D(SplineLoc.GetDistance(), SplineLoc.GetReferenceZ());
		};
		const FVector2D PlayerPos = GetPosFromSplineLoc(PlayerSplinePoint);
		auto Push = [&PositionMap, &GetPosFromSplineLoc, &PlayerPos, &MinDistance, &MaxDelta](AActor* ActorToPush, AActor* ActorToPushFrom)
		{
			const FSoSplinePoint& SplineLocation = PositionMap[ActorToPush];
			PushActor(ActorToPush,
					  SplineLocation,
					  GetPosFromSplineLoc(SplineLocation),
					  GetPosFromSplineLoc(PositionMap[ActorToPushFrom]),
					  PlayerPos,
					  MinDistance,
					  MaxDelta,
					  ActorToPushFrom->GetActorLocation());
		};

		const int32 Mid = (VectorList.Num() + 1) / 2;
		if (VectorList.Num() % 2 == 0)
		{
			// simply push the one who is above the other
			AActor* Actor0 = VectorList[Mid - 1].Actor;
			AActor* Actor1 = VectorList[Mid].Actor;
			const bool bPushActor0 = Actor0->GetActorLocation().Z > Actor1->GetActorLocation().Z;
			Push(bPushActor0 ? Actor0 : Actor1, bPushActor0 ? Actor1 : Actor0);

			for (int32 i = 2; i <= Mid; ++i)
			{
				Push(VectorList[Mid - i].Actor, VectorList[Mid - i + 1].Actor);
				Push(VectorList[Mid + i - 1].Actor, VectorList[Mid + i - 2].Actor);
			}
		}
		else
		{
			for (int32 i = 1; i < Mid; ++i)
			{
				Push(VectorList[Mid - i - 1].Actor, VectorList[Mid - i].Actor);
				Push(VectorList[Mid + i - 1].Actor, VectorList[Mid + i - 2].Actor);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEnemyHelper::PushActor(AActor* Actor,
							   const FSoSplinePoint& SplineLocation,
							   const FVector2D& Pos,
							   const FVector2D& OtherActorPos,
							   const FVector2D& PlayerPos,
							   float DesiredDistance,
							   float MaxDelta,
							   const FVector& OtherActorLocation)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Helper - PushActor"), STAT_Helper_PushActor, STATGROUP_SoEnemy);

	const FVector OldPos = Actor->GetActorLocation();

	// normal of the line we want the actor to push on - it is perpendicular to the normal movement direction (which is towards the player)
	FVector2D N = (PlayerPos - Pos).GetSafeNormal();

	// Nx * X + Ny * Y = Nx * X0 + Ny * Y0, point is Pos
	// (X-X1)^2 + (Y-Y1)^2 = r^2, circle is around OtherActorPos

	//...
	// X = Nx*X0...

	// just a random helper variable
	const float H = Pos.X + (N.Y / N.X) * Pos.Y - OtherActorPos.X;

	// a*x^2 + bx + c = 0
	const float A = 1 + N.Y * N.Y / (N.X * N.X);
	const float B = -2 * (H * (N.Y / N.X) + OtherActorPos.Y);
	const float C = OtherActorPos.Y * OtherActorPos.Y + H * H - DesiredDistance * DesiredDistance;

	const float D = B * B - 4 * A * C;

	// nothing to do here
	if (D <= 0.0f)
		return;

	const float Ya = (-B + FMath::Sqrt(D)) / (2 * A);
	const float Yb = (-B - FMath::Sqrt(D)) / (2 * A);

	const float Xa = Pos.X + (N.Y / N.X) * (Pos.Y - Ya);
	const float Xb = Pos.X + (N.Y / N.X) * (Pos.Y - Yb);

	// checks:
	auto CircleCheck = [&OtherActorPos, &DesiredDistance](float X, float Y)
	{
		const float CircleCheckSum = (X - OtherActorPos.X) * (X - OtherActorPos.X) + (Y - OtherActorPos.Y) * (Y - OtherActorPos.Y);
		// verify(FMath::IsNearlyEqual(CircleCheckSum, DesiredDistance * DesiredDistance, 5.0f));
		const float Error = fabs(CircleCheckSum - DesiredDistance * DesiredDistance);
		if (Error < 3.0f)
			UE_LOG(LogTemp, Warning, TEXT("Circle error: %f"), Error);
	};

	auto LineCheck = [&N, &Pos](float X, float Y)
	{
		const float LineCheckSum0 = N.X * X + N.Y * Y;
		const float LineCheckSum1 = N.X * Pos.X + N.Y * Pos.Y;
		// verify(FMath::IsNearlyEqual(LineCheckSum0, LineCheckSum1, 5.0f));

		const float Error = fabs(LineCheckSum0 - LineCheckSum1);
		if (Error < 3.0f)
			UE_LOG(LogTemp, Warning, TEXT("Line error: %f"), Error);
	};

	if (bDrawDebugModeOn)
	{
		CircleCheck(Xa, Ya);
		CircleCheck(Xb, Yb);

		LineCheck(Xa, Ya);
		LineCheck(Xb, Yb);
	}

	const float DistA = (Pos - FVector2D(Xa, Ya)).Size();
	const float DistB = (Pos - FVector2D(Xa, Yb)).Size();

	const FVector2D Target = DistA < DistB ? FVector2D(Xa, Ya) : FVector2D(Xb, Yb);

	const FVector2D DesiredDelta = (Target - Pos);
	const float DeltaSize = FMath::Min(DesiredDelta.Size(), MaxDelta);
	const FVector2D Delta = DesiredDelta.GetSafeNormal() * DeltaSize;

	const FSoSplinePoint NewSplinePoint = SplineLocation + Delta.X * SplineLocation.GetSpline()->GetSplineDirection();
	Actor->SetActorLocation(NewSplinePoint.GetWorldLocation(NewSplinePoint.GetReferenceZ() + Delta.Y), true);
	ISoSplineWalker::Execute_SetSplineLocation(Actor, NewSplinePoint, false);

	if (bDrawDebugModeOn)
	{
		// circle around actor from push
		DrawDebugCircle(Actor->GetWorld(), OtherActorLocation, DesiredDistance, 32, FColor::Green, false, 0.1f, 0, 0.f, NewSplinePoint.GetDirection());
		// old pos - character line
		DrawDebugLine(Actor->GetWorld(), OldPos, USoStaticHelper::GetPlayerCharacterAsActor(Actor)->GetActorLocation(), FColor::Blue, false, 0.1f);
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoEnemyHelper::GetSignedSplineDistanceToClosestEnemy(const ASoEnemy* Source, bool bIgnoreFloatings, bool bOnlySameClass)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Helper - GetSignedSplineDistanceToClosestEnemy"), STAT_Helper_GetSignedSplineDistanceToClosestEnemy, STATGROUP_SoEnemy);

	float ClosestDistance = BIG_NUMBER;
	const FSoSplinePoint SourceSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Source);
	for (ASoEnemy* Enemy : ASoGameMode::Get(Source).GetEnemiesFromGroup(Source->GetSoGroupName()))
	{
		if (Enemy != Source &&
			Enemy != nullptr &&
			(!bIgnoreFloatings || Enemy->GetSoMovement()->DefaultLandMovementMode != EMovementMode::MOVE_Flying) &&
			(!bOnlySameClass || Enemy->GetClass() == Source->GetClass()))
		{
			const float DistanceSigned = SourceSplineLocation - Enemy->GetSoMovement()->GetSplineLocation();
			if (fabs(DistanceSigned) < fabs(ClosestDistance))
				ClosestDistance = DistanceSigned;
		}
	}


	return ClosestDistance;
}
