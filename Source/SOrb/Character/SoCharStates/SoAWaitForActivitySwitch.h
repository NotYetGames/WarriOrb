// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"

#include "SoAWaitForActivitySwitch.generated.h"

class ALevelSequenceActor;
class ASoMarker;

UCLASS()
class SORB_API USoASoAWaitForActivitySwitch : public USoActivity
{
	GENERATED_BODY()

public:

	USoASoAWaitForActivitySwitch();

	virtual void Tick(float DeltaSeconds) override;

	virtual void Move(float Value) override {}
	virtual void StrikePressed() override {}
	virtual void RightBtnPressed() override {}
	virtual void JumpPressed() override {}
	virtual void RollPressed() override {}
	virtual void ToggleWeapons() override {}
	virtual void ToggleItems() override {}
	virtual void UseItemFromSlot0() override {}

	virtual void InteractKeyPressed(bool bPrimary) override {}

	FORCEINLINE virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override
	{
		return	((ActivityToWaitFor == EActivity::EA_Max && DesiredState != EActivity::EA_InUI) ||
				  ActivityToWaitFor == DesiredState);
	}

	/** if UISource is not nullptr UIActivityClass will be entered via SoAInUI */
	UFUNCTION(BlueprintCallable, Category = Interact)
	bool Enter(EActivity InActivityToWaitFor, bool bForce, bool bInArmed = false);

	bool Enter(ALevelSequenceActor* SequenceActor,
			   AActor* InCameraActor,
			   UAnimSequenceBase* AnimToPlay,
			   ASoMarker* TelTargetAfterSequence,
			   bool bInTriggerAreaChangeAfterSequence,
			   bool bInHideUI = false);

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

protected:

	UPROPERTY(BlueprintReadWrite)
	EActivity ActivityToWaitFor;

	UPROPERTY()
	ALevelSequenceActor* LevelSequenceActor;

	UPROPERTY()
	ASoMarker* TeleportAfterSequence = nullptr;

	UPROPERTY()
	AActor* CameraActor;

	/** reseted from bp on playback end */
	UPROPERTY(BlueprintReadWrite)
	UAnimSequenceBase* AnimationToPlay = nullptr;

	float Counter = -1.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bAnimationFinished = false;

	UPROPERTY(BlueprintReadWrite)
	bool bPingAnimation = true;

	bool bTriggerAreaChangeAfterSequence = false;

	bool bHideUI = false;
};
