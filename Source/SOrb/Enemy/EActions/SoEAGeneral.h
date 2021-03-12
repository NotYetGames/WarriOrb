// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"
#include "Enemy/SoEnemyDataTypes.h"
#include "SoEAGeneral.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Simple wait action - the enemy doesn't do anything for the given Duration
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAWait : public USoEAction
{
	GENERATED_BODY()
public:

	void SetDuration(float DeltaSeconds) { Duration = DeltaSeconds; }

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Duration;

	/** > 0: duration is randomized on enter in range Duration +- Range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Range = -1.0f;

	float RandomFactor = 0.0f;

	float RestTime = 0.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  Action to change one of the active actionlist to one of the named actionlist
 */
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAChangeActionSet : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	ESoActionList NewActionList;

	/** the action is executed instantly, but after that it waits Duration seconds before it says it is done */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Params)
	float Duration = 0.0f;

	float RestTime;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAModifyFloat : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	ESoEnemyFloat Variable;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bDelta = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float Value = 0.0f;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEABPEvent : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEABPEventIndexed : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index;

	/** used if >= Index, then all index is called in range */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 MaxIndex = -1;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAPlayMaterialAnimation : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	int32 Index = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEASetAnim : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName TargetName;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName SourceName;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEASetForcedLookAtPlayer : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bActivate = true;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEASetDynamicName : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName ValueToSet;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEASetVOOn : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bEnable = true;
};
