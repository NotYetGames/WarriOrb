// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoASwing.generated.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UCLASS()
class SORB_API USoASwing : public USoActivity
{
	GENERATED_BODY()
public:

	USoASwing();

	virtual void Tick(float DeltaSeconds) override;

	// inputs
	virtual void JumpPressed() override;
	// virtual void Move (float Value) override;

	virtual void OnTeleportRequest() override;

	// bad idea, don't override this: mesh would not be inside the capsule
	// virtual bool ShouldDecreaseCollision() const override { return true; }

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MovementConstValue)
	float SwingInputForce = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MovementConstValue)
	float SwingLeaverJumpVelocity = 1000.f;

	UPROPERTY(BlueprintReadOnly, Category = SwingAnimData)
	bool bForward;

	UPROPERTY(BlueprintReadOnly, Category = SwingAnimData)
	bool bLeftHanded = true;

	friend class USoCharacterMovementComponent;
};

