// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CharacterBase/SoIMortalTypes.h"

#include "SoProjectileTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ESoBehaviourOnTargetHit : uint8
{
	EBOTH_UseDefault			UMETA(DisplayName = "UseDefault"),
	EBOTH_KillProjectile		UMETA(DisplayName = "KillProjectile"),
	EBOTH_Ignore				UMETA(DisplayName = "Ignore")
};


/** init data from spawner */
USTRUCT(BlueprintType, Blueprintable)
struct FSoProjectileInitData
{
	GENERATED_USTRUCT_BODY()
public:

	/**
	 * Starting velocity, usage is a bit meh/overcomplicated
	 * if Spline based:
	 *			if bShouldVelocityInfluenceDirection is false:
	 *					- X along spline, Y is vertical
	 *			else:
	 *					- X along forward vector (can contain Z as well), Y adds to that along (0,0,1) only
	 * else:
	 *			if bShouldVelocityInfluenceDirection is false:
	 *				- X is along forward vector, Y is ignored
	 *			else:
	 *				- X and Y and ForwardVector somehow determines it, the way it used to be with bullets before projectiles
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D Velocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSetVelocityAsMaxSpeed = false;

	/** old bullet like thing, can be still useful in some case */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShouldVelocityInfluenceDirection = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float GravityScale = 0.0f;

	/* must be >0! */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.1"))
	float MaxLifeTime = 20.0f;

	/* used if > 0 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 HitCountBeforeDestroyed = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SpawnerProtectionTime = -2.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESoBehaviourOnTargetHit TargetHitBehaviourOverride = ESoBehaviourOnTargetHit::EBOTH_UseDefault;
};


/** runtime data from spawner */
USTRUCT(BlueprintType, Blueprintable)
struct FSoProjectileRuntimeData
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoDmg Damage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESoHitReactType HitReactType = ESoHitReactType::EHR_Stun;

	/** the one used for hitreact in the case of break into peaces (normally the bullet is used) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AActor* AssociatedActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Irresistibility = 0;
};
