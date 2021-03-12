// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once


#include "SoActivity.h"
#include "SoAAiming.generated.h"


UCLASS()
class SORB_API USoAAiming : public USoActivity
{

	GENERATED_BODY()

public:

	USoAAiming();

	virtual void Tick(float DeltaSeconds) override;

	virtual void UseItemFromSlot0() override;

	virtual void TakeWeaponAway() override;
	virtual void RightBtnPressed() override;
	virtual void StrikePressed() override;

	virtual void ToggleWeapons() override {};

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;
	
	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return true; }

};

