// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoADead.generated.h"

UCLASS()
class SORB_API USoADead : public USoActivity
{

	GENERATED_BODY()

public:

	USoADead();

	virtual void Tick(float DeltaSeconds) override;

	void SetProperties(bool bWithSK) { bWithSoulkeeper = bWithSK; }

	virtual void UseItemFromSlot0() override {};
	virtual void InteractKeyPressed(bool bPrimary) override;

	virtual bool DecreaseHealth(const FSoDmg& Damage) override { return false; }
	virtual bool OnDmgTaken(const FSoDmg& Damage, const FSoHitReactDesc& HitReactDesc) override { return false; }

	virtual void OnDeath() override {}

	// input
	virtual void JumpPressed() override {};
	virtual void Move(float Value) override {};
	virtual void ToggleWeapons() override {};

	virtual void HandleCollision() override {};
	virtual void OnBaseChanged(AActor* ActualMovementBase) override {};

	void OverrideCameraData(const FRotator& Rotation, const FVector& Location);

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return DesiredState == EActivity::EA_Teleport; }

	virtual void UpdateCamera(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = ">Time")
	float GetInputBanTime() const;

protected:
	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;


	virtual bool ShouldUpdateMeshFade() const override { return false; }

protected:

	FRotator SavedCamRotation;
	FVector SavedCamWorldLocation;

	bool bWithSoulkeeper = false;

	bool bBanTimeOver = false;

	UPROPERTY(EditAnywhere, Category = ">Time")
	float InputBanTime = 0.2f;

	UPROPERTY(BlueprintReadOnly)
	float Counter = 0.0f;

	bool bRevived = false;

	bool bTryToRespawn = false;
	bool bWouldLeaveSpline = false;
};
