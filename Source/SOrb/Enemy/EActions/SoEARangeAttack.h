// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"
#include "Engine/EngineTypes.h"

#include "Enemy/SoEnemyDataTypes.h"
#include "SoEARangeAttack.generated.h"


USTRUCT(BlueprintType, Blueprintable)
struct FSoRangeAttackSpawn
{
	GENERATED_USTRUCT_BODY()
public:

	/** time of spawn (after range attack start) */
	UPROPERTY(EditAnywhere)
	float Time = 0.0f;

	/** index of the projectile profile */
	UPROPERTY(EditAnywhere)
	int32 Index = 0;

	/** subtype, can be used in BP */
	UPROPERTY(EditAnywhere)
	int32 Variant = 0;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoAnimationOverride
{
	GENERATED_USTRUCT_BODY()
public:

	/** time of spawn (after range attack start) */
	UPROPERTY(EditAnywhere)
	float Time;

	UPROPERTY(EditAnywhere)
	FName AnimationName;

	UPROPERTY(EditAnywhere)
	float Duration;

	UPROPERTY(EditAnywhere)
	bool bLoop = false;

	UPROPERTY(EditAnywhere)
	bool bInterruptible = false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEARangeAttack : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float Duration;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bDoneIfProjectilesDied = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bForceDoneOnDurationOver = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	ESoRangeAttackAnimType AnimationType;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	TArray<FSoRangeAttackSpawn> AttackList;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	TArray<FSoAnimationOverride> AnimationOverrideList;

	float Counter;
};
