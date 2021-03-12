// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoALillian.generated.h"


UCLASS()
class SORB_API USoALillian : public USoActivity
{
	GENERATED_BODY()
public:
	USoALillian();

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	virtual void OnInteract(AActor* Interactable) override;

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return DesiredState == EActivity::EA_InUI ||
																									  DesiredState == EActivity::EA_Teleport; }
	virtual bool CanOpenCharacterPanels() const { return false; }
	virtual bool CanJumpFromAir() const override { return false; }


	virtual void UseItemFromSlot0() override {};
	virtual bool DecreaseHealth(const FSoDmg& Damage) override { return true; }
	virtual bool OnDmgTaken(const FSoDmg& Damage, const FSoHitReactDesc& HitReactDesc) override { return true; }
	virtual void OnDeath() override {}
	
	// input
	virtual void JumpPressed() override {};
	virtual void ToggleWeapons() override {};

	virtual void HandleCollision() override { IncreaseCollisionSize(); };
	virtual void OnBaseChanged(AActor* ActualMovementBase) override {};


	friend class ASoCharacter;
};
