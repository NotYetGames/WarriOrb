// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "SoAItemUsage.generated.h"

UCLASS()
class SORB_API USoAItemUsage : public USoActivity
{
	GENERATED_BODY()

public:

	USoAItemUsage ();

	virtual void Tick(float DeltaSeconds) override;

	virtual void OnAnimEvent(EAnimEvents Event) override;

	virtual void JumpPressed() override {};
	virtual void Move(float Value) override;

	virtual void ToggleItems() override {};
	virtual void UseItemFromSlot0() override;

	virtual bool ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const override;
	virtual void OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal) override;

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return false; }


	USoActivity* StoredActivity;

	UPROPERTY(BlueprintReadOnly)
	ESoUsableItemType UsableType;

	UPROPERTY(BlueprintReadOnly)
	bool bCanBeUsed;


	UPROPERTY(BlueprintReadOnly)
	UAnimSequenceBase* PingAnimation;

	UPROPERTY(BlueprintReadOnly)
	UAnimSequenceBase* PongAnimation;

	UPROPERTY(BlueprintReadOnly)
	bool bPingState;

	UPROPERTY(EditAnywhere)
	TMap<ESoUsableItemType, UAnimSequenceBase*> Animations;

	float Counter = 0.0f;
	float MeshShowThreshold;
};

