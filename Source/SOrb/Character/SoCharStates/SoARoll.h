// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoARoll.generated.h"


UCLASS()
class SORB_API USoARoll : public USoActivity
{
	GENERATED_BODY()
public:

	USoARoll();

	virtual void Tick(float DeltaSeconds) override;

	// how about no?!
	virtual void IncreaseCollisionSize() override {};

	// don't you say
	virtual void RollPressed() override {};

	// yep!

	virtual void JumpPressed() override;

	virtual bool ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const override;
	virtual void OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal) override;
	
	virtual void OnPushed(const FVector& DeltaMovement, float DeltaSeconds, bool bStuck, AActor* RematerializeLocation, int32 DamageAmountIfStuck, bool bForceDamage) override;

	virtual bool ShouldDecreaseCollision() const override { return true; }
	
protected:

	// check cpp for comment
	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;
	virtual void OnPostExit(USoActivity* NewActivity) override;
	// if user input can force the state to change
	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override;
	
	virtual void OnJumped() override;

	void ClearMaxSpeedBoost();

	UPROPERTY(BlueprintReadOnly)
	bool bInverseRollAnim = false;

	float LastWallJumpTime = -1.0f;
	float MaxVelocityBoostCounter = -1.0f;
};
