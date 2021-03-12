// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "SoEAction.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoEATeleportToMarker.generated.h"


UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEATeleportToMarker : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	/** The distance the owner has to be from the target, once it is reached the action is finished */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	int32 Index = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bPutToGround = true;

	/** sum duration, includes wait time before and after teleport */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Duration = 1.0f;

	/** delay from action start before the teleport is performed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float TeleportDelay = 0.5f;

	float LastFrameTime = 0.0f;
};
