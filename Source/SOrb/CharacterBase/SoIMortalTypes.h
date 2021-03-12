// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "SoIMortalTypes.generated.h"


class UParticleSystem;
class UParticleSystemComponent;
class USoEffectBase;
class UCurveFloat;
class AActor;
class UCurveLinearColor;


////////////////////////////////////////////////////////////////////////////////////////
/** used to request some special visual effect */
UENUM(BlueprintType)
enum class ESoStatusEffect : uint8
{
	ESE_Fire = 0				UMETA(DisplayName = "Fire"),
	ESE_Stun					UMETA(DisplayName = "Stun"),
	ESE_Slow					UMETA(DisplayName = "Slow"),
	ESE_Speed					UMETA(DisplayName = "Speed"),
	ESE_Shade					UMETA(DisplayName = "Shade"),
	ESE_LightningShade			UMETA(DisplayName = "LightningShade"),
	ESE_ReducedCooldown			UMETA(DisplayName = "ReducedCooldown"),
	ESE_Floating				UMETA(DisplayName = "Floating"),
	ESE_Bounce					UMETA(DisplayName = "Bounce"),
	ESE_Shield					UMETA(DisplayName = "Shield"),

	ESE_BootTransmutation		UMETA(DisplayName = "BootTransmutation"),

	ESE_Worthy					UMETA(DisplayName = "Worthy"),

	// weird sign above characters to generate attention
	ESE_FloatingSign			UMETA(DisplayName = "FloatingSign"),
	
	ESE_NumOf					UMETA(DisplayName = "None"),
};

////////////////////////////////////////////////////////////////////////////////////////
/** used to request visual effects in a fire and forget way */
UENUM(BlueprintType)
enum class ESoVisualEffect : uint8
{
	EVE_Heal = 0			UMETA(DisplayName = "Heal"),
	EVE_SecondChanceCast	UMETA(DisplayName = "SecondChanceCast"),

	EVE_Electrified			UMETA(DisplayName = "Electrified"),
	EVE_ElectrifiedBlue		UMETA(DisplayName = "ElectrifiedBlue"),
	EVE_ElectrifiedYellow	UMETA(DisplayName = "ElectrifiedYellow"),

	EVE_Damaged				UMETA(DisplayName = "Damaged"),
	EVE_RegenerateSpells	UMETA(DisplayName = "RegenerateSpell"),

	EVE_MAX					UMETA(DisplayName = "Invalid")
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoHitReactType : uint8
{
	EHR_Nothing = 0		UMETA(DisplayName = "Nothing"),
	EHR_JumpAway		UMETA(DisplayName = "JumpAway"),
	EHR_JumpAwayLight	UMETA(DisplayName = "JumpAwayLight"),
	EHR_Stun			UMETA(DisplayName = "Stun"),
	EHR_Pushed			UMETA(DisplayName = "Pushed"),

	EHR_FallToDeath		UMETA(DisplayName = "FallToDeath"),
	EHR_BreakIntoPieces	UMETA(DisplayName = "BreakIntoPieces"),

	EHR_MAX				UMETA(Hidden),
};

////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoPushDesc
{
	GENERATED_USTRUCT_BODY()
public:
	// the velocity the character is kicked away
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HorizontalVelocity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VerticalVelocity = 0.0f;

	/** FIXMYSELF: we don't need this here, who would want to set this up for each damage?! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StunDuration = -1.0f;
};

////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoHitReactDesc
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESoHitReactType HitReact = ESoHitReactType::EHR_Stun;

	/** Source of the damage type, or the Actor providing the respawn location via implementing  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AActor* AssociatedActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 OutDir = 0;

	/** used for push */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoPushDesc PushDesc;

	/** normally decides if hit react is triggered or not. if abs of this is greater than or equal to 10 it can't be blocked by enemies who ignore ranged attacks */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Irresistibility = 0;

	/** 
	 *  Used to decide if the hit is critical or not for NPCs as they can have Critical vulnerable parts
	 *  Critical hit on npc uses a different set of res values
	 *  A damage is critical on the player (ignores res) if the array contains the "Critical" name
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<FName> AssociatedBones;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoRootMotionDesc
{
	GENERATED_USTRUCT_BODY()

public:

	// defines if the given components of the root motion are ignored or not
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHorizontalRootMotionEnabled = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bVerticalRootMotionEnabled = false;

	// applied to the extracted values
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Multiplier = 1.0f;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UENUM(BlueprintType)
// enum class ESoDamageType : uint8
// {
// 	// physical
// 	EDT_Crush = 0			UMETA(DisplayName = "Crush"),
// 	EDT_Slash				UMETA(DisplayName = "Slash"),
// 	// magical
// 	EDT_WarmMagic			UMETA(DisplayName = "WarmMagic"),
// 	EDT_ColdMagic			UMETA(DisplayName = "ColdMagic"),
//
// 	EDT_Overlap				UMETA(DisplayName = "Overlap"),
//
// 	EDT_MAX					UMETA(Hidden),
// };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Don't ever rename this to ESoDamageType as it used t be that way
UENUM(BlueprintType)
enum class ESoDmgType : uint8
{
	None = 0				UMETA(DisplayName = "None"),
	
	Physical 				UMETA(DisplayName = "Physical"),
	Magic					UMETA(DisplayName = "Magical"),
	Mixed					UMETA(DisplayName = "Mixed"),

	Max						UMETA(Hidden),
};

////////////////////////////////////////////////////////////////////////////////////////
// New damage type
USTRUCT(BlueprintType, Blueprintable)
struct FSoDmg
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Physical = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Magical = 0.0f;

public:

	FSoDmg operator*(const FSoDmg& Other) const;
	FSoDmg operator*(float Multiplier) const;
	void operator*=(const FSoDmg& Other);
	void operator*=(float Multiplier);
	FSoDmg operator+(const FSoDmg& Other) const;
	void operator+=(const FSoDmg& Other);
	float operator[](int32 Index) const;

	FSoDmg OneMinus() const;
	float Sum() const;
	ESoDmgType GetType() const;
	bool HasPhysical() const;
	bool HasMagical() const;
	void SetToZero();
};

////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoMeleeHitParam
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoDmg Dmg;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TSubclassOf<USoEffectBase>> Effects;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoHitReactDesc HitReactDesc;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FHitResult Hit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bBlockable = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCritical = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UParticleSystem* ImpactFX = nullptr;
};


/**
 *   Helper struct used to animate material properties 
 */
USTRUCT(BlueprintType, Blueprintable)
struct FSoMaterialAnimationEntry
{
	GENERATED_USTRUCT_BODY()
public:

	/** Animation Curve, evalueated between 0 and 1 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UCurveFloat* ScalarCurve = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UCurveLinearColor* ColorCurve = nullptr;

	/** length of material animation */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Duration = 0.f;

	/** name of material we have to animate */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName MaterialParameterName;

	// all material is modified if the set is empty, only selected indices otherwise
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSet<int32> MaterialIndexList;

	UPROPERTY(BlueprintReadWrite)
	float Counter = 1000.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bPlayForward = true;

	UPROPERTY(BlueprintReadWrite)
	float PlaySpeed = 1.0f;

	UPROPERTY(BlueprintReadWrite)
	TArray<UParticleSystemComponent*> TargetOverridePS;
};
