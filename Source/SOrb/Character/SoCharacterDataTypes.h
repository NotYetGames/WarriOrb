// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CharacterBase/SoIMortalTypes.h"
#include "SoCharacterDataTypes.generated.h"

class UAnimSequenceBase;
class USoWeaponTemplate;
class AStaticMeshActor;
class ADestructibleActor;
class USoEffectBase;
class UParticleSystem;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// struct used to store data for fading in/out objects between the player and camera
// the fade system is based on trigger volumes they register/unregister mesh components on BeginOverlap/EndOverlap
USTRUCT(BlueprintType)
struct FSoFadeEntry
{
	GENERATED_USTRUCT_BODY()
public:
	// mesh to fade out
	UPROPERTY()
	AStaticMeshActor* Mesh = nullptr;

	// > 0 -> fade out, == 0 -> fade in, == 0 -> WTH???
	int32 InstanceCount;

	// 0: invisible, 1: visible
	float FadeValue;

	// interpolation speed
	float FadeSpeed;

	bool bCanHideIfOutFaded;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoWeaponTrailFade
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	AStaticMeshActor* Mesh = nullptr;

	UPROPERTY()
	float Counter;

	UPROPERTY()
	float MaxValue;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoFadeAndKillDestructible
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	ADestructibleActor* Destructible = nullptr;

	float FadeCounter;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoCooldownCounter
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	const UObject* Object = nullptr;

	UPROPERTY(BlueprintReadOnly)
	float Counter;

	UPROPERTY(BlueprintReadOnly)
	bool bAllowInAir;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType)
struct FSoWeaponBoost
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoEffectBase* OwnerEffect = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSoDmg BonusDamage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USoWeaponTemplate* WeaponTemplate = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UParticleSystem* ParticleSystem = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UParticleSystem* ImpactFX = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor TrailColor = FLinearColor(4.8f, 5.0f, 1.2f, 1.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor TrailAnimLight = FLinearColor(100.0f, 7.8f, 0.0f, 1.0f);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USTRUCT(BlueprintType, Blueprintable)
struct FSoAnimationSet
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* Idle = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* IdleLeft = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* Run = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* RunBack = nullptr;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* Jump = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* JumpLeft = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* Float = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* FloatLeft = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* Fall = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* FallLeft = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* Land = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Jump)
	UAnimSequenceBase* LandLeft = nullptr;
};
