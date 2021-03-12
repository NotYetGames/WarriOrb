// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "SoEAction.h"
#include "SoEAActionComposition.generated.h"

/**
 *  An action containing and controlling other actions
 *  It has one primary action, and optional secondary ones
 *  action start is based on the root action's precondition, the primary action's preconditions are ignored
 *  one or none of the secondary ones are played after the primary action depending on their preconditions
 *  action is over after the secondary action is over, or if none of the secondary can be selected
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAActionChainElement : public USoEAction
{
	GENERATED_BODY()
public:

	virtual float Evaluate(const ASoEnemy* Owner) const override;

	virtual void Interrupt(ASoEnemy* Owner) override;

	virtual bool IsInterruptible() const override;

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	USoEAction* PrimaryAction;

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	TArray<USoEAction*> SecondActions;

	/** whether to call GetFirstSatisfiedAction() or ChooseActionFromList() to select the secondary action */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	bool bPickFirstSecond = true;

	bool bPrimaryPhase = true;
	USoEAction* ActiveSecondaryAction = nullptr;
};

/**
 *  A sequence of actions, executed after each other
 *  if one's preconditions fail the sequence is interrupted
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAActionSequence : public USoEAction
{
	GENERATED_BODY()
public:

	virtual float Evaluate(const ASoEnemy* Owner) const override;

	virtual void Interrupt(ASoEnemy* Owner) override;

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	TArray<USoEAction*> ActionSequence;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	int32 LoopCount = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	bool bCheckPreconditionEachLoop = true;

	int32 ActiveSequenceID = -1;

	int32 LoopCounter = 0;
};



/**
 *  Selects an action randomly from an array of actions
 *  SubAction preconditions are ignored, only the root matters on evaluation
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEARandomer : public USoEAction
{
	GENERATED_BODY()
public:

	virtual void Interrupt(ASoEnemy* Owner) override;

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	float GetRepeatAvoidanceMultiplier(int32 Index) const;

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	TArray<USoEAction*> Actions;

	/** 1.0f by default for out of range, x-th element goes for x-th action in Actions */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	TArray<float> Chances;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Params)
	bool bLowerChanceToRepeat = false;

	int32 PrevSelectedIndex = -1;
	int32 PrevPrevSelectedIndex = -1;

	USoEAction* ActiveAction = nullptr;
};




/**
 *  An array of actions
 *  If any is satisfied it is as well - own preconditions are taken into account as well
 *  One action is selected and executed on enter
 *  Evaluate returns with the max value of Evaluate calls on the action array
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAActionList : public USoEAction
{
	GENERATED_BODY()
public:

	virtual float Evaluate(const ASoEnemy* Owner) const override;

	virtual void Interrupt(ASoEnemy* Owner) override;

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

protected:

	UPROPERTY(Instanced, BlueprintReadWrite, EditAnywhere, Category = Params)
	TArray<USoEAction*> Actions;

	USoEAction* ActiveAction = nullptr;
};
