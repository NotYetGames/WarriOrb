// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"

#include "SoAInteractWithEnvironment.generated.h"

class USoInGameUIActivity;
class UAnimSequenceBase;

/** character is too busy browsing his inventory so he can't do anything else */
UCLASS()
class SORB_API USoAInteractWithEnvironment : public USoActivity
{
	GENERATED_BODY()

public:

	USoAInteractWithEnvironment();

	virtual void Tick(float DeltaSeconds) override;

	virtual void Move(float Value) override {};
	virtual void StrikePressed() override {};
	virtual void RightBtnPressed() override {};
	virtual void JumpPressed() override {};
	virtual void RollPressed() override {};
	virtual void ToggleWeapons() override {};
	virtual void ToggleItems() override {};
	virtual void UseItemFromSlot0() override {};

	virtual void InteractKeyPressed(bool bPrimary) override {};

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return bCanBeInterrputed; }

	/** if UISource is not nullptr UIActivityClass will be entered via SoAInUI */
	UFUNCTION(BlueprintCallable, Category = Interact)
	bool Enter(UAnimSequenceBase* AnimToPlay,
			   const TSubclassOf<USoInGameUIActivity> InUIActivityClass,
			   UObject* InUISource = nullptr,
			   float InCounterTarget = 0.0f,
			   bool bInUseAnimBlendIn = true);

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;

	float Counter = -1.0f;

	float CounterTarget = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimSequenceBase* Anim = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseAnimBlendIn = true;


	/** data to use after animation with UI if set */
	UPROPERTY()
	UObject* UISource = nullptr;

	/** UI to use after animation if set */
	UPROPERTY()
	TSubclassOf<USoInGameUIActivity> UIActivityClass;

	bool bCanBeInterrputed = false;
};
