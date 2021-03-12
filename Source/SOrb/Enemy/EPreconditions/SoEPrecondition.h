// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Enemy/SoEnemyDataTypes.h"
#include "SoEPrecondition.generated.h"

class USoEAction;
class ASoEnemy;

/**
 *
 */
UCLASS(BlueprintType, EditInlineNew, Abstract)
class SORB_API USoEPrecondition : public UObject
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const;
protected:
	// the value this precondition returns if it is satisfied
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1000000"), Category = Params)
	float TrueValue = 1.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPSplineDistanceFromPlayer : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MinDistance;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxDistance;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bCheckAgainstAlert = false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPSplineDistanceFromClosestAlly : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MinDistance;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxDistance;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bIgnoreFloatings = false;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bOnlySameClass = true;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPZDistanceFromPlayer : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:
	// compared to OwnerZ - PlayerZ, sign matters
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MinDistance;
	// compared to OwnerZ - PlayerZ, sign matters
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxDistance;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse = false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPDistanceFromPlayer : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MinDistance;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxDistance;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPZDistanceFromStart : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:
	// compared to OwnerZ - StartOwnerZ, sign matters
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MinDistance;
	// compared to OwnerZ - StartOwnerZ, sign matters
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxDistance;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPNotFacingPlayer : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse;
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPPlayerNotInDialogue : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPWasNotUsedRecently : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:
	// Delta < SoftTimeDelta ? 0.0f : >= SoftTimeValue
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float SoftTimeDelta;
	// Delta >= HardTimeDelta ? HardTimeValue : <HardTimeValue
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float HardTimeDelta;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float SoftTimeValue;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float HardTimeValue;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPIsStandingOnGround : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew, Abstract)
class SORB_API USoEPRandom : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const;
protected:
	// value between 0.0f and 1.0f, 0.0f means never, 1.0f means always
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = "0", ClampMax = "1"), Category = Params)
	float Percent = 1.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPBlockedRecently : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	// delta time which is the upper bound of the "recently" consideration
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxTimeDelta;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPLastID : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	// the value we will ask from the player
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	ESoEnemyIDType IDType;

	// player's value is compared to this
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;

	// bInverse -> !=
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPRangeAttackTestHit : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index = 0;

	// amount of raycasts for the hit test
	// with gravity scale 0 and straight spline it could be 1
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Precision = 3;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPFloatValueIsGreater : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	ESoEnemyFloat Value;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float CompareTo;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPFloatValueIsSmaller : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	ESoEnemyFloat Value;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float CompareTo;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPIsPlayerAlive : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bShouldBeAlive = true;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPBlueprintCondition : public USoEPrecondition
{
	GENERATED_BODY()

public:

	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPIsPlayerSurrounded : public USoEPrecondition
{
	GENERATED_BODY()

public:

	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPEnemyDistanceFromPlayer : public USoEPrecondition
{
	GENERATED_BODY()

public:

	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 MinValue = -1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 MaxValue = -1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bIgnoreFloatings = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bOnlySameClass = false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPAorB : public USoEPrecondition
{
	GENERATED_BODY()

public:

	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	USoEPrecondition* A;

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	USoEPrecondition* B;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPCheckDynamicName : public USoEPrecondition
{
	GENERATED_BODY()

public:

	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName ExpectedValue;
};
