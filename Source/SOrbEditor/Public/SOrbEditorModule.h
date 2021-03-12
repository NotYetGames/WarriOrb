// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Modules/ModuleInterface.h"
#include "Logging/LogCategory.h"
#include "IAssetTypeActions.h"
#include "AssetTypeCategories.h"
#include "SoEditorUtilitiesTypes.h"
#include "Containers/Ticker.h"

class FLightingBuildOptions;
class FToolBarBuilder;
class FExtender;
class STextBlock;
class FUICommandList;

DECLARE_LOG_CATEGORY_EXTERN(LogSoEditor, All, All)

enum class ESoLightBuildStatus : uint8
{
	None = 0,

	Loading_WaitLevelChange,

	Building,
	Failed,
	Succeeded,
	Succeeded_Wait
};

enum class ESoBuildAllType : uint8
{
	All = 0,

	AllEpisodes,
	AllChapters,

	// Build only one chapter
	OnlyLevelsInChapter
};


class FSOrbEditorModule : public IModuleInterface, public FTickerObjectBase
{
	typedef FSOrbEditorModule Self;
public:
	/** IModuleInterface implementation */
	void StartupModule() override;
	void ShutdownModule() override;

	bool IsTickable() const;
	bool Tick(float DeltaTime) override;

	static bool SaveAllItems();
	static bool SaveAllEffectInstances();
	static bool SaveAllCharacterStrikes();

protected:

	static void LoadCameraData();
	static bool CanLoadCameraData() { return true; }

	static void SaveCameraData();
	static bool CanSaveCameraData() { return true; }

	static void RebuildEnemyGroups();
	static bool CanRebuildEnemyGroups() { return true; }

	void CopyActorsToClipboard();
	bool CanCopyActorsToClipboard() const { return true;  }

	static void ToggleSplinePositionCorrection();
	static bool CanToggleSplinePositionCorrection() { return true; }
	static bool IsSplinePositionCorrectionEnabled() { return bIsSplinePositionCorrectionEnabled; }

	static void ToggleDisplayCameraKeys();
	static bool CanToggleDisplayCameraKeys() { return true; }
	static bool IsDisplayCameraKeysEnabled();

	static void OnModuleChanged(FName ModuleName, EModuleChangeReason ChangeReason);

	/** Handle clicking on save all dialogues. */
	static void HandleOnSaveAllItems();
	static void HandleOnSaveAllEffectInstances();
	static void HandleOnSaveAllCharacterStrikes();


	// Loads all the USoItemTemplate
	void LoadAllItems();

	/** Extends the Top Toolbar */
	void ExtendToolbar();

	/** Extend the Menus of the editor */
	void ExtendMenu();

	// Set lights build presets
	bool SetChapterLevelsPresetTo(int32 ChapterPresetIndex, const int32 LevelsPresetIndex);
	bool SetChapterPresetTo(int32 ChapterPresetIndex);
	bool SetEpisodePresetTo(int32 EpisodePresetIndex);
	bool SetPersistentMapName(const FString& MapPackageName);
	bool SetSkyPresetTo(FName SkyName);
	void LogCurrentBuildPreset();
	void LoadBuildPresets();
	void AddNotificationBar(FToolBarBuilder& ToolBarBuilder);
	TSharedRef<SWidget> GenerateBuildPresetsMenu();
	void UpdateNotificationWidgets();

	// Build all, lights, geometry etc
	static void ConfigureLightingBuildOptions(const FLightingBuildOptions& Options);

	bool ConfirmUserBuildQuestion(const FString& Body);
	void RunEditorBuildAll();
	void PreStartBuild();

	// Episodes
	void StartBuildAllEpisodes(bool bAskConfirmation);
	void BuildCurrentEpisode();
	void FinishedBuildNextEpisode();

	// Chapters
	void StartBuildAllChapters(bool bAskConfirmation);
	void BuildCurrentChapter();
	void FinishedBuildNextChapter();

	// General methods for ALL
	void StartBuildAll();
	bool CanStartLightBuild() const;
	void FinishedBuildNext();

	void OnFinishedBuildingEpisodes();
	void OnFinishedBuildingChapters();
	void OnMapOpened(const FString&  Filename, bool bAsTemplate);
	void OnLightBuildStarted();
	void OnLightBuildFailed();
	void OnLightBuildSucceeded();
	void OnLightingBuildKept();
	void SubscribeToLightBuildEvents();
	void UnsubscribeFromLightBuildEvents();
	void ResetBuildSettings();

protected:
	// Hold the value of TSC check status.
	static bool bIsSplinePositionCorrectionEnabled;

	/** Hold reference to all the toolbar buttons */
	TSharedPtr<FUICommandList> CommandList;

	/** All created asset type actions. Cached here so that we can unregister them during shutdown. */
	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;

	/**
	 * File menu Editor commands bound from this plugin.
	 */
	TSharedPtr<FUICommandList> FileMenuEditorCommands;

	/** The submenu type of  for the sorb assets. */
	EAssetTypeCategories::Type WarriorbAssetCategoryBit;

	/** Holds all the build presets */
	FSoEditorBuildPresets BuildPresets;

	// Widgets
	TSharedPtr<FExtender> NotificationBarExtender;
	TSharedPtr<FExtender> ToolbarGameExtender;

	bool bShowNotificationBar = false;
	TSharedPtr<STextBlock> WidgetTextBuildType;
	TSharedPtr<STextBlock> WidgetTextBuildInfo;
	TSharedPtr<STextBlock> WidgetTextMap;

	// The current preset
	ESoLightBuildStatus LightStatus = ESoLightBuildStatus::None;
	ESoBuildAllType BuildAllType = ESoBuildAllType::All;
	int32 PreviousChapterIndexBuildPreset = INDEX_NONE;
	int32 ChapterIndexBuildPreset = INDEX_NONE;
	int32 ChapterLevelIndexBuildPreset = INDEX_NONE;
	int32 EpisodeIndexBuildPreset = INDEX_NONE;

	// Used when BuildAllType ==  ESoBuildAllType::OnlyLevelsInChapter
	int32 ChapterIndexToBuild = INDEX_NONE;

	bool bIsUsingChapterPreset = true;
	bool bSubscribedToLightEvents = false;

	float AfterLightCurrentSeconds = 0.f;
	static constexpr float AfterLightWaitSeconds = 5.f;

	// Old Values we used to modify
	bool bOldAutoApplyLightingEnable = false;
	bool bOldAutoSaveEnable = false;

};
