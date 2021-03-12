// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

class UWorld;

DECLARE_LOG_CATEGORY_EXTERN(LogSoCameraDataIO, Log, All);

/**
* Interface for the editor buttons
*/
class SORB_API FSoEditorGameInterface
{
public:

	static bool LoadCameraData(UWorld* World);
	static bool SaveCameraData(UWorld* World);

	static bool ToggleSplinePosCorrection();
	static bool IsSplinePosCorrectionEnabled();

	/**
	 *  Gathers the Group data from the enemies and enemy spawners placed in the level and exports it to a config the character will load
	 *  Do not ever call this function from game!!!
	 */
	static bool BuildEnemyGroupDataConfigFile(UWorld* World);

	/** Adds unique prefix if it isn't there yet */
	static void FixEnemyGroupName(FName& InOutName);

	static FString GetEnemyGroupDataConfigPath(UWorld* World);
};
