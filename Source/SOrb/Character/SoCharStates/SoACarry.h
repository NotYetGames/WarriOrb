// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoACarry.generated.h"

// TODO: maybe a single class instead of this 3 different?!

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoACarry : public USoActivity
{
	GENERATED_BODY()
public:

	USoACarry();

	virtual void Tick(float DeltaSeconds) override;

	// let's drop the junk we just carried
	virtual void OnInteract(AActor* Interactable) override;
	virtual void StrikePressed() override;

	virtual void OnTeleportRequest() override;

	virtual bool ShouldDecreaseCollision() const override { return false; }
	
	virtual bool CanInteract() const override;

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;


	void TryToDrop();
	bool CanDrop() const;

	bool bCanDrop = false;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoACarryDrop : public USoActivity
{
	GENERATED_BODY()
public:

	USoACarryDrop();

	virtual void Tick(float DeltaSeconds) override;

	virtual void OnAnimEvent(EAnimEvents Event) override;

	virtual void JumpPressed() override {};
	virtual void Move(float Value) override {};

	virtual void InteractKeyPressed(bool bPrimary) {};

	virtual void OnTeleportRequest() override;

	virtual bool ShouldDecreaseCollision() const override { return false; }

protected:
	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

protected:
	
	UPROPERTY(BlueprintReadOnly)
	float Duration = 0.1f;

	UPROPERTY(BlueprintReadOnly)
	float Counter = 0.0f;

	float DirectionSign = 1.0f;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UCLASS()
//class SORB_API USoACarryPickUp : public USoActivity
//{
//	GENERATED_BODY()
//public:
//
//	USoACarryPickUp();
//
//	virtual void Tick(float DeltaSeconds) override;
//
//	virtual void JumpPressed() override {};
//	virtual void Move(float Value) override {};
//
//	virtual void InteractKeyPressed(bool bPrimary) {};
//
//	virtual void OnTeleportRequest() override;
//
//	virtual bool ShouldDecreaseCollision() const override { return false; }
//
//protected:
//	virtual void OnEnter(USoActivity* OldActivity) override;
//	virtual void OnExit(USoActivity* NewActivity) override;
//
//	UPROPERTY(BlueprintReadOnly)
//	float Duration = 0.8f;
//
//	float PickedUpTime = 0.0f;
//	// float PickedUpTime = 0.5f;
//
//	UPROPERTY(BlueprintReadOnly)
//	float Counter = 0.0f;
//};
