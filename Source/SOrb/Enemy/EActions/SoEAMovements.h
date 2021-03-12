// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "SoEAction.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoEAMovements.generated.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Abstract base class for spline based movement
 */
UCLASS(BlueprintType, EditInlineNew, Abstract)
class SORB_API USoEAMoveTo : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	/** The distance the owner has to be from the target, once it is reached the action is finished */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float AcceptedDistance = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bGiveUpIfNoProgress = false;

	float DistanceLastFrame;
	float TimeSinceProgress;

	FSoSplinePoint TargetPoint;

	FVector OwnerLastFramePos;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAMoveToMarker : public USoEAMoveTo
{
	GENERATED_BODY()

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	int32 Index;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAMoveToPlayer : public USoEAMoveTo
{
	GENERATED_BODY()

protected:

	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bOnlyBetweenMarkers = false;

	/** Only works for same spline */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bClampBetweenMarkers = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bCheckResultOnInvalidMarkers = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	int32 MarkerIndex0 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	int32 MarkerIndex1 = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float SplineOffset = 0.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAMoveToPlayerForDuration : public USoEAMoveToPlayer
{
	GENERATED_BODY()

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Duration = 2.0f;

	float RestTime = 0.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAMoveAwayFromPlayer : public USoEAction
{
	GENERATED_BODY()

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float AcceptedDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float MaxMoveTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bFacePlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bBetweenMarkers = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float MarkerDistanceThreshold = 400;

	float Timer;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAMoveAwayFromClosestAlly : public USoEAction
{
	GENERATED_BODY()

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float AcceptedDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float MaxMoveTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bIgnoreFloatings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	bool bOnlySameClass = true;

	float Timer;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEATurnToPlayer : public USoEAction
{
	GENERATED_BODY()
protected:
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEATurn : public USoEAction
{
	GENERATED_BODY()
protected:
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAMoveTowardsDirection : public USoEAction
{
	GENERATED_BODY()

protected:

	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnEnter(ASoEnemy* Owner) override;


protected:

	UPROPERTY()
	int32 DirectionMultiplier = 1.0f;

	UPROPERTY()
	float Duration = 4.0f;

	float Counter = 0.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAJumpTo : public USoEAMoveTo
{
	GENERATED_BODY()

protected:
	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;

	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

	virtual void CalculateTarget(ASoEnemy* Owner);

	UPROPERTY(EditAnywhere)
	float JumpZ = 800;

	UPROPERTY(EditAnywhere)
	float DistanceOffset = 0.0f;

	UPROPERTY(EditAnywhere)
	bool bClampToMarkers = false;

	float CachedWalkSpeed;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAJumpToPlayer : public USoEAJumpTo
{
	GENERATED_BODY()

protected:

	virtual void CalculateTarget(ASoEnemy* Owner) override;
};
