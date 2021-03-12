// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "Items/SoItemTypes.h"
#include "SoAWeaponInArm.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoAWeaponInArm : public USoActivity
{
	GENERATED_BODY()
public:
	USoAWeaponInArm();


	virtual void Tick(float DeltaSeconds) override;


	virtual void TakeWeaponAway() override;
	virtual void ToggleWeapons() override;

	UFUNCTION(BlueprintPure, Category = "General")
	ESoWeaponType GetActiveWeaponType() const { return ActiveWeaponType; }

	virtual void OnLanded() override;


	virtual void StartFloating() override;;
	virtual void StopFloating() override;;

	UFUNCTION(BlueprintCallable)
	void RefreshWeapon();

protected:

	ESoWeaponType ActiveWeaponType;

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override;
	virtual bool CanJumpFromAir() const override { return true; }

	/** set to true if stance must be modified cause of environment */
	UPROPERTY(BlueprintReadOnly)
	bool bLowerArms = false;
};
