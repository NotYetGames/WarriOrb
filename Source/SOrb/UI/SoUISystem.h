// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"

#include "SoUISystem.generated.h"

class USoUIMenuMain;
class USoUIInGame;
class USoUIVideoPlayer;

/**
 *
 */
UCLASS()
class SORB_API USoUISystem : public UUserWidget
{
	GENERATED_BODY()

public:
	USoUIMenuMain* GetMainMenu() const;

	// NOTE: All USoInGameUIActivity are also managed from USoAInUI
	USoUIInGame* GetInGameUI() const;

	USoUIVideoPlayer* GetVideoPlayer() const;

protected:

	/** Contains everything Menu related - settings panel, save/load, etc. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USoUIMenuMain* MainMenu = nullptr;

	/** contains ingame elements including basic GUI, character panels, loot UI, etc. */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USoUIInGame* InGameUI = nullptr;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	USoUIVideoPlayer* VideoPlayer = nullptr;
};
