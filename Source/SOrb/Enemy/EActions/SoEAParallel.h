// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "SoEAction.h"
#include "SoEAParallel.generated.h"

/** Different predefined logic the USoEAParallel is controlled */
UENUM(BlueprintType)
enum class ESoParallelBehaviour : uint8
{
	// both action is executed at the same time
	EPB_ExecuteAtOnce				UMETA(DisplayName = "ExecuteAtOnce"),

	// first is executed normally
	// second is tried from time to time, if it does not fail first is not updated until second is running
	// does not have a not looped variant
	EPB_TrySecondPauseFirst			UMETA(DisplayName = "TrySecondPauseFirst"),

	// first is executed in a loop
	// second is tried from time to time, if it does not fail first is interrupted
	// and once second is finished the task is over
	EPB_ExecuteFirstWhileNotSecond	UMETA(DisplayName = "ExecuteFirstWhileNotSecond"),
};


/**
 *  Two actions executed parallel
 *  Only use with actions which aren't cause conflict!
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAParallel : public USoEAction
{
	GENERATED_BODY()
public:

	/** Evaluate = Super::Evaluate() * Acton0.Evaluate() * Action1.Evaluate() */
	virtual float Evaluate(const ASoEnemy* Owner) const override;

	virtual void Interrupt(ASoEnemy* Owner) override;

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	USoEAction* Action0;

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	USoEAction* Action1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	bool bEndlessLoop = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	ESoParallelBehaviour Behaviour = ESoParallelBehaviour::EPB_ExecuteAtOnce;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	float SecondTryInterval = 1.5f;

	float Counter = -1.0f;
	bool bSecondIsRunning = false;

	// used in EPB_ExecuteAtOnce mode to stop calling tick on the already finished one
	bool bFirstFinished;
	bool bSecondFinished;
};
