// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Enemy/SoEnemyDataTypes.h"
#include "WorldCollision.h"
#include "SoCombatComponent.generated.h"

class ASoCharacterBase;

/**
 * Struct to store hits
 * Used to make sure that one weapon can only damage one target once in one strike
 */
USTRUCT(BlueprintType)
struct FSoTargetHit
{
	GENERATED_USTRUCT_BODY()
public:
	/** The actor we already damaged */
	UPROPERTY()
	AActor* Target;

	/** The weapon slot which already hit the actor */
	UPROPERTY()
	int32 WeaponCollisionIndex;
};

/**
 * TODO: Write comment
 */
struct FSoWeapon
{
	FName StartSocketName;
	FName EndSocketName;
	float WeaponLength;
	/** amount of line trace source/end points along weapon, at least 2 */
	int32 PartitionNum;
	float PartitionLength;

	float TraceRadius;
};


struct FSoWeaponDynamicData
{
	bool bEnabled;

	FVector LastStartSocketPosition;
	FVector LastDirection;
};


inline bool operator==(const FSoTargetHit& _First, const FSoTargetHit& _Second) { return _First.Target == _Second.Target && _First.WeaponCollisionIndex == _Second.WeaponCollisionIndex; }


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SORB_API USoCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USoCombatComponent();

	// Called when the game starts
	virtual void InitializeComponent() override;

	// virtual void BeginPlay() override;
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UFUNCTION(BlueprintCallable, Category = Combat)
	void RegisterWeapon(FName StartSocketName, FName EndSocketName, int32 PartitionNum, float TraceRadius);

	void ClearActorsAlreadyHit();

	void SetActivePushDesc(const FSoPushDesc& PushDesc);
	void SetBlockable(bool bInBlockable);

	void SetupNewStrike(const FSoStrikePhase& StrikeData);

	UFUNCTION(BlueprintCallable, Category = Combat)
	void SetCurrentIrresistibility(int32 Irresistibility) { CurrentIrresistibility = Irresistibility; }

	UFUNCTION(BlueprintCallable, Category = Combat)
	void ModifyCurrentIrresistibility(int32 IrresistibilityChange) { CurrentIrresistibility += IrresistibilityChange; }

	void SetWeaponCollisionEnabled(int32 Index, bool bEnabled);
	void DisableAllWeaponCollision();

public:

	static bool bDisplayDebugHitLines;

protected:

	ASoCharacterBase* SoOwner;

	// gives a hint to the target about his next action
	ESoHitReactType HitReactType = ESoHitReactType::EHR_JumpAway;

	FSoDmg ActiveDamage;

	int32 CurrentIrresistibility = 1;

	UPROPERTY(EditAnywhere)
	FSoPushDesc ActivePushDesc;

	UPROPERTY(EditAnywhere)
	bool bBlockable = true;

	UPROPERTY(EditAnywhere)
	bool bOneActorOncePerStrike = true;

	UPROPERTY()
	USkeletalMeshComponent* OwnerSkeletal;

	TArray<FName> BonesHit;

	// list of actors already hit with this strike
	// one strike hits a target only once normally
	// if it is a more complicated movement sending an anim event can reset the array during a strike
	TArray<FSoTargetHit> ActorsAlreadyHit;


	TArray<FSoWeapon> Weapons;
	TArray<FSoWeaponDynamicData> WeaponsDynamic;

	FCollisionShape WeaponShape;
};

inline void USoCombatComponent::ClearActorsAlreadyHit() { ActorsAlreadyHit.Empty(); }
inline void USoCombatComponent::SetActivePushDesc(const FSoPushDesc& PushDesc) { ActivePushDesc = PushDesc; }
inline void USoCombatComponent::SetBlockable(bool bInBlockable) { bBlockable = bInBlockable; }
