// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/General/SoUITypes.h"

#include "SoUIGameActivity.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSoUIActivity, Log, All);

/**
 *  Base class for Character/User input related UI logic / activities (character panels, looting, puzzle control window, etc.)
 *  USoAInUI character activity handles these, any child class can only be active while the character is in the USoAInUI state
 *  It could be an interface but then we could not list the implementations via TSubclassOf<>
 */
UCLASS(HideCategories = (Navigation), AutoCollapseCategories = (Performance, Clipping, Interaction, Input))
class SORB_API USoInGameUIActivity : public UUserWidget
{
	GENERATED_BODY()

public:

	USoInGameUIActivity(const FObjectInitializer& PCIP);

	/**
	 *  Tries to activate/deactivate the object
	 *  Activation may fail if the Source is invalid
	 *  or the widget does not intend to be activated based on the source right now
	 *  @param Source: optional, the object responsible for the control (e.g. the chest the character tries to loot)
	 *  @param bEnable: whether enable or disable is requested (character is interrupted -> disable call)
	 *  @Return: if activated after the call
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
	bool SetInGameActivityEnabled(UObject* Source, bool bEnable);

	/**
	 *  Input handling - should be only called on active InGameUIActivity
	 *  @Return: true if the widget handled the command or it does not want to let others handle it
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
	bool HandleCommand(ESoUICommand Command);

	/**
	 *  Frame update - should be only called on active InGameUIActivity
	 *  @Return: if the widget is still active
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = UI)
	bool Update(float DeltaSeconds);

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = UI)
	bool IsOpened() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = UI)
	bool ShouldHideUIElements() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = UI)
	bool ShouldKeepMusicFromPreviousUIActivity() const;

	// True if this activity can be used in paused mode
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = UI)
	bool IsActiveInPausedGame() const;

	// True if this activity with the current CurrentTimeDilation
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = UI)
	bool IsActiveInDilatedTime(float CurrentTimeDilation) const;

	// HACK: Fix me, because ActionBack conflicts with MenuBack
	// Hack similar to the bIgnoredFirstUICommand in USoUISpellCastSwitch
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = UI)
	bool ShouldParentIgnoreTheNextMenuBack() const;

	UFUNCTION(BlueprintPure, Category = UI)
	bool IsArmedStateAllowedOnLeave() const { return bArmedStateAllowedOnLeave; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ">UIActivity")
	bool bRegisterAsActivity = true;

	UPROPERTY(EditAnywhere, Category = ">UIActivity")
	bool bArmedStateAllowedOnLeave = true;
};
