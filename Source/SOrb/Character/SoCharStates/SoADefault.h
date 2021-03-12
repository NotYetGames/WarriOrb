// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoADefault.generated.h"


UCLASS()
class SORB_API USoADefault : public USoActivity
{
	GENERATED_BODY()
public:
	USoADefault();

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return true; }

	virtual bool CanJumpFromAir() const override { return true; }
	virtual void OnLanded() override;

	friend class ASoCharacter;
};
