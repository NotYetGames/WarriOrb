// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoCombatComponent.h"

#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"

#include "Basic/SoCollisionChannels.h"
#include "CharacterBase/SoMortal.h"
#include "SplineLogic/SoSplineWalker.h"
#include "Character/SoCharacter.h"

bool USoCombatComponent::bDisplayDebugHitLines = false;

DEFINE_LOG_CATEGORY_STATIC(LogSoCombatComponent, All, All);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values for this component's properties
USoCombatComponent::USoCombatComponent()
{
	// bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;

	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	ActiveDamage.Physical = 20.0f;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts
//void USoCombatComponent::BeginPlay()
//{
//	Super::BeginPlay();
//
//	GetOwner();
//
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// some interfaces are expected to be implemented
	SoOwner = Cast<ASoCharacterBase>(GetOwner());
	check(SoOwner);

	OwnerSkeletal = SoOwner->GetMesh();

	WeaponShape.SetSphere(1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCombatComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );


	for (int32 i = 0; i < WeaponsDynamic.Num(); ++i)
		if (WeaponsDynamic[i].bEnabled)
		{
			WeaponShape.SetSphere(Weapons[i].TraceRadius);

			FCollisionQueryParams TraceParams(FName(TEXT("Not Yet Trace")), true, SoOwner);
			TraceParams.bTraceComplex = false;
			TraceParams.bReturnPhysicalMaterial = false;

			const FVector CurrentSocketStartPos = OwnerSkeletal->GetSocketLocation(Weapons[i].StartSocketName);

			const FVector CurrentDirection = (OwnerSkeletal->GetSocketLocation(Weapons[i].EndSocketName) - CurrentSocketStartPos).GetSafeNormal();

			if ((CurrentDirection - WeaponsDynamic[i].LastDirection).Size() < KINDA_SMALL_NUMBER &&
				(CurrentSocketStartPos - WeaponsDynamic[i].LastStartSocketPosition).Size() < KINDA_SMALL_NUMBER)
				continue;

			for (int32 j = 0; j <= Weapons[i].PartitionNum; ++j)
			{
				const float DistanceFromStart = j * Weapons[i].PartitionLength;

				const FVector StartLocation = WeaponsDynamic[i].LastStartSocketPosition + DistanceFromStart * WeaponsDynamic[i].LastDirection;
				const FVector EndLocation = CurrentSocketStartPos + DistanceFromStart * CurrentDirection;

				if (USoCombatComponent::bDisplayDebugHitLines)
				{
					const FColor Color = Cast<ASoCharacter>(SoOwner) == nullptr ? FColor(255, 0, 0) : FColor(0, 255, 0);
					DrawDebugLine(SoOwner->GetWorld(), StartLocation, EndLocation, Color, false, 3, 0, Weapons[i].TraceRadius);
				}

				TArray<FHitResult> Hits;
				SoOwner->GetWorld()->SweepMultiByChannel(Hits, StartLocation, EndLocation, {}, ECC_Weapon, WeaponShape, TraceParams);

				for (const auto& Hit : Hits)
				{
					AActor* TargetActor = Hit.Actor.Get();

					UPrimitiveComponent* Component = Hit.Component.Get();

					if (TargetActor == nullptr ||
						!TargetActor->GetClass()->ImplementsInterface(USoMortal::StaticClass()) ||
						(bOneActorOncePerStrike && ActorsAlreadyHit.Contains(FSoTargetHit{ TargetActor, i })))
						break;

					ActorsAlreadyHit.Add({ TargetActor, i });

					FSoMeleeHitParam Params;
					Params.Dmg = ActiveDamage;
					Params.HitReactDesc.AssociatedActor = GetOwner();
					Params.Hit = Hit;
					Params.HitReactDesc.HitReact = ESoHitReactType::EHR_Pushed;
					Params.HitReactDesc.PushDesc = ActivePushDesc;
					Params.HitReactDesc.Irresistibility = CurrentIrresistibility;

					for (const auto LocalHit : Hits)
						if (LocalHit.Actor.Get() == TargetActor)
							Params.HitReactDesc.AssociatedBones.Add(LocalHit.BoneName);

					// direction correction happens in char now
					// const int32 Modifier = ISoSplineWalker::Execute_GetSplineLocationI(SoOwner).GetDirectionModifierFromVector(TargetActor->GetActorLocation() - GetOwner()->GetActorLocation());
					// Params.HitReactDesc.PushDesc.HorizontalVelocity *= Modifier;
					Params.bBlockable = bBlockable;

					if (!ISoMortal::Execute_MeleeHit(TargetActor, Params))
						SoOwner->OnBlocked();

					SoOwner->OnMeleeHit.Broadcast(TargetActor, Params);
				}
			}

			WeaponsDynamic[i].LastDirection = CurrentDirection;
			WeaponsDynamic[i].LastStartSocketPosition = CurrentSocketStartPos;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCombatComponent::RegisterWeapon(FName StartSocketName, FName EndSocketName, int32 PartitionNum, float TraceRadius)
{
	PartitionNum = FMath::Max(PartitionNum, 1);

	FSoWeapon Weapon;

	Weapon.StartSocketName = StartSocketName;
	Weapon.EndSocketName = EndSocketName;
	Weapon.PartitionNum = PartitionNum;
	Weapon.TraceRadius = TraceRadius;

	Weapon.PartitionLength = (OwnerSkeletal->GetSocketLocation(EndSocketName) - OwnerSkeletal->GetSocketLocation(StartSocketName)).Size() / PartitionNum;

	Weapons.Add(Weapon);
	WeaponsDynamic.Add({ false, {}, {} });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCombatComponent::SetupNewStrike(const FSoStrikePhase& StrikeData)
{
	ClearActorsAlreadyHit();
	ActivePushDesc = StrikeData.PushDesc;
	bBlockable = StrikeData.bBlockable;
	ActiveDamage = StrikeData.Damage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCombatComponent::SetWeaponCollisionEnabled(int32 Index, bool bEnabled)
{
	if (WeaponsDynamic.IsValidIndex(Index))
	{
		WeaponsDynamic[Index].bEnabled = bEnabled;
		WeaponsDynamic[Index].LastStartSocketPosition = OwnerSkeletal->GetSocketLocation(Weapons[Index].StartSocketName);
		WeaponsDynamic[Index].LastDirection = (OwnerSkeletal->GetSocketLocation(Weapons[Index].EndSocketName) - WeaponsDynamic[Index].LastStartSocketPosition).GetSafeNormal();
	}
	else
		UE_LOG(LogSoCombatComponent, Warning, TEXT("USoCombatComponent::SetWeaponCollisionEnabled called with invalid weapon index"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoCombatComponent::DisableAllWeaponCollision()
{
	for (auto& Weapon : WeaponsDynamic)
		Weapon.bEnabled = false;
}
