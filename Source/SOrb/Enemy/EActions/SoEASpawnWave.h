// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"
#include "SoEASpawnWave.generated.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEASpawnWave : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index = 0;
};
