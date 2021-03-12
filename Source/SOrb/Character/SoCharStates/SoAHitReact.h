// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SplineLogic/SoSplinePoint.h"
#include "SoAHitReact.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoAHitReact : public USoActivity
{
	GENERATED_BODY()
public:
	USoAHitReact();

	/**
	 *  Called when HitReact is not active, may or may not switch to HitReact state!!!
	 *  bAliveIfPossible: if the character's hit point count is still greater than 0
	 *  
	 *	@return: if false we had to die
	 */ 
	bool Activate(const FSoHitReactDesc& Desc, bool bAliveIfPossible);

	virtual void Tick(float DeltaSeconds) override;

	virtual bool OnDmgTaken(const FSoDmg& Damage, const FSoHitReactDesc& HitReactDesc) override;
	virtual bool DecreaseHealth(const FSoDmg& Damage) override;

	// time to... stop copy pasting comments
	virtual bool OnPreLanded(const FHitResult& Hit) override;
	// input
	virtual void JumpPressed() override {};
	virtual void Move(float Value) override {};

	
	virtual bool BlocksTeleportRequest() const override { return true; }

	virtual bool ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const override;
	virtual void OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal) override;
	
	void SetDuration(float InDuration);

	UFUNCTION()
	void OnForceHitReactAnimTimeOver() { bForceStayInTime = false; }

protected:
	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	virtual void UpdateCamera(float DeltaSeconds) override;

	virtual bool CanBeArmedState() const;

	void SaveCamValues();

	UPROPERTY(EditAnywhere)
	float BreakIntoPiecesTime = 0.8f;

	UPROPERTY(EditAnywhere)
	float FallToDeathTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float PushedStunTime = 0.4f;

	UPROPERTY(EditAnywhere)
	float JumpAwayTime = 0.4f;

	UPROPERTY(EditAnywhere)
	float JumpAwayTimeLight = 0.1f;

	/** whether movement and face direction is the same or opposite */
	UPROPERTY(BlueprintReadOnly)
	bool bMoveToLookDir;


	bool bReturnToArmed;
	float Duration;
	float CurrentTime;

	ESoHitReactType HitReactType;

	UPROPERTY(BlueprintReadOnly)
	bool bShouldPlayAnim;

	UPROPERTY(BlueprintReadOnly)
	bool bForceStayInTime = false;

	FSoSplinePoint StoredSplineLocation;

	FRotator SavedCamRotation;
	FVector SavedCamWorldLocation;

	FTimerHandle ForceAnimInTimer;
};
inline void USoAHitReact::SetDuration(float InDuration) { Duration = InDuration; }
