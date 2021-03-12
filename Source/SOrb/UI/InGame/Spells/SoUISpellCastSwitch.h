// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGame/SoUIGameActivity.h"

#include "SoUISpellCastSwitch.generated.h"

class USoUISpellCastSwitchSlot;
class UTextBlock;
class ASoCharacter;
class ASoPlayerController;
class USoUICommandTooltip;
class UCanvasPanel;
class USoPlayerCharacterSheet;
class UWorld;
class UFMODEvent;
class USoItemTemplateRuneStone;


/** Quick spell cast switch */
UCLASS()
class SORB_API USoUISpellCastSwitch : public USoInGameUIActivity
{
	GENERATED_BODY()
	typedef USoUISpellCastSwitch Self;
public:
	void SetQuickSelectionMode(bool bInValue);
	bool CanBeOpened() const;

protected:
	// Begin UUserWidget Interface
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void NativeConstruct() override;
	void NativeDestruct() override;
	// End UUserWidget Interface

	// Begin USoInGameUIActivity Interface
	bool SetInGameActivityEnabled_Implementation(UObject *Source, bool bEnable) override;
	bool HandleCommand_Implementation(ESoUICommand Command) override;
	bool Update_Implementation(float DeltaSeconds) override;
	bool IsOpened_Implementation() const override { return bOpened; }
	bool IsActiveInPausedGame_Implementation() const override { return true; }
	bool IsActiveInDilatedTime_Implementation(float CurrentTimeDilation) const override { return true; }
	// End USoInGameUIActivity Interface

	void SetOpen(bool bOpen);

	int32 ConvertCircleSpellIndexToTrueIndex(int32 CircleSlotIndex) const;
	int32 ConvertTrueIndexToCircleSpellIndex(int32 TrueIndex) const;

	int32 GetCircleIndexForSpellSlot(const USoUISpellCastSwitchSlot* SpellSlot) const;
	int32 GetTrueIndexForSpellSlot(const USoUISpellCastSwitchSlot* SpellSlot) const;

	void ReinitializeSpells();
	void SetCurrentCircleSpellIndex(int32 NewSpellIndex, bool bForce = false);

	void UpdateHintsVisibilities();

	UFUNCTION()
	void HandleDeviceTypeChanged(ESoInputDeviceType InDeviceType);

	void SubscribeToDeviceChanged();
	void UnSubscribeFromDeviceChanged();

protected:
	float PreviousTickDirectionDegrees = 0.f;

	// Are we fast changing
	bool bFastChangingNeighbour = false;

	// How many times did we skip
	int32 CountChanges = 0;

	// Is this still opened
	bool bOpened = false;

	// HACK: pls fix me
	bool bIgnoredFirstUICommand = false;

	// Means this was opened by moving the right thumbstick
	bool bQuickSelectionMode = false;

	// When we close it timers
	float TickShouldCloseQuickSelectionStart = 0.f;

	// When we allow to open again quickly
	float TickOpenAgainQuickSelectionStart = 0.f;

	static constexpr float ThresholdShouldCloseQuickSelection = 0.2f;
	static constexpr float ThresholdOpenAgainQuickSelection = 0.3f;

	// NOTE: This is not the same as, it must be converted to that ActiveEquippedRuneStoneIndex
	// NOTE: Index for CircleLinearSpellsForEquipped
	int32 CurrentCircleSpellSlotIndex = 0;

	// NOTE: The order here is the same as the circler order, clockwise starting from N
	UPROPERTY(BlueprintReadWrite, Category = ">Runtime")
	TArray<USoUISpellCastSwitchSlot*> CircleLinearSpellsForEquipped;

	// Our custom order
	// Index: Equipped spell index from EquippedRuneStones
	// Value: Our Slot
	UPROPERTY(BlueprintReadWrite, Category = ">Runtime")
	TArray<USoUISpellCastSwitchSlot*> SpellsForEquipped;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UTextBlock* SelectedSpellNameText;

	// cardinal coordinates of of all eight slots
	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_N;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_S;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_E;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_W;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_SW;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_SE;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_NW;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUISpellCastSwitchSlot* Slot_NE;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* TooltipSelect;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUICommandTooltip* TooltipSelectAndCast;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	UCanvasPanel* KeyboardShortcutsContainer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Options")
	FName SpellSelectedName = TEXT("UISpellCastSelectedName");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Options")
	FName SpellSelectedAndCastedName = TEXT("UISpellCastSelectedAndCastedName");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = ">Options")
	FName KeyboardQuickSelectSpellsName = TEXT("UIKeyboardQuickSelectSpellsName");

	// Cached properties
	UPROPERTY()
	ASoCharacter* SoCharacter = nullptr;

	UPROPERTY()
	ASoPlayerController* SoController = nullptr;

	UPROPERTY()
	USoPlayerCharacterSheet* SoSheet = nullptr;

	//
	// Audio
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	float AudioSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	float AudioSlowDownTimeIn = 0.1f;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	float AudioSlowDownTimeOut = 0.1f;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOnOpen = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOnClose = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXSelect = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXSelectAndCast = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXNavigate = nullptr;
};
