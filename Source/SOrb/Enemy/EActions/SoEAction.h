// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SoEAction.generated.h"

class ASoEnemy;
class USoEPrecondition;

/**
 *  Abstract base class for all enemy actions
 *  Basic element of the AI system:
 *  1. An action is selected based on its precondition
 *  2. The selected action does something with the owner character
 *  3. Action is done, owner selects another one
 */
UCLASS(BlueprintType, EditInlineNew, Abstract)
class SORB_API USoEAction : public UObject
{
	GENERATED_BODY()
public:

	USoEAction() {};

	/** Called each time the action is selected */
	void Start(ASoEnemy* Owner);
	/** Called before the action is interrupted unexpectedly, can be called even if bInterruptible is false (e.g. Death) */
	virtual void Interrupt(ASoEnemy* Owner);

	/**
	 * Called each frame after Start() as long as it is neither finished nor terminated
	 * @Return: whether the action is still going on (true) or finished (false)
	 */
	bool Tick(float DeltaSeconds, ASoEnemy* Owner);

	/**
	 * Evaluates the preconditons in order to generate the necessity of the action
	 * Check SoEnemy::GetFirstSatisfiedAction() and SoEnemy::ChooseActionFromList()
	 */
	virtual float Evaluate(const ASoEnemy* Owner) const;

	/** Returns the last time the action was selected in seconds */
	float GetLastUsageTime() const;

	/** whether an outside effect can stop the action from being executed (e.g. a successful melee strike) */
	virtual bool IsInterruptible() const { return bInterruptible; }

	void SetInterruptible(bool bCanBe) { bInterruptible = bCanBe; }

protected:

	/**
	 * Implementable function, called from Tick()
	 * @Return: whether the action is still going on (true) or finished (false)
	 */
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) { return false; }

	/** Called when the action is started */
	virtual void OnEnter(ASoEnemy* Owner) {};
	/** Called when the action is finished or interrupted */
	virtual void OnLeave(ASoEnemy* Owner) {};

protected:

	/** whether an outside effect can stop the action from being executed (e.g. a successful melee strike) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	bool bInterruptible = true;

	/** if true the actor's orientation is modified each frame to match the spine's direction in the given point */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	bool bForceSplineDirection = false;

	/** preconditons are used to decide how important it is for this action to be chosen (0: can't be chosen, the bigger the stronger necessity) */
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category = Params)
	TArray<USoEPrecondition*> Preconditions;

	/** if this is set to true the action is evaluated each frame and it is over if the preconditions fail */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Params)
	bool bOnlyWhileEvaluated = false;


	/** last time the action was selected in seconds */
	float LastUsageTime = -300.f;
};
