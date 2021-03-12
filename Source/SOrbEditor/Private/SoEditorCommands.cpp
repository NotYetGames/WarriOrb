// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEditorCommands.h"
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "WarriorbEditor"

void FSoEditorCommands::RegisterCommands()
{
	UI_COMMAND(LoadCameraData, "Camera Load", "Loads camera data", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(SaveCameraData, "Camera Save", "Saves camera data", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToggleSplinePositionCorrection, "TSC", "Toggles spline location correction", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(ToggleDisplayCameraKeys, "CamKeys", "Toggles visualization of camera keys", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(RebuildEnemyGroups, "EnemyGroups", "Rebuilds Enemy Groups", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(BuildAll, "Build All", "Warriorb Build all Version. Builds all levels (precomputes lighting data and visibility data, generates navigation networks and updates brush models.)", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CopyActorsToClipboard, "Copy selected Actors to clipboard", "Copy selected Actors into an Array (in clipboard) so that you can paste it in the property editor", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SaveAllItems,
		"Save All Items",
		"Saves all Items to the disk",
		EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SaveAllEffectInstances,
		"Save All Effect Instances",
		"Saves all effect instances to the disk",
		EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SaveAllCharacterStrikes,
		"Save All Character Strikes",
		"Saves all Character Strikes to the disk",
		EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
