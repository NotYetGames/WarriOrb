// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once


#include "CoreMinimal.h"
#include "UI/General/SoUIConfirmPanel.h"

#include "SoUIConfirmKeyboardSettings.generated.h"

class UTextBlock;


UCLASS()
class SORB_API USoUIConfirmKeyboardSettings : public USoUIConfirmPanel
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetOpenedForResetToWASD(bool bEnabled) { bOpenedForResetToWASD = bEnabled; }

protected:
	//
	// USoUIEventHandler Interface
	//
	void Open_Implementation(bool bOpen) override;
	bool OnUICommand_Implementation(ESoUICommand Command) override;

	//
	// USoUIConfirmPanel interface
	//

	void RefreshButtonsArray() override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

	//
	// Own methods
	//

	// Apply the preset for the WASD keys
	UFUNCTION(BlueprintCallable)
	void OnApplyWASDPreset();

	// Apply the preset for the arrow keys
	UFUNCTION(BlueprintCallable)
	void OnApplyArrowsPreset();

protected:
	static constexpr ESoUICommand UICommand_Apply = ESoUICommand::EUC_Action0;
	static constexpr ESoUICommand UICommand_Cancel = ESoUICommand::EUC_Action1;

	bool bOpenedForResetToWASD = false;
};
