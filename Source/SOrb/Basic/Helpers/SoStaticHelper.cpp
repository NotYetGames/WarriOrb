// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoStaticHelper.h"

#include "EngineUtils.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "Components/SplineComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/WidgetAnimation.h"
#include "Misc/Paths.h"
#include "Engine/EngineTypes.h"

#include "DlgSystem/Public/IO/DlgConfigWriter.h"
#include "DlgSystem/Public/DlgContext.h"

#include "Basic/SoCollisionChannels.h"
#include "Basic/SoGameMode.h"
#include "Levels/SoLevelManager.h"
#include "Character/SoCharacter.h"
#include "SplineLogic/SoSplineWalker.h"
#include "SplineLogic/SoSplinePointPtr.h"
#include "Enemy/SoEnemy.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Enemy/SoEnemySpawner.h"
#include "Levels/SoLevelHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Basic/SoAudioManager.h"
#include <cwctype>

#if WITH_EDITOR
#include "Engine/HLODProxy.h"
#endif

const FName USoStaticHelper::NameGold = FName("Gold");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoGameMode* USoStaticHelper::GetSoGameMode(const UObject* WorldContextObject)
{
	return ASoGameMode::GetInstance(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoGameMode* USoStaticHelper::GetSoGameModeFromContext(const UObject* Context)
{
	return GetSoGameMode(Context);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoTimerManager& USoStaticHelper::GetSoTimerManager(const UObject* WorldContextObject)
{
	return ASoGameMode::Get(WorldContextObject).GetTimerManager();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoStaticHelper::GetNameFromActor(const UObject* WorldContextObject, FString PreFix)
{
	if (WorldContextObject == nullptr)
		return NAME_None;

	if (PreFix.Len() == 0)
		return WorldContextObject->GetFName();

	return FName(*(PreFix + WorldContextObject->GetName()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoStaticHelper::IsActorInPlay(const UObject* WorldContextObject)
{
	if (const AActor* Actor = Cast<AActor>(WorldContextObject))
		return Actor->HasActorBegunPlay();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
APlayerController* USoStaticHelper::GetPlayerController(const UObject* WorldContextObject)
{
	if (!GEngine || !WorldContextObject)
		return nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		return World->GetFirstPlayerController();

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AActor* USoStaticHelper::GetPlayerCharacterAsActor(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = GetPlayerController(WorldContextObject))
		return Cast<AActor>(PlayerController->GetPawn());

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoCharacter* USoStaticHelper::GetPlayerCharacterAsSoCharacter(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = GetPlayerController(WorldContextObject))
		return Cast<ASoCharacter>(PlayerController->GetPawn());

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoCharacterBase* USoStaticHelper::GetPlayerCharacterAsSoCharacterBase(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = GetPlayerController(WorldContextObject))
		return Cast<ASoCharacterBase>(PlayerController->GetPawn());

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AActor* USoStaticHelper::GetPlayerCharacterAsNullptr(const UObject* WorldContextObject)
{
	// good luck with this one
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoEnemy* USoStaticHelper::GetClosestEnemyToLocation(const UObject* WorldContextObject, const FVector& WorldLocation, float MaxDistance, bool bPreferFlying)
{
	ASoEnemy* Closest = nullptr;
	const float MaxDistancePow2 = MaxDistance * MaxDistance;
	float BestDistancePow2 = MaxDistancePow2;
	bool bBestFlying = false;
	for (TActorIterator<ASoEnemy> Itr(WorldContextObject->GetWorld()); Itr; ++Itr)
	{
		ASoEnemy* Enemy = *Itr;
		if (Enemy != nullptr && !Enemy->IsPendingKill() && Enemy->IsAlive() && !Enemy->IsInAnotherDimension())
		{
			const FName BoneName = Enemy->GetBoneToTarget();
			const FVector TargetLocation = BoneName == NAME_None ? Enemy->GetActorLocation() : Enemy->GetMesh()->GetSocketLocation(BoneName);

			const float DistancePow2 = (TargetLocation - WorldLocation).SizeSquared();
			if (bPreferFlying && DistancePow2 < MaxDistancePow2)
			{
				const bool bFlying = Enemy->GetSoMovement()->DefaultLandMovementMode == EMovementMode::MOVE_Flying;
				if (bBestFlying == bFlying)
				{
					if (DistancePow2 < BestDistancePow2)
					{
						BestDistancePow2 = DistancePow2;
						Closest = Enemy;
					}
				}
				else
				{
					if (bFlying)
					{
						bBestFlying = true;
						BestDistancePow2 = DistancePow2;
						Closest = Enemy;
					}
				}
			}
			else
			{
				if (DistancePow2 < BestDistancePow2)
				{
					BestDistancePow2 = DistancePow2;
					Closest = Enemy;
				}
			}
		}
	}

	return Closest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoEnemy* USoStaticHelper::GetClosestEnemyInDirection(const UObject* WorldContextObject,
													  const FVector& WorldLocation,
													  float MaxDistance,
													  const FVector& Direction,
													  bool bOnlyMovable)
{
	ASoEnemy* Closest = nullptr;
	const float MaxDistancePow2 = MaxDistance * MaxDistance;
	float BestDistancePow2 = MaxDistancePow2;
	for (TActorIterator<ASoEnemy> Itr(WorldContextObject->GetWorld()); Itr; ++Itr)
	{
		ASoEnemy* Enemy = *Itr;
		if (Enemy != nullptr && !Enemy->IsPendingKill() && Enemy->IsAlive() && (!bOnlyMovable || Enemy->CanBeMovedByPlayer()))
		{
			const FName BoneName = Enemy->GetBoneToTarget();
			const FVector TargetLocation = BoneName == NAME_None ? Enemy->GetActorLocation() : Enemy->GetMesh()->GetSocketLocation(BoneName);

			FVector SourceToTarget = TargetLocation - WorldLocation;
			const float DistancePow2 = SourceToTarget.SizeSquared();
			if (DistancePow2 < BestDistancePow2)
			{
				SourceToTarget.Z = 0.0f;
				if ((Direction | SourceToTarget.GetSafeNormal()) >= 0.0f)
				{
					BestDistancePow2 = DistancePow2;
					Closest = Enemy;
				}
			}
		}
	}

	return Closest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoStaticHelper::GetEnemiesInRadius(const UObject* WorldContextObject, const FVector& WorldLocation, float Radius, TArray<ASoEnemy*>& Enemies)
{
	ASoEnemy* Closest = nullptr;
	const float MaxDistancePow2 = Radius * Radius;
	for (TActorIterator<ASoEnemy> Itr(WorldContextObject->GetWorld()); Itr; ++Itr)
	{
		ASoEnemy* Enemy = *Itr;
		if (Enemy != nullptr && !Enemy->IsPendingKill() && Enemy->IsAlive() && !Enemy->IsSoPaused())
		{
			const FName BoneName = Enemy->GetBoneToTarget();
			const FVector TargetLocation = BoneName == NAME_None ? Enemy->GetActorLocation() : Enemy->GetMesh()->GetSocketLocation(BoneName);
			const float DistancePow2 = (TargetLocation - WorldLocation).SizeSquared();
			if (DistancePow2 < MaxDistancePow2)
				Enemies.Add(Enemy);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoStaticHelper::GetPointOnBoxTopClosestToBone(FVector& OutLocation, const TArray<FName>& BoneList, USkeletalMeshComponent* Mesh, UBoxComponent* Box, float MaxAcceptedOffset)
{
	if (Mesh == nullptr || Box == nullptr)
		return false;

	const FVector BoxCenter = Box->GetComponentLocation();
	const FVector ScaledBoxExtend = Box->GetScaledBoxExtent();

	const float ZLevel = BoxCenter.Z + ScaledBoxExtend.Z;

	const FVector BoxForwardVector = Box->GetForwardVector();
	const FVector BoxRightVector = Box->GetRightVector();

	const FVector2D A = -FVector2D(BoxForwardVector) * ScaledBoxExtend.X - FVector2D(BoxRightVector) * ScaledBoxExtend.Y;
	const FVector2D B = FVector2D(BoxForwardVector) * ScaledBoxExtend.X - FVector2D(BoxRightVector) * ScaledBoxExtend.Y;
	// const FVector2D C = FVector2D(BoxForwardVector) * ScaledBoxExtend.X + FVector2D(BoxRightVector) * ScaledBoxExtend.Y;
	const FVector2D D = -FVector2D(BoxForwardVector) * ScaledBoxExtend.X + FVector2D(BoxRightVector) * ScaledBoxExtend.Y;

	const FVector2D AB = B - A;
	const FVector2D AD = D - A;

	FVector BestWorldLocation = BoxCenter;
	float BestDistance = BIG_NUMBER;

	for (const FName Bone : BoneList)
	{
		const FVector WorldLocation = Mesh->GetSocketLocation(Bone);
		const FVector TranslatedWorldLocation = WorldLocation - BoxCenter;
		const FVector2D TranslatedWorldLocation2D = FVector2D(TranslatedWorldLocation);
		const FVector2D AM = TranslatedWorldLocation2D - A;

		// https://math.stackexchange.com/questions/190111/how-to-check-if-a-point-is-inside-a-rectangle/190373#190373
		const float AMDotAB = AM | AB;
		const float AMDotAD = AM | AB;
		// check if it is inside the rectangle
		if ((AMDotAB > 0.0f && AMDotAB < (AB | AB)) && (AMDotAD > 0.0f) && AMDotAD < (AD | AD))
		{
			const float DistanceZ = fabs(WorldLocation.Z - ZLevel);
			if (DistanceZ < BestDistance)
			{
				BestDistance = DistanceZ;
				BestWorldLocation = WorldLocation;
			}
		}
	}

	if (BestDistance < MaxAcceptedOffset)
	{
		OutLocation = BestWorldLocation;
		OutLocation.Z = ZLevel;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint USoStaticHelper::GetPlayerSplineLocation(const UObject* WorldContextObject)
{
	// maybe simulation
	if (AActor* Player = USoStaticHelper::GetPlayerCharacterAsActor(WorldContextObject))
		return ISoSplineWalker::Execute_GetSplineLocationI(Player);

	return FSoSplinePoint();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoStaticHelper::GetEnemyOverlapDamageImmunityName()
{
	static FName EnemyOverlapDamageImmunityName = FName("EnemyOverlapDamageImmunity");
	return EnemyOverlapDamageImmunityName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoStaticHelper::GetLillianFormName()
{
	static FName LillianFormName = FName("bLillianFormReached");
	return LillianFormName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoStaticHelper::GetForceNoEnvMusicName()
{
	static FName ForceNoEnvMusicName = FName("bForceNoEnvMusic");
	return ForceNoEnvMusicName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoDmgType USoStaticHelper::GetDmgTypeFromDmg(const FSoDmg& Dmg)
{
	return Dmg.GetType();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoStaticHelper::CheckAndClaimLevelsInSplineLocation(const FSoSplinePoint& SplinePoint)
{
	if (!SplinePoint.IsValid(true))
		return false;

	bool bLoaded;
	return FSoLevelManager::Get().ClaimSplineLocation(SplinePoint, bLoaded);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoStaticHelper::CalcDialogueDisplayTime(const UDlgContext* Context)
{
	ensure(Context);
	const int32 Length = Context->GetActiveNodeText().ToString().Len();
	return FMath::Max(Length / 50.0f * 3.0f, 3.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoStaticHelper::GenerateNameFromAssetPath(UObject* Object)
{
	if (Object == nullptr)
		return NAME_None;

	const FString FileName = Object->GetPathName();

	// get rid of the filename.extension from the end
	int32 End = FileName.Len() - 1;
	while (End > 0 && FileName[End] != '.')
		End -= 1;

	// get rid of the game/ from the begining
	int32 Start = End;
	while (Start > 0 && FileName[Start - 1] != '/')
		Start -= 1;

	if (End == 0 || Start >= End)
		return NAME_None;

	return FName(*FileName.Mid(Start, End - Start));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoStaticHelper::GetScaledAnimLength(UAnimSequenceBase* AnimSequenceBase)
{
	if (AnimSequenceBase == nullptr)
		return 0.f;

	return AnimSequenceBase->SequenceLength / AnimSequenceBase->RateScale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoStaticHelper::BuildAnimationMap(UObject* Widget, TMap<FName, UWidgetAnimation*>& AnimationMap)
{
	AnimationMap.Empty();

	UProperty* Property = Widget->GetClass()->PropertyLink;
	while (Property != nullptr)
	{
		if (Property->GetClass() == UObjectProperty::StaticClass())
		{
			UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Property);

			if (ObjectProperty->PropertyClass == UWidgetAnimation::StaticClass())
			{
				UObject* Object = ObjectProperty->GetObjectPropertyValue_InContainer(Widget);
				UWidgetAnimation* WidgetAnimation = Cast<UWidgetAnimation>(Object);

				if (WidgetAnimation != nullptr)
					AnimationMap.Add(WidgetAnimation->GetMovieScene()->GetFName(), WidgetAnimation);
			}
		}

		Property = Property->PropertyLinkNext;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoStaticHelper::SetSceneComponentMobility(USceneComponent* SceneComponent, EComponentMobility::Type Mobility)
{
	if (SceneComponent != nullptr)
		SceneComponent->SetMobility(Mobility);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoStaticHelper::GetLastRenderTimeOnScreen(UPrimitiveComponent* PrimitiveComponent)
{
	return PrimitiveComponent->LastRenderTimeOnScreen;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoStaticHelper::WasPrimitiveRenderedOnScreenRecently(UPrimitiveComponent* PrimitiveComponent, float Recently)
{
	return (PrimitiveComponent->GetWorld()->GetTimeSeconds() - PrimitiveComponent->LastRenderTimeOnScreen) < Recently;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoStaticHelper::GetBoneNameFromBodyIndex(USkeletalMeshComponent* SkeletalMeshComponent, int32 BodyIndex)
{
	if (SkeletalMeshComponent != nullptr && SkeletalMeshComponent->Bodies.IsValidIndex(BodyIndex))
		return SkeletalMeshComponent->GetBoneName(SkeletalMeshComponent->Bodies[BodyIndex]->InstanceBoneIndex);

	return NAME_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoStaticHelper::DamageOverlappedMortals(UPrimitiveComponent* PrimitiveComponent,
											  const FSoDmg& Damage,
											  const FSoHitReactDesc& HitReactDesc)
{
	if (PrimitiveComponent == nullptr)
		return;

	const ECollisionEnabled::Type OldState = PrimitiveComponent->GetCollisionEnabled();

	PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	const FComponentQueryParams Param = FComponentQueryParams::DefaultComponentQueryParams;
	TArray<struct FOverlapResult> OutOverlaps;
	PrimitiveComponent->ComponentOverlapMulti(	OutOverlaps,
												PrimitiveComponent->GetWorld(),
												PrimitiveComponent->GetComponentLocation(),
												PrimitiveComponent->GetComponentTransform().GetRotation(),
												ECC_Weapon,
												Param,
												FCollisionObjectQueryParams::DefaultObjectQueryParam);
	PrimitiveComponent->SetCollisionEnabled(OldState);

	TMap<AActor*, FSoHitReactDesc> HitList;

	for (const FOverlapResult& Overlap : OutOverlaps)
		if (UPrimitiveComponent* Component = Overlap.Component.Get())
		{
			AActor* Actor = Overlap.Actor.Get();
			if (Actor->GetClass()->ImplementsInterface(USoMortal::StaticClass()))
			{
				FSoHitReactDesc* HitParamPtr = HitList.Find(Actor);
				if (HitParamPtr == nullptr)
				{
					HitParamPtr = &HitList.Add(Actor);
					*HitParamPtr = HitReactDesc;
				}
				HitParamPtr->AssociatedBones.Add(USoStaticHelper::GetBoneNameFromBodyIndex(Cast<USkeletalMeshComponent>(Overlap.Component.Get()), Overlap.ItemIndex));
			}
		}

	for (const auto& Pair : HitList)
		ISoMortal::Execute_CauseDmg(Pair.Key, Damage, Pair.Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector USoStaticHelper::GetClosestPointOnMesh(USkeletalMeshComponent* Mesh, const FVector& SourceLocation)
{
	FVector MinLocation = FVector(0.0f, 0.0f, 0.0f);
	float MinDist = BIG_NUMBER;

	TArray<FName> BoneNames;
	Mesh->GetBoneNames(BoneNames);

	for (const FName BoneName : BoneNames)
	{
		FVector Value;
		const float Dist = Mesh->GetClosestPointOnCollision(SourceLocation, Value, BoneName);
		if (Dist > 0.0f && Dist < MinDist)
		{
			MinDist = Dist;
			MinLocation = Value;
		}
	}

	return MinLocation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoStaticHelper::ClearAndInvalidateTimer(AActor* Actor, FTimerHandle& Handle)
{
	if (Handle.IsValid())
	{
		UWorld* World = GEngine->GetWorldFromContextObject(Actor, EGetWorldErrorMode::LogAndReturnNull);
		if (World)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoStaticHelper::CallMarkPackageDirtyOnObject(const UObject* Object)
{
	if (Object != nullptr)
		Object->MarkPackageDirty();
}


#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoStaticHelper::AddActorAndMeshToHLODProxy(UObject* Proxy, ALODActor* InLODActor, UStaticMesh* InStaticMesh)
{
	if (Proxy != nullptr && InLODActor != nullptr && InStaticMesh != nullptr)
		if (UHLODProxy* HLODProxy = Cast<UHLODProxy>(Proxy))
		{
			HLODProxy->AddMesh(InLODActor, InStaticMesh, InLODActor->GetKey());
		}
}
#endif
