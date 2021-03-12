// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoActivity.h"
#include "UI/InGame/SoUIGameActivity.h"

#include "SoAInUI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoUIStateNotify);

class USoInGameUIActivity;
class UDlgDialogue;

/** character is too busy browsing his inventory so he can't do anything else */
UCLASS()
class SORB_API USoAInUI : public USoActivity
{
	GENERATED_BODY()

public:

	USoAInUI();

	virtual void Tick(float DeltaSeconds) override;

	// can't do anything while beeing way too busy reading my epic item descriptions
	virtual void Move(float Value) override {};
	virtual void StrikePressed() override {};
	virtual void RightBtnPressed() override {};
	virtual void JumpPressed() override {};
	virtual void RollPressed() override {};
	virtual void UseItemFromSlot0() override {};
	virtual void ToggleSpells(bool bQuickSelectionMode) override;

	virtual void InteractKeyPressed(bool bPrimary) override {};

	virtual void HandleUICommand(FKey Key, ESoUICommand Command) override;

	virtual bool ShouldDecreaseCollision() const override { return bLastCollisionWasDecreased; }

	virtual bool CanBeInterrupted(EActivity DesiredState = EActivity::EA_Max) const override { return true; }
	virtual void TakeWeaponAway() override {};

	virtual bool BlocksTeleportRequest() const override { return true; }
	virtual bool IsSaveAllowed() const override;

	/**
	 *  Function may called when it is not the active UI activity
	 *  @Param Source: object, can be nullptr or e.g. the chest the character loots
	 *  @Param UIActivityClass: class of the handler UMGWidget
	 */
	UFUNCTION(BlueprintCallable, Category = UI)
	bool Enter(UObject* Source, const TSubclassOf<USoInGameUIActivity>& UIActivityClass, bool bForceOpen = false);

	/** More BP friendly version, old one stays because idk if the removed & would cause problems or not in BP */
	UFUNCTION(BlueprintCallable, Category = UI)
	bool EnterBP(UObject* Source, const TSubclassOf<USoInGameUIActivity> UIActivityClass) { return Enter(Source, UIActivityClass); }

	/** Tries to start the dialogue, send the context to UI control and activate this activity */
	UFUNCTION(BlueprintCallable, Category = UI)
	bool StartDialogue(UDlgDialogue* Dialogue, const TArray<UObject*>& Participants, bool bForced = false);

	/** Tries to start the dialogue, send the context to UI control and activate this activity */
	UFUNCTION(BlueprintCallable, Category = UI)
	void StartDialogue2(UDlgDialogue* Dialogue, UObject* Participant, bool bForced = false);

	UFUNCTION(BlueprintCallable, Category = UI)
	void ToggleCharacterPanels();

	UFUNCTION(BlueprintCallable, Category = UI)
	void RegisterInGameActivity(USoInGameUIActivity* InGameActivity);

	UFUNCTION(BlueprintCallable, Category = UI)
	void UnregisterInGameActivity(USoInGameUIActivity* InGameActivity);

	void StartSpellCastSwitch() { Enter(nullptr, SpellCastSwitchClass); }

	bool IsSpellCastSwitchOpen() const;

	UFUNCTION(BlueprintCallable, Category = UI)
	void StartEndGamePanel() { Enter(nullptr, EndGameClass); }

	UFUNCTION(BlueprintCallable, Category = UI)
	void OpenRestPanel() { Enter(nullptr, RestClass); }

	// true for Wizard Dialogue and full screen dialogue (not for Lillian dialogue tho)
	bool IsInDialogue() const;


	virtual bool CanPauseOnIdle() const { return false; }

protected:

	virtual void OnEnter(USoActivity* OldActivity) override;
	virtual void OnExit(USoActivity* NewActivity) override;
	virtual void OnPostExit(USoActivity* NewActivity) override;

	// Gets the game activity from the class
	USoInGameUIActivity* GetInGameUIActivity(const TSubclassOf<USoInGameUIActivity>& Class) const;

	virtual bool CanBeArmedState() const override { return true; }

	void Save();

public:

	UPROPERTY(BlueprintAssignable)
	FSoUIStateNotify OnUILeft;

protected:

	UPROPERTY()
	USoInGameUIActivity* ActiveUIActivity = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<TSubclassOf<USoInGameUIActivity>, USoInGameUIActivity*> InGameActivities;

	bool bLastCollisionWasDecreased = false;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USoInGameUIActivity> CharacterPanelsClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USoInGameUIActivity> SpellCastSwitchClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USoInGameUIActivity> RestClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USoInGameUIActivity> DialoguePanelClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USoInGameUIActivity> EndGameClass;

	// used to store the ui activity, but only if we want to return to it after the new one is closed
	// to do that the source has to be the current UI activity in Enter()
	UPROPERTY(BlueprintReadWrite)
	USoInGameUIActivity* LastIngameActivity;

	bool bDialogueWasLast = false;

	UPROPERTY(BlueprintReadWrite)
	USoActivity* ActivityToEnterAfterLeave = nullptr;

	bool bArmAllowed = true;

	UPROPERTY(BlueprintReadWrite)
	float InputBlockCounter = -1.0f;

	bool bForceAllowSave = false;

	bool bRollAllowed = false;
};
