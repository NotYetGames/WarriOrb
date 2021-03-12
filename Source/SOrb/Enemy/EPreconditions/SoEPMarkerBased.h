// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEPrecondition.h"
#include "SoEPMarkerBased.generated.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPSplineDistanceFromMarker : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index = 0;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MinDistance = 0.0f;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxDistance = BIG_NUMBER;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bShouldBeIn = true;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPNotBetweenMarkers : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index0 = 0;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index1 = 1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bFailsOnInvalidMarker = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bFailsIfNotBetween = false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPCharacterBetweenMarkers : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;
protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index0 = 0;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index1 = 1;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bResultOnInvalidMarkers = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bInverse = false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEPCharacterSplineDistancefromMarker : public USoEPrecondition
{
	GENERATED_BODY()
public:
	virtual float Evaluate(const ASoEnemy* OwnerCharacter, const USoEAction* OwnerAction) const override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index = 0;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MinDistance = 0.0f;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float MaxDistance = BIG_NUMBER;
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bShouldBeIn = true;
};
