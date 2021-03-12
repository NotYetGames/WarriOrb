// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"
#include "Enemy/SoEnemyDataTypes.h"
#include "SoEAFlyHome.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAFlyHome : public USoEAction
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
	bool bOnlyAlongZ = false;

	// Runtime:
	FVector CurrentVelocity;
	bool bTargetReached;
};
