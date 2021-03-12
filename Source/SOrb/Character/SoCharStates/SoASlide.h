// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoASlide.generated.h"


UCLASS()
class SORB_API USoASlide : public USoActivity
{
	GENERATED_BODY()
public:
	USoASlide();

	virtual void Tick(float DeltaSeconds) override;
	
	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	virtual void OnBaseChanged(AActor* ActualMovementBase) override;

	// inputs
	virtual void JumpPressed() override;
	virtual void Move(float Value) override;

	virtual void HandleCollision() override;

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override;

protected:

	UPROPERTY(EditAnywhere)
	float CrouchSpeed = 1000;

	UPROPERTY(EditAnywhere)
	float CrouchJumpVelocity = 300;

	UPROPERTY(EditAnywhere)
	float JumpControlBanTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAllowStrikeInSlide = false;

	float DirModifier = 1.0f;

	bool bRollWasPressed = false;
	bool bWasInAir = false;

	friend class ASoCharacter;
	friend class USoActivity;
};
