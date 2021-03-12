// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "IAssetTypeActions.h"

class ASoSky;
class ULevelStreaming;
class ULevel;
class USoItemTemplate;
class USoEffectBase;
class USoCharacterStrike;

/**
 * Holds all the common functionality used by the SOrbEditor plugin
 */
class FSoEditorUtilities
{
public:
	/***
	 * Pops up a class picker dialog to choose the class that is a child of the Classprovided.
	 *
	 * @param	TitleText		The title of the class picker dialog
	 * @param	OutChosenClass  The class chosen (if this function returns false, this will be null) by the the user
	 * @param	Class			The children of this class we are displaying and prompting the user to choose from.
	 *
	 * @return true if OK was pressed, false otherwise
	 */
	static bool PickChildrenOfClass(const FText& TitleText, UClass*& OutChosenClass, UClass* Class);

	/**
	 * Gets the asset type actions for the specified UClass
	 *
	 * @param Class		The class we are trying to get the asset type action for.
	 *
	 * @return The shared pointer to t h AssetTypeAction corresponding for the class
	 */
	static TSharedPtr<IAssetTypeActions> GetAssetTypeActionsForClass(UClass* Class);

	// This is copied from  FEditorBuildUtils::EditorBuild only that it has no warnings
	static void EditorBuildAll(UWorld* InWorld);

	/** Saves all the dirty packages in the editor. */
	static bool SaveAllDirtyPackages(const int32 NumAttemptsSave = 6);

	/** Waits for all the levels in the editor to load. */
	static void WaitForLevelsInEditorToLoad();

	static bool IsAnyLevelInWorldLoading(const UWorld* World);

	/** Makes the levels inside LevelNamesToBeVisible to be visible.  */
	static bool MakeEditorLevelsVisible(const TArray<FName>& LevelNamesToBeVisible, FString& OutErrorMessage, bool bHideLevelsNotInArray = true);
	static bool MakeAllEditorLevelsVisible(FString& OutErrorMessage);

	static void MarkAllLevelsToBeLoaded();

	static UWorld* GetEditorWorld();

	/** Gets the ASoSky from the Editor world. */
	static bool GetEditorSoSky(ASoSky*& OutSky, FString& OutErrorMessage);

	// Adds an error notification
	static void AddNotificationError(const FString& ErrorMessage);

	static FString GetLevelShortName(const ULevelStreaming* StreamingLevel);
	static FString GetLevelShortName(const ULevel* Level);

	static FString GetLongPackageNameFromObject(const UObject* Object);

	// Loads and gets
	static TArray<USoItemTemplate*> GetAllItemsFromMemory();
	static int32 LoadAllItemsIntoMemory();

	static TArray<USoCharacterStrike*> GetAllCharacterStrikesFromMemory();
	static int32 LoadAllCharacterStrikesIntoMemory();

	// NOTE: most of these are blueprints
	static TArray<UObject*> GetAllEffectInstances();

private:
	FSoEditorUtilities() = delete;
};
