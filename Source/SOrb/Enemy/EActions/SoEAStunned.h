// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"
#include "Enemy/SoEnemyDataTypes.h"
#include "SoEAStunned.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAStunned : public USoEAction
{
	GENERATED_BODY()
public:

	void SetDuration(float DeltaSeconds) { Duration = DeltaSeconds; }
	void SetStatusEffect(ESoStatusEffect InStatusEffect) { StatusEffect = InStatusEffect; }

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void Interrupt(ASoEnemy* Owner) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Duration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	ESoStatusEffect StatusEffect;

	UPROPERTY()
	USoEAction* HitReact;

	float RestTime;
};
