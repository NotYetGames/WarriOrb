// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"

#include "SplineLogic/SoSplinePoint.h"

#include "SoEAMoveToPosition.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAMoveToPosition : public USoEAction
{
	GENERATED_BODY()

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bToPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float ZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float SplineOffset = 0.0f;

	// only works on a single spline!!!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bAlongSpline = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bAlongZ = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bClamBetweenMarkers = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bSmoothStep = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	int32 MarkerIndex0 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	int32 MarkerIndex1 = 1;



	float Counter;


	FSoSplinePoint InitialSplineLocation;
	float InitialZ;

	float TargetZ;
	float TargetX;
};
