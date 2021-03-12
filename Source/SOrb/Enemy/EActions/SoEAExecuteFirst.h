// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "SoEAction.h"
#include "SoEAExecuteFirst.generated.h"

/**
 *  An array of actions, first satisfied action is executed
 *  Only the precondition of the base is checked for enter, but the action immediately finished if the others are all unsatisfied
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAExecuteFirst : public USoEAction
{
	GENERATED_BODY()
public:

	virtual void Interrupt(ASoEnemy* Owner) override;

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	TArray<USoEAction*> Actions;

	USoEAction* ActiveAction = nullptr;
};
