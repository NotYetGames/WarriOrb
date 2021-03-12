// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoEAction.h"
#include "Engine/EngineTypes.h"
#include "SoEACombat.generated.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// owner strikes
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class SORB_API USoEAStrike : public USoEAction
{
	GENERATED_BODY()
public:

	USoEAStrike();

	UFUNCTION(BlueprintCallable)
	FName GetStrikeName();

	UFUNCTION()
	void OnHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;

	void SetStopOnHit(bool bNewValue, ASoEnemy* Owner);

protected:

	void Update(ASoEnemy* Owner, const struct FSoStrikePhase& Phase, float OldTime, float NewTime);

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	bool bAreaLimitedRootMotion;

	float Counter;
	int32 ActivePhase;

	// controlled movement params
	float CachedMaxWalkSpeed;
	int32 ControlledMotionCounter = 0;
	bool bStopOnHit = false;

	bool bLeave = false;
};
inline FName USoEAStrike::GetStrikeName() { return Name; }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// owner's strike is blocked
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEABlocked : public USoEAction
{
	GENERATED_BODY()

protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float Duration;

	float Counter;
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// owner blocks a strike
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEABlock : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float Duration;

	float Counter;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS(BlueprintType, EditInlineNew)
class SORB_API USoEAHitReact : public USoEAction
{
	GENERATED_BODY()
protected:

	virtual void OnEnter(ASoEnemy* Owner) override;
	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
	virtual void OnLeave(ASoEnemy* Owner) override;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	FName Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
	float Duration;

	float Counter;
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UCLASS(BlueprintType, EditInlineNew)
//class SORB_API USoEARangeAttack : public USoEAction
//{
//	GENERATED_BODY()
//protected:
//
//	virtual void OnEnter(ASoEnemy* Owner) override;
//	virtual bool OnTick(float DeltaSeconds, ASoEnemy* Owner) override;
//	virtual void OnLeave(ASoEnemy* Owner) override;
//
//
//	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
//	FName Name;
//
//	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Params)
//	bool bOnSpline = true;
//
//
//	float Counter;
//	bool bProjectileCreated = false;
//};
