// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CharacterBase/SoIMortalTypes.h"
#include "Projectiles/SoProjectileTypes.h"

#include "SoEnemyDataTypes.generated.h"

class ASoProjectile;
class USoEAction;

/** Basic activity of an AI character */
UENUM(BlueprintType)
enum class ESoEnemyActivity : uint8
{
	// idle/walking/doing basic stuff/thinking about life
	EEA_Default		UMETA(DisplayName = "Default"),
	// melee attack
	EEA_Strike		UMETA(DisplayName = "Strike"),
	// range attack
	EEA_RangeAttack	UMETA(DisplayName = "RangeAttack"),
	// taking damage, shouting "autch"
	EEA_HitReact	UMETA(DisplayName = "HitReact"),
	// blocking enemy attack
	EEA_Block		UMETA(DisplayName = "Block"),
	// being blocked by enemy
	EEA_Blocked		UMETA(DisplayName = "Blocked"),
	// guess what
	EEA_Dead		UMETA(DisplayName = "Dead"),

	EEA_Spawn		UMETA(DisplayName = "Disabled")
};


/**
 * struct used to enable root motion for the given time interval, used by strike
 * if the "To" time is greater than the strike length it is not turned off
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoRootMotionEntry
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float From = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float To = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoRootMotionDesc RootMotion;
};


/**
*  struct used to enable weapon collision for the given time interval, used by strike
*  if the "To" time is greater than the strike length it is not turned off
*/
USTRUCT(BlueprintType, Blueprintable)
struct FSoWeaponCollisionEntry
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float From = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float To = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Slot = 0;
};


/**
 * struct used to enable controlled movement for the given time interval
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoControlledMotionEntry
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float From = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float To = 1.5f;

	/** it can only be positive right now TODO: handle negative value if necessary */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bStopIfStuck = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHorizontal = true;
};


USTRUCT(BlueprintType, Blueprintable)
struct FSoStrikePhase
{
	GENERATED_USTRUCT_BODY()
public:
	/**
	 *  length of the strike phase in seconds
	 *  non-looped animation is played with a speed so that the length matches this
	 */
	UPROPERTY(VisibleAnywhere)
	float Duration = 0.f;

	/** Name of the animation to play during this strike phase, the enemy has to have a matching entry in ASoEnemy::StrikeMap */
	UPROPERTY(VisibleAnywhere)
	FName Animation;

	/** weather the animation is looped or not (not the phase). Looped phase takes Duration seconds, but the animation isn't aligned to that */
	UPROPERTY(VisibleAnywhere)
	bool bLooped = false;

	/**
	 *  only used if >0 and bLooped is false. Use this to have a shorter/longer animation than the actual phase duration
	 *  if it is shorter, the last frame of the animation will be displayed. if it is longer the end is skipped
	 */
	UPROPERTY(VisibleAnywhere)
	float AnimationDuration = -1.0f;


	UPROPERTY(VisibleAnywhere)
	TArray<FSoRootMotionEntry> RootMotion;

	UPROPERTY(VisibleAnywhere)
	TArray<FSoControlledMotionEntry> ControlledMotion;

	UPROPERTY(VisibleAnywhere)
	TArray<FSoWeaponCollisionEntry> WeaponCollision;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	FSoDmg Damage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoPushDesc PushDesc;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bBlockable = false;

public:
	static FSoStrikePhase Invalid;
};


/**
 *  Everything the AI needs to perform a strike
 *  Strikes are defined in "SO\AIConfig\ActionLists"
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoStrike
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(VisibleAnywhere)
	TArray<FSoStrikePhase> Phases;

	UPROPERTY(VisibleAnywhere)
	bool bLookAtPlayer;
};


/**
 *  WIP
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoERangeAttackProfile
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseThisInsteadOfComponentDefaults = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<ASoProjectile> ProjectileClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseProjectileInitDataOverride = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoProjectileInitData InitDataOverride;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPreferLargeArc = false;

	// Dynamic Data, initialized runtime:

	UPROPERTY(BlueprintReadWrite)
	FVector Location;

	UPROPERTY(BlueprintReadWrite)
	FRotator Orientation;
};


/**
 *  Struct is required because a container has to have a wrapper if it wants to be in another container
 *  tbh I really hate copy-paste comments
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoEStrikes
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TMap<FName, FSoStrike> StrikeMap;
};


/**
 *  Struct is required because a container has to have a wrapper if it wants to be in another container
 *  tbh I have no idea if it wants to be there or not, and I don't even care
 *  I want him to be there, that was the point : /
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoEActions
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Instanced)
	TArray<USoEAction*> Array;
};


/**
 * Named action lists to provide combinations more easily
 * E.g. switch to "EAL_Idle" state is possible as an action
 * -> can work with any idle state, action list independent
 * check the usage of ESoEnemy::NamedActionLists
 */
UENUM(BlueprintType)
enum class ESoActionList : uint8
{
	// slots to select from config
	EAL_Idle		UMETA(DisplayName = "Idle"),
	EAL_Melee		UMETA(DisplayName = "Melee"),
	EAL_Ranged		UMETA(DisplayName = "Ranged"),

	// slots used in code directly (with GetFirstSatisfiedAction())
	EAL_Block		UMETA(DisplayName = "Block"),
	EAL_HitReact	UMETA(DisplayName = "HitReact"),
	EAL_Blocked		UMETA(DisplayName = "Blocked"),

	EAL_Max			UMETA(DisplayName = "Max"),
};


/**
 *  Used e.g. to get the name of the last strike/block/hitreact
 */
UENUM(BlueprintType)
enum class ESoEnemyIDType : uint8
{
	EEIT_Strike		UMETA(DisplayName = "Strike"),
	EEIT_Block		UMETA(DisplayName = "Block"),
	EEIT_HitReact	UMETA(DisplayName = "HitReact")
};


UENUM(BlueprintType)
enum class ESoRangeAttackAnimType : uint8
{
	ERAT_Single					UMETA(DisplayName = "Single"),
	ERAT_SingleLooped			UMETA(DisplayName = "SingleLooped"),
	ERAT_SingleTimeScaled		UMETA(DisplayName = "SingleTimeScaled"),

	// duration of both the anim and the range attack is determined by the anim length
	ERAT_SingleAnimBasedTiming	UMETA(DisplayName = "SingleAnimBasedTiming"),

	ERAT_PrepareAndFinish		UMETA(DisplayName = "PrepareAndFinish"),
	ERAT_PrepareAndLoop			UMETA(DisplayName = "PrepareAndLoop"),

	ERAT_NoAnim					UMETA(DisplayName = "NoAnim"),
};


/**
 *  Named values for script support
 */
UENUM(BlueprintType)
enum class ESoEnemyFloat : uint8
{
	// read only outside of SoEnemy
	EEF_TimeSinceLastBlock			UMETA(DisplayName = "TimeSinceLastBlock"),
	EEF_TimeSinceLastHitReact		UMETA(DisplayName = "TimeSinceLastHitReact"),
	EEF_TimeSinceLastMonologueEnd	UMETA(DisplayName = "TimeSinceLastMonolougeEnd"),

	// can be set directly
	EEF_BlockChance					UMETA(DisplayName = "BlockChance"),
	EEF_BlockChanceDelta			UMETA(DisplayName = "BlockChanceDelta"),
};


UENUM(BlueprintType)
enum class ESoAnimationType : uint8
{
	// idle/walk/fall cycle
	EAT_Default							UMETA(DisplayName = "Default"),

	EAT_InterruptibleSingle				UMETA(DisplayName = "InterruptibleSingle"),
	EAT_UninterruptibleSingle			UMETA(DisplayName = "UninterruptibleSingle"),

	EAT_InterruptibleWithPreparation	UMETA(DisplayName = "InterruptibleWithPreparation"),

	EAT_Spawn							UMETA(DisplayName = "Spawn")
};


USTRUCT(BlueprintType, Blueprintable)
struct FSoPingPongAnimName
{
	GENERATED_USTRUCT_BODY()
public:

	void SetAnimation(FName AnimName, float Duration, bool bLoop = false)
	{
		bPing = !bPing;

		if (bPing)
		{
			PingAnimName = AnimName;
			PingDuration = Duration;
			bPingLoop = bLoop;
		}
		else
		{
			PongAnimName = AnimName;
			PongDuration = Duration;
			bPongLoop = bLoop;
		}
	}

	void InitAsInterruptibleWithPreparation(FName PreperAnimName, FName MainAnimName, bool bMainLooped, float MainPlayRate)
	{
		bPing = true;
		PingAnimName = PreperAnimName;
		PongAnimName = MainAnimName;
		bPingLoop = false;
		bPongLoop = bMainLooped;
		PingDuration = 1.0f;
		PongDuration = MainPlayRate;
	}

	FORCEINLINE FName GetName(bool bInPing) const { return bInPing ? PingAnimName : PongAnimName; }
	FORCEINLINE bool IsLooped(bool bInPing) const { return bInPing ? bPingLoop : bPongLoop; }
	FORCEINLINE float GetDuration(bool bInPing) const { return bInPing ? PingDuration : PongDuration; }

protected:
	UPROPERTY(BlueprintReadOnly)
	bool bPing = false;

	UPROPERTY(BlueprintReadOnly)
	FName PingAnimName;

	UPROPERTY(BlueprintReadOnly)
	FName PongAnimName;

	UPROPERTY(BlueprintReadOnly)
	float PingDuration = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float PongDuration = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bPingLoop = false;

	UPROPERTY(BlueprintReadOnly)
	bool bPongLoop = false;
};


/**
 *  Information for animation state machines
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoEnemyAnimData
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(BlueprintReadOnly)
	ESoAnimationType Type = ESoAnimationType::EAT_Default;

	UPROPERTY(BlueprintReadOnly)
	FSoPingPongAnimName UninterruptibleSingle;

	UPROPERTY(BlueprintReadOnly)
	FSoPingPongAnimName InterruptibleSingle;

	UPROPERTY(BlueprintReadOnly)
	FSoPingPongAnimName InterruptibleWithPreparation;

	UPROPERTY(BlueprintReadOnly)
	FName SpawnAnim;
};
