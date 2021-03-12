// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "SoEditorStyle.h"
#include "Framework/Commands/Commands.h"

class FSoEditorCommands : public TCommands<FSoEditorCommands>
{
public:
	FSoEditorCommands()
		: TCommands<FSoEditorCommands>(
			TEXT("SOrbEditor"), // Context name for fast lookup, used by  the SoEditorStyle
			 FText::FromString(TEXT("Sorb Editor")),
			NAME_None, // Parent
			FSoEditorStyle::GetStyleSetName()) // Icon Style Set
	{
	}

	// TCommand<> interface
	void RegisterCommands() override;
	// End of TCommand<> interface

public:
	TSharedPtr<FUICommandInfo> SaveAllItems;
	TSharedPtr<FUICommandInfo> SaveAllEffectInstances;
	TSharedPtr<FUICommandInfo> SaveAllCharacterStrikes;

	TSharedPtr<FUICommandInfo> LoadCameraData;
	TSharedPtr<FUICommandInfo> SaveCameraData;

	TSharedPtr<FUICommandInfo> ToggleSplinePositionCorrection;
	TSharedPtr<FUICommandInfo> ToggleDisplayCameraKeys;

	TSharedPtr<FUICommandInfo> RebuildEnemyGroups;

	TSharedPtr<FUICommandInfo> BuildAll;
	TSharedPtr<FUICommandInfo> CopyActorsToClipboard;
};
