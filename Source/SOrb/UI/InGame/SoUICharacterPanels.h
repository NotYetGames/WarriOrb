// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/InGame/SoUIGameActivity.h"

#include "SoUICharacterPanels.generated.h"

class USoUIButtonArray;
class ASoPlayerController;
class USoInGameUIActivity;
class UFMODEvent;

/**
 *
 */
UCLASS()
class SORB_API USoUICharacterPanels : public USoInGameUIActivity
{
	GENERATED_BODY()

public:
	// Begin UUserWidget Interface
	void NativeConstruct() override;
	// End UUserWidget Interface

protected:
	// USoInGameUIActivity Interface
	bool SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable) override;
	bool HandleCommand_Implementation(ESoUICommand Command) override;
	bool Update_Implementation(float DeltaSeconds) override;
	bool IsActiveInPausedGame_Implementation() const override { return true;  }
	bool IsActiveInDilatedTime_Implementation(float CurrentTimeDilation) const override { return true; }

	bool IsMenuBackCommand(ESoUICommand Command) const;
	USoInGameUIActivity* GetSelectedSubPanel() const;

protected:

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoUIButtonArray* SubMenus = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoInGameUIActivity* CharacterSheet = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoInGameUIActivity* Inventory = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoInGameUIActivity* Spells = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Widgets", meta = (BindWidget))
	USoInGameUIActivity* Notes = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = ">Runtime")
	TArray<USoInGameUIActivity*> SubPanels;

	UPROPERTY(BlueprintReadWrite, Category = ">Runtime")
	USoInGameUIActivity* PanelToOpenOnNextStartOverride = nullptr;

	UPROPERTY()
	bool bShouldIgnoreMenuBackCommand = false;

	UPROPERTY()
	bool bOpened = false;

	//
	// SFX
	//

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXPanelSwitch = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXOpen = nullptr;

	UPROPERTY(EditAnywhere, Category = ">SFX")
	UFMODEvent* SFXClose = nullptr;
};
