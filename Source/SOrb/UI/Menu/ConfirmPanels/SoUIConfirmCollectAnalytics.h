// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "UI/General/SoUIConfirmPanel.h"

#include "SoUIConfirmCollectAnalytics.generated.h"


UCLASS()
class SORB_API USoUIConfirmCollectAnalytics : public USoUIConfirmPanel
{
	GENERATED_BODY()

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeConstruct() override;

protected:
	//
	// USoUIEventHandler Interface
	//
	bool OnUICommand_Implementation(ESoUICommand Command) override;

	//
	// USoUIConfirmPanel interface
	//

	void RefreshButtonsArray() override;
	bool OnPressedButtonTooltipsCommand(ESoUICommand Command) override;

protected:
	static constexpr ESoUICommand UICommand_DoNotSend = ESoUICommand::EUC_Action1;
	static constexpr ESoUICommand UICommand_SendAutomatically = ESoUICommand::EUC_Action0;
};
