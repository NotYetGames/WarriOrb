// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"
#include "Enemy/SoEnemyDataTypes.h"
#include "SoEAFlyToPlayer.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAFlyToPlayer : public USoEAction
{
	GENERATED_BODY()

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float AcceptedDistance = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float MaxVelocity = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float StartVelocity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Acceleration = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float ZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float SplineOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bAway = false;

	// only works without bAway atm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bAllowZMovement = true;

	/** time before deceleration applied */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float AwayTime = 2.0f;

	// Runtime:
	FVector CurrentVelocity;
	bool bTargetReached;

	FVector AwayDirection;

	// actual z offset can't be bigger than x offset so we reach player no matter what
	float ActualZOffset;


	float RestTime;
};
