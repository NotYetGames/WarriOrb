// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISystem.h"

#include "Menu/SoUIMenuMain.h"
#include "InGame/SoUIInGame.h"
#include "General/SoUIVideoPlayer.h"

USoUIMenuMain* USoUISystem::GetMainMenu() const { return MainMenu; }
USoUIInGame* USoUISystem::GetInGameUI() const { return InGameUI; }
USoUIVideoPlayer* USoUISystem::GetVideoPlayer() const { return VideoPlayer; }
