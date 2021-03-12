// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "GameFramework/HUD.h"

#include "SoHUD.generated.h"


UCLASS()
class SORB_API ASoHUD : public AHUD
{
	GENERATED_BODY()
public:

	// AHUD interfafce

	// NOTE: seems to be not used by anybody
	void OnLostFocusPause(bool bEnable) override;

	/** Gives the HUD a chance to display project-specific data when taking a "bug" screen shot.	 */
	void HandleBugScreenShot() override;
};
