// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoActivityTypes.generated.h"

class UAnimSequenceBase;

/** Basic activity of the character */
// should be ESoActivity but... whatever
UENUM(BlueprintType)
enum class EActivity : uint8
{
	// standing or walking on ground
	EA_Default			UMETA(DisplayName = "Default"),
	// rolling - can be done both in air and in ground
	EA_Rolling			UMETA(DisplayName = "Rolling"),
	// swinging on a flagpole or on something like that - hope we will be able to implement this
	EA_Swinging 		UMETA(DisplayName = "Swinging"),
	// automatic movement on slippery surfaces
	EA_Sliding 			UMETA(DisplayName = "Sliding"),
	// slow fall / fly in currents with umbrella
	EA_SkyDiving		UMETA(DisplayName = "SkyDiving"),

	// melee weapon
	EA_WeaponInArm		UMETA(DisplayName = "WeaponInArm"),
	EA_Striking			UMETA(DisplayName = "Striking"),

	// crossbow
	EA_Aiming			UMETA(DisplayName = "Aiming"),
	// shield
	EA_ShieldMode		UMETA(DisplayName = "ShieldMode"),

	// I had another potion, I am sure I had one... it must be here somewhere...
	EA_InUI				UMETA(DisplayName = "InUI"),

	/** let's push this rock */
	EA_InteractWithEnvironment	UMETA(DisplayName = "InteractWithEnvironment"),
	EA_WaitForActivitySwitch	UMETA(DisplayName = "WaitForActivitySwitch"),
	EA_Wait						UMETA(DisplayName = "Wait"),

	// pull the switch!
	EA_LeverPush		UMETA(DisplayName = "LeverPush"),
	// magic boxes
	EA_PickUpCarryable	UMETA(DisplayName = "PickUpCarryable"),
	EA_DropCarryable	UMETA(DisplayName = "DropCarryable"),
	EA_Carrying			UMETA(DisplayName = "Carrying"),

	// item usage
	EA_ItemThrow		UMETA(DisplayName = "Throw"),
	EA_ItemUsage		UMETA(DisplayName = "ItemUsage"),

	// autch
	EA_HitReact			UMETA(DisplayName = "HitReacting"),
	// autch... puff
	EA_Flattened		UMETA(DisplayName = "Flattened"),
	// character is too busy being dead -> can't do anything
	EA_FallToDeath		UMETA(DisplayName = "FallToDeath"),
	EA_Dead				UMETA(DisplayName = "Dead"),

	EA_Teleport			UMETA(DisplayName = "Teleport"),

	EA_Lillian			UMETA(DisplayName = "Lillian"),

	EA_CameraEdit		UMETA(Hidden),
	EA_SkyEdit			UMETA(Hidden),
	EA_CharShadowEdit	UMETA(Hidden),
	EA_Max				UMETA(Hidden)
};

/** anim notifications */
// this should be ESoAnimEvents :( then I would ruin all my anim notifications :/
UENUM(BlueprintType)
enum class EAnimEvents : uint8
{
	// standing or walking on ground
	EAE_OnCarryStuffPicked						UMETA(DisplayName = "OnCarryStuffPicked"),
	EAE_OnCarriedStuffReleased					UMETA(DisplayName = "OnCarriedStuffReleased"),
	EAE_OnReleaseCarriedStuffInterrupted		UMETA(DisplayName = "OnReleaseCarriedStuffInterrupted"),

	EAE_AnimFinished							UMETA(DisplayName = "AnimFinished"),
	EAE_AnimStarted								UMETA(DisplayName = "AnimStarted"),
	EAE_ClearRootMotion							UMETA(DisplayName = "ClearRootMotion"),
	EAE_OnItemLaunched							UMETA(DisplayName = "ItemLaunched"),
	EAE_ShowItemMesh							UMETA(DisplayName = "ShowItemMesh")
};


// Lever: value goes from 0.0f until 1.0f
// starting value and speed are defined by lever
USTRUCT(BlueprintType)
struct FSoLeverData
{
	GENERATED_USTRUCT_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActualValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PushDirection;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequenceBase* Animation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ScaledAnimDuration;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bForcePull;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WaitAfterLeverPull;
};
