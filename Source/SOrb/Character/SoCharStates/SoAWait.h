// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"

#include "SoAWait.generated.h"


UCLASS()
class SORB_API USoAWait : public USoActivity
{
	GENERATED_BODY()

public:

	USoAWait();

	virtual void Tick(float DeltaSeconds) override;

	virtual void Move(float Value) override {};
	virtual void StrikePressed() override {};
	virtual void RightBtnPressed() override {};
	virtual void JumpPressed() override {};
	virtual void RollPressed() override {};
	virtual void ToggleWeapons() override {};
	virtual void ToggleItems() override {};
	virtual void UseItemFromSlot0() override {};

	virtual void InteractKeyPressed(bool bPrimary) override {};

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return false; }

	UFUNCTION(BlueprintCallable, Category = Wait)
	void Enter(float Duration, bool bInArmed = false);

protected:

	UPROPERTY(BlueprintReadWrite)
	float Counter = -1.0f;
};
