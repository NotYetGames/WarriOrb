// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SOrbEditorModule.h"

#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyle.h"
#include "Editor.h"
#include "Engine/ObjectLibrary.h"
#include "AssetToolsModule.h"
#include "EngineUtils.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "EditorLevelUtils.h"
#include "FileHelpers.h"
#include "SoEditorUtilities.h"
#include "LightingBuildOptions.h"
#include "Settings/LevelEditorMiscSettings.h"
#include "Containers/Ticker.h"

#include "IO/DlgJsonParser.h"
#include "Objects/SoSky.h"
#include "SplineLogic/SoEditorGameInterface.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Settings/SoGameSettings.h"
#include "Items/ItemTemplates/SoItemTemplate.h"
#include "Effects/SoEffectBase.h"
#include "Character/SoCharacterStrike.h"

#include "SoEditorStyle.h"
#include "SoEditorCommands.h"
#include "NotYetUtilities.h"
#include "SoEditorUtilitiesTypes.h"
#include "ThumbnailRenderers/SoItemThumbnailRenderer.h"
#include "ThumbnailRenderers/SoCharacterStrikeThumbnailRenderer.h"
#include "TypeActions/AssetTypeActions_SoItem.h"
#include "TypeActions/AssetTypeActions_SoCharacterStrike.h"


static const FName NAME_MODULE_LevelEditor(TEXT("LevelEditor"));

IMPLEMENT_GAME_MODULE(FSOrbEditorModule, SOrbEditor)
DEFINE_LOG_CATEGORY(LogSoEditor)

bool FSOrbEditorModule::bIsSplinePositionCorrectionEnabled = false;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::StartupModule()
{
	UE_LOG(LogSoEditor, Verbose, TEXT("SOrbEditor: Log Started"));

	const FString Message = FString::Printf(
		TEXT("GPackageFileUE4Version = %d, GPackageFileLicenseeUE4Version = %d, Engine = %s"),
		GPackageFileUE4Version, GPackageFileLicenseeUE4Version, *FEngineVersion::Current().ToString()
	);
	UE_LOG(LogSoEditor, Verbose, TEXT("%s"), *Message);

	FModuleManager::Get().OnModulesChanged().AddLambda([](FName ModuleName, EModuleChangeReason ChangeReason)
	{
		FSOrbEditorModule::OnModuleChanged(ModuleName, ChangeReason);
	});

	// Register slate style ovverides
	FSoEditorStyle::Initialize();
	bIsSplinePositionCorrectionEnabled = FSoEditorGameInterface::IsSplinePosCorrectionEnabled();

	FSoEditorCommands::Register();

	// Register asset types, add the right click submenu
	// Make the Sorb assets be displayed in the filters menu and in the create new menu
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	WarriorbAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Warriorb")), FText::FromString(TEXT("Warriorb")));
#define REGISTER_TYPE_ACTIONS(TypeAction)                                    \
	{                                                                        \
	  auto Action = MakeShareable(new TypeAction(WarriorbAssetCategoryBit)); \
	  AssetTools.RegisterAssetTypeActions(Action);                           \
	  CreatedAssetTypeActions.Add(Action);                                   \
	}

	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoItemTemplate);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoShardItem);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoQuestItem);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoUsableItem);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoWeaponItem);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoItemTemplateJewelry);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoItemTemplateQuestBook);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoRuneStone);
	REGISTER_TYPE_ACTIONS(FAssetTypeActions_SoCharacterStrike);
#undef REGISTER_TYPE_ACTIONS

	// Register the thumbnail renderers
	UThumbnailManager::Get().RegisterCustomRenderer(
		USoItemTemplate::StaticClass(),
		USoItemThumbnailRenderer::StaticClass()
	);
	UThumbnailManager::Get().RegisterCustomRenderer(
		USoCharacterStrike::StaticClass(),
		USoCharacterStrikeThumbnailRenderer::StaticClass()
	);

	// Bind Editor commands
	FileMenuEditorCommands = MakeShared<FUICommandList>();
	FileMenuEditorCommands->MapAction(
		FSoEditorCommands::Get().SaveAllItems,
		FExecuteAction::CreateStatic(&Self::HandleOnSaveAllItems)
	);
	FileMenuEditorCommands->MapAction(
		FSoEditorCommands::Get().SaveAllEffectInstances,
		FExecuteAction::CreateStatic(&Self::HandleOnSaveAllEffectInstances)
	);
	FileMenuEditorCommands->MapAction(
		FSoEditorCommands::Get().SaveAllCharacterStrikes,
		FExecuteAction::CreateStatic(&Self::HandleOnSaveAllCharacterStrikes)
	);

	// Extend menu/toolbar
	ExtendToolbar();
	ExtendMenu();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		// Unregister the thumbnail renderers
		UThumbnailManager::Get().UnregisterCustomRenderer(USoItemTemplate::StaticClass());
		UThumbnailManager::Get().UnregisterCustomRenderer(USoCharacterStrike::StaticClass());
	}

	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}

	// Unregister level editor stuff
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

		if (NotificationBarExtender.IsValid())
			LevelEditorModule.GetNotificationBarExtensibilityManager()->RemoveExtender(NotificationBarExtender);

		if (ToolbarGameExtender.IsValid())
			LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(ToolbarGameExtender);
	}

	CreatedAssetTypeActions.Empty();
	FSoEditorCommands::Unregister();
	FSoEditorStyle::Shutdown();

	UE_LOG(LogSoEditor, Verbose, TEXT("SOrbEditor: Log Ended"));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::LoadAllItems()
{
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(USoItemTemplate::StaticClass(), true, GIsEditor);
	const TArray<FString> PathsToSeach = { TEXT("/Game") };
	ObjectLibrary->AddToRoot();
	ObjectLibrary->LoadAssetDataFromPaths(PathsToSeach);
	ObjectLibrary->LoadAssetsFromAssetData();
	ObjectLibrary->RemoveFromRoot();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::ExtendToolbar()
{
	// Running in game mode (standalone game) exit as we can't get the LevelEditorModule.
	if (IsRunningGame() || IsRunningCommandlet())
	{
		return;
	}

	// Register the toolbar commands
	CommandList = MakeShareable(new FUICommandList);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));

	CommandList->Append(LevelEditorModule.GetGlobalLevelEditorActions());

	CommandList->MapAction(
		FSoEditorCommands::Get().LoadCameraData,
		FExecuteAction::CreateStatic(&Self::LoadCameraData),
		FCanExecuteAction::CreateStatic(&Self::CanLoadCameraData)
	);

	CommandList->MapAction(
		FSoEditorCommands::Get().SaveCameraData,
		FExecuteAction::CreateStatic(&Self::SaveCameraData),
		FCanExecuteAction::CreateStatic(&Self::CanSaveCameraData)
	);

	CommandList->MapAction(
		FSoEditorCommands::Get().ToggleSplinePositionCorrection,
		FExecuteAction::CreateStatic(&Self::ToggleSplinePositionCorrection),
		FCanExecuteAction::CreateStatic(&Self::CanToggleSplinePositionCorrection),
		FIsActionChecked::CreateStatic(&Self::IsSplinePositionCorrectionEnabled)
	);

	CommandList->MapAction(
		FSoEditorCommands::Get().ToggleDisplayCameraKeys,
		FExecuteAction::CreateStatic(&Self::ToggleDisplayCameraKeys),
		FCanExecuteAction::CreateStatic(&Self::CanToggleDisplayCameraKeys),
		FIsActionChecked::CreateStatic(&Self::IsDisplayCameraKeysEnabled)
	);

	CommandList->MapAction(
		FSoEditorCommands::Get().RebuildEnemyGroups,
		FExecuteAction::CreateStatic(&Self::RebuildEnemyGroups),
		FCanExecuteAction::CreateStatic(&Self::CanRebuildEnemyGroups)
	);

	CommandList->MapAction(
		FSoEditorCommands::Get().CopyActorsToClipboard,
		FExecuteAction::CreateRaw(this, &Self::CopyActorsToClipboard),
		FCanExecuteAction::CreateRaw(this, &Self::CanCopyActorsToClipboard));

	CommandList->MapAction(
		FSoEditorCommands::Get().BuildAll,
		FExecuteAction::CreateRaw(this, &Self::StartBuildAll),
		FCanExecuteAction::CreateLambda([this] {
			return CanStartLightBuild();
		})
	);


	LoadBuildPresets();
	ToolbarGameExtender = MakeShareable(new FExtender());
	ToolbarGameExtender->AddToolBarExtension(
		TEXT("Game"),
		EExtensionHook::After,
		CommandList.ToSharedRef(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection(TEXT("SoOwnButtons"));
			ToolbarBuilder.AddToolBarButton(FSoEditorCommands::Get().LoadCameraData);
			ToolbarBuilder.AddToolBarButton(FSoEditorCommands::Get().SaveCameraData);
			ToolbarBuilder.AddToolBarButton(FSoEditorCommands::Get().ToggleSplinePositionCorrection);
			ToolbarBuilder.AddToolBarButton(FSoEditorCommands::Get().ToggleDisplayCameraKeys);
			ToolbarBuilder.AddToolBarButton(FSoEditorCommands::Get().RebuildEnemyGroups);
			ToolbarBuilder.AddToolBarButton(FSoEditorCommands::Get().CopyActorsToClipboard);

			// Can't know when a light build finishes :(
			//ToolbarBuilder.AddToolBarButton(FSoEditorCommands::Get().BuildAll);

			ToolbarBuilder.AddComboButton(
				FUIAction(
					FExecuteAction()
				),
				FOnGetContent::CreateRaw(this, &Self::GenerateBuildPresetsMenu),
				FText::FromString(TEXT("Build presets")),
				FText::FromString(TEXT("Build presets")),
				FSlateIcon(),
				true
			);

			ToolbarBuilder.EndSection();
		})
	);
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarGameExtender);

	// Notification Extender
	NotificationBarExtender = MakeShareable(new FExtender());
	NotificationBarExtender->AddToolBarExtension(
		TEXT("Start"),
		EExtensionHook::After,
		nullptr,
		FToolBarExtensionDelegate::CreateRaw(this, &Self::AddNotificationBar)
	);
	LevelEditorModule.GetNotificationBarExtensibilityManager()->AddExtender(NotificationBarExtender);
	LevelEditorModule.BroadcastNotificationBarChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SaveAllItems()
{
	FSoEditorUtilities::LoadAllItemsIntoMemory();
	const TArray<USoItemTemplate*> Items = FSoEditorUtilities::GetAllItemsFromMemory();
	TArray<UPackage*> PackagesToSave;

	for (USoItemTemplate* Item : Items)
	{
		Item->MarkPackageDirty();
		PackagesToSave.Add(Item->GetOutermost());
	}

	static constexpr bool bCheckDirty = false;
	static constexpr bool bPromptToSave = false;
	return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave) == FEditorFileUtils::EPromptReturnCode::PR_Success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SaveAllEffectInstances()
{
	const TArray<UObject*> Effects = FSoEditorUtilities::GetAllEffectInstances();
	TArray<UPackage*> PackagesToSave;

	for (UObject* Object : Effects)
	{
		Object->MarkPackageDirty();
		PackagesToSave.Add(Object->GetOutermost());
	}

	static constexpr bool bCheckDirty = false;
	static constexpr bool bPromptToSave = false;
	return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave) == FEditorFileUtils::EPromptReturnCode::PR_Success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SaveAllCharacterStrikes()
{
	FSoEditorUtilities::GetAllCharacterStrikesFromMemory();
	const TArray<USoCharacterStrike*> Strikes = FSoEditorUtilities::GetAllCharacterStrikesFromMemory();
	TArray<UPackage*> PackagesToSave;

	for (USoCharacterStrike* Strike : Strikes)
	{
		Strike->MarkPackageDirty();
		PackagesToSave.Add(Strike->GetOutermost());
	}

	static constexpr bool bCheckDirty = false;
	static constexpr bool bPromptToSave = false;
	return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave) == FEditorFileUtils::EPromptReturnCode::PR_Success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::HandleOnSaveAllItems()
{
	const EAppReturnType::Type Response = FPlatformMisc::MessageBoxExt(EAppMsgType::YesNo,
		TEXT("Save all item template assets?"),
		TEXT("Save Item Templates?")
	);
	if (Response == EAppReturnType::No)
	{
		return;
	}

	if (!SaveAllItems())
	{
		UE_LOG(LogSoEditor, Error, TEXT("Failed To save all items. An error occurred."));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::HandleOnSaveAllEffectInstances()
{
	const EAppReturnType::Type Response = FPlatformMisc::MessageBoxExt(EAppMsgType::YesNo,
		TEXT("Save all effect instances assets?"),
		TEXT("Save Effect Instances?")
	);
	if (Response == EAppReturnType::No)
	{
		return;
	}

	if (!SaveAllEffectInstances())
	{
		UE_LOG(LogSoEditor, Error, TEXT("Failed To save all effect instances. An error occurred."));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::HandleOnSaveAllCharacterStrikes()
{
	const EAppReturnType::Type Response = FPlatformMisc::MessageBoxExt(EAppMsgType::YesNo,
		TEXT("Save all Characther Strikes assets?"),
		TEXT("Save Characther Strikes?")
	);
	if (Response == EAppReturnType::No)
	{
		return;
	}

	if (!SaveAllCharacterStrikes())
	{
		UE_LOG(LogSoEditor, Error, TEXT("Failed To save all Characther Strikes. An error occurred."));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::ExtendMenu()
{
	// Running in game mode (standalone game) exit as we can't get the LevelEditorModule.
	if (IsRunningGame() || IsRunningCommandlet())
	{
		return;
	}

	// File -> Save all Item Templates
	{
		TSharedRef<FExtender> FileMenuExtender(new FExtender);

		// Fill after the File->FileLoadAndSave
		FileMenuExtender->AddMenuExtension(
			"FileLoadAndSave",
			EExtensionHook::After,
			FileMenuEditorCommands.ToSharedRef(),
			FMenuExtensionDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
			{
				// Save All
				MenuBuilder.BeginSection("WarriorbFileLoadAndSave", FText::FromString(TEXT("Warriorb")));
				{
					MenuBuilder.AddMenuEntry(FSoEditorCommands::Get().SaveAllItems);
					MenuBuilder.AddMenuEntry(FSoEditorCommands::Get().SaveAllEffectInstances);
					MenuBuilder.AddMenuEntry(FSoEditorCommands::Get().SaveAllCharacterStrikes);
				}
				MenuBuilder.EndSection();
			}));

		// Add to the level editor
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(NAME_MODULE_LevelEditor);
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(FileMenuExtender);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::SaveCameraData()
{
	UWorld* World = FSoEditorUtilities::GetEditorWorld();
	if (!World)
		return;

	const bool bSuccess = FSoEditorGameInterface::SaveCameraData(World);
	if (!bSuccess)
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Save failed!!!"), TEXT("Fail"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::LoadCameraData()
{
	UWorld* World = FSoEditorUtilities::GetEditorWorld();
	if (!World)
		return;

	const bool bSuccess = FSoEditorGameInterface::LoadCameraData(World);
	if (!bSuccess)
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Load failed!!!"), TEXT("Fail"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::ToggleSplinePositionCorrection()
{
	bIsSplinePositionCorrectionEnabled = FSoEditorGameInterface::ToggleSplinePosCorrection();
	if (bIsSplinePositionCorrectionEnabled)
	{
		UE_LOG(LogSoEditor, Warning, TEXT("Spline Location Correction based on world position is ENABLED!"));
		USoPlatformHelper::PrintToScreen(TEXT("Spline Location Correction based on world position is ENABLED!"), 5.f, FColor::Yellow);
	}
	else
	{
		UE_LOG(LogSoEditor, Error, TEXT("Spline Location Correction based on world position is DISABLED!"));
		USoPlatformHelper::PrintToScreen(TEXT("Spline Location Correction based on world position is DISABLED!"), 5.f, FColor::Yellow);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::ToggleDisplayCameraKeys()
{
	if (GEditor == nullptr)
		return;

	UWorld* World = FSoEditorUtilities::GetEditorWorld();
	if (World != nullptr)
	{
		ASoPlayerSpline::bHideCameraKeys = !ASoPlayerSpline::bHideCameraKeys;

		for (TActorIterator<ASoPlayerSpline> It(World, ASoPlayerSpline::StaticClass()); It; ++It)
		{
			if (*It != nullptr)
			{
				(*It)->UpdateCameraKeyData();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::IsDisplayCameraKeysEnabled()
{
	return !ASoPlayerSpline::bHideCameraKeys;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::RebuildEnemyGroups()
{
	UWorld* World = FSoEditorUtilities::GetEditorWorld();
	if (World == nullptr)
		return;

	FString ErrorMessage;

	// Make persistent level visible
	UEditorLevelUtils::SetLevelVisibility(World->PersistentLevel, true, false);

	// Make levels visible and hide the other
	if (!FSoEditorUtilities::MakeAllEditorLevelsVisible(ErrorMessage))
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *(FString("Failed to activate all levels: ") + ErrorMessage), TEXT("Fail"));
		return;
	}

	FSoEditorGameInterface::BuildEnemyGroupDataConfigFile(World);
	FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Enemy groups are rebuilt for the current chapter!"), TEXT("Task Done!"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::CopyActorsToClipboard()
{
	FNotYetUtilities::CopyEditorSelectedActorsToClipboard(ENotYetCopyActors::NTCA_PropertyEditor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SetChapterLevelsPresetTo(const int32 ChapterPresetIndex, const int32 LevelsPresetIndex)
{
	// Set chapter preset
	if (!SetChapterPresetTo(ChapterPresetIndex))
		return false;

	UWorld* World = FSoEditorUtilities::GetEditorWorld();
	if (World == nullptr)
		return false;

	// Set level preset
	const FSoEditorChapterBuildPreset& ChapterBuildPreset = BuildPresets.ChapterPresets[ChapterIndexBuildPreset];
	if (!ChapterBuildPreset.LevelPresets.IsValidIndex(LevelsPresetIndex))
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("LevelsBuildPreset = %d is not a valid index"), LevelsPresetIndex));
		return false;
	}
	ChapterLevelIndexBuildPreset = LevelsPresetIndex;
	const FSoEditorChapterBuildPresetLevels& LevelsBuildPreset = ChapterBuildPreset.LevelPresets[ChapterLevelIndexBuildPreset];
	UE_LOG(LogSoEditor, Log, TEXT("SetChapterLevelsPresetTo: CurrentChapterLevelBuildPreset = %d"), ChapterLevelIndexBuildPreset);

	// Make levels visible and hide the other
	FString ErrorMessage;
	if (!FSoEditorUtilities::MakeEditorLevelsVisible(LevelsBuildPreset.Levels, ErrorMessage))
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("Failed to make editor levels visible. ErrorMessage = `%s`"), *ErrorMessage));
		return false;
	}

	// Set sky Preset
	SetSkyPresetTo(LevelsBuildPreset.SkyPreset);
	LogCurrentBuildPreset();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SetChapterPresetTo(int32 ChapterPresetIndex)
{
	UE_LOG(LogSoEditor, Log, TEXT("SetChapterPresetTo = %d"), ChapterPresetIndex);

	// Get the chapter preset
	if (!BuildPresets.ChapterPresets.IsValidIndex(ChapterPresetIndex))
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("ChapterBuildPreset = %d is not a valid index"), ChapterPresetIndex));
		return false;
	}

	bIsUsingChapterPreset = true;
	ChapterIndexBuildPreset = ChapterPresetIndex;
	const FSoEditorChapterBuildPreset& ChapterBuildPreset = BuildPresets.ChapterPresets[ChapterIndexBuildPreset];
	const FString ChapterPackageName = ChapterBuildPreset.ChapterPackageName.ToString();
	const bool bReturnStatus = SetPersistentMapName(ChapterPackageName);
	if (!bReturnStatus)
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("ChapterBuildPreset = %d, Chapter = %s. Error happened"), ChapterPresetIndex, *ChapterPackageName));
	}
	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SetEpisodePresetTo(int32 EpisodePresetIndex)
{
	UE_LOG(LogSoEditor, Log, TEXT("SetEpisodePresetTo = %d"), EpisodePresetIndex);

	// Get the chapter preset
	if (!BuildPresets.EpisodePresets.IsValidIndex(EpisodePresetIndex))
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("EpisodeBuildPreset = %d is not a valid index"), EpisodePresetIndex));
		return false;
	}

	bIsUsingChapterPreset = false;
	EpisodeIndexBuildPreset = EpisodePresetIndex;
	const FSoEditorEpisodeBuildPreset& EpisodeBuildPreset = BuildPresets.EpisodePresets[EpisodeIndexBuildPreset];
	const FString EpisodePackageName = EpisodeBuildPreset.EpisodePackageName.ToString();

	const bool bReturnStatus = SetPersistentMapName(EpisodePackageName);
	if (!bReturnStatus)
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("EpisodeBuildPreset = %d, Episode = %s. Error happened"), EpisodePresetIndex, *EpisodePackageName));
	}

	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SetPersistentMapName(const FString& MapPackageName)
{
	FString ErrorMessage;
	FText ErrorTextMessage;
	if (!FPackageName::IsValidLongPackageName(MapPackageName, false, &ErrorTextMessage))
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("MapPackageName = %s. Error = %s"), *MapPackageName, *ErrorTextMessage.ToString()));
		return false;
	}

	UWorld* World = FSoEditorUtilities::GetEditorWorld();
	if (World == nullptr || World->PersistentLevel == nullptr)
	{
		FSoEditorUtilities::AddNotificationError(TEXT("World or World->PersistentLevel is null"));
		return false;
	}

	// Load new chapter
	const FString CurrentMapPackageName = FSoEditorUtilities::GetLongPackageNameFromObject(World->PersistentLevel);
	if (CurrentMapPackageName != MapPackageName)
	{
		// Load New map
		if (!FEditorFileUtils::LoadMap(MapPackageName, false, true))
		{
			FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("Can't load MapPackageName = %s"), *MapPackageName));
			return false;
		}

		// NOTE: Must load the new world as the map changed
		World = FSoEditorUtilities::GetEditorWorld();
	}

	if (World == nullptr || World->PersistentLevel == nullptr)
	{
		FSoEditorUtilities::AddNotificationError(TEXT("World or World->PersistentLevel is null"));
		return false;
	}

	// Make chapter visible
	UEditorLevelUtils::SetLevelVisibility(World->PersistentLevel, true, false);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::SetSkyPresetTo(FName SkyName)
{
	ASoSky* Sky = nullptr;
	FString ErrorMessage;
	if (!FSoEditorUtilities::GetEditorSoSky(Sky, ErrorMessage))
	{
		FSoEditorUtilities::AddNotificationError(FString::Printf(TEXT("Failed set SoSky Preset to `%s`. ErrorMessage = `%s`"), *SkyName.ToString(), *ErrorMessage));
		return false;
	}

	Sky->SetPreviewedPreset0(SkyName);
	Sky->SetBlendValue(0.f);
	Sky->UpdatePreviewedSky();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::LogCurrentBuildPreset()
{
	if (bIsUsingChapterPreset)
	{
		if (!BuildPresets.ChapterPresets.IsValidIndex(ChapterIndexBuildPreset))
			return;

		// Log
		const FSoEditorChapterBuildPreset& ChapterPreset = BuildPresets.ChapterPresets[ChapterIndexBuildPreset];
		if (!ChapterPreset.LevelPresets.IsValidIndex(ChapterLevelIndexBuildPreset))
			return;
		const FSoEditorChapterBuildPresetLevels& LevelsPreset = ChapterPreset.LevelPresets[ChapterLevelIndexBuildPreset];

		UE_LOG(LogSoEditor, Log, TEXT(""));
		UE_LOG(LogSoEditor, Log, TEXT("CurrentChapterBuildPreset = %d, CurrentLevelsBuildPreset = %d"), ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset);
		UE_LOG(LogSoEditor, Log, TEXT("\tChapterPackageName = %s"), *ChapterPreset.ChapterPackageName.ToString());
		UE_LOG(LogSoEditor, Log, TEXT("\tSkyPreset = %s"), *LevelsPreset.SkyPreset.ToString());
		UE_LOG(LogSoEditor, Log, TEXT("\tLevels:"), *LevelsPreset.SkyPreset.ToString());
		for (FName Level : LevelsPreset.Levels)
			UE_LOG(LogSoEditor, Log, TEXT("\t\t%s"), *Level.ToString());
		UE_LOG(LogSoEditor, Log, TEXT(""));
	}
	else
	{
		if (!BuildPresets.EpisodePresets.IsValidIndex(EpisodeIndexBuildPreset))
			return;

		const FSoEditorEpisodeBuildPreset& EpisodePreset = BuildPresets.EpisodePresets[EpisodeIndexBuildPreset];
		UE_LOG(LogSoEditor, Log, TEXT(""));
		UE_LOG(LogSoEditor, Log, TEXT("CurrentEpisodeBuildPreset = %d"), EpisodeIndexBuildPreset);
		UE_LOG(LogSoEditor, Log, TEXT("\tEpisodePackageName = %s"), *EpisodePreset.EpisodePackageName.ToString());
		UE_LOG(LogSoEditor, Log, TEXT(""));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::LoadBuildPresets()
{
	FDlgJsonParser Parser(FPaths::ProjectContentDir() / TEXT("SO/Data/LightBuildPresets.json"));
	if (Parser.IsValidFile())
		Parser.ReadAllProperty(FSoEditorBuildPresets::StaticStruct(), &BuildPresets, nullptr);

	ChapterLevelIndexBuildPreset = 0;
	ChapterIndexBuildPreset = 0;
	EpisodeIndexBuildPreset = 0;
	UE_LOG(LogSoEditor, Log, TEXT("LoadBuildPresets: BuildPresets = \n%s"), *BuildPresets.ToString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::AddNotificationBar(FToolBarBuilder& ToolBarBuilder)
{
	const FSlateFontInfo& SmallFixedFont = FEditorStyle::GetFontStyle(TEXT("MainFrame.DebugTools.SmallFont"));
	const FSlateFontInfo& NormalFixedFont = FEditorStyle::GetFontStyle(TEXT("MainFrame.DebugTools.NormalFont"));
	const FSlateFontInfo& LabelFont = FEditorStyle::GetFontStyle(TEXT("MainFrame.DebugTools.LabelFont"));

	TSharedRef<SWidget> BodyWidget =
		SNew(SHorizontalBox)

		// BuildType
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("BuildType: ")))
				.Font(LabelFont)
				.ColorAndOpacity(FLinearColor( 0.3f, 0.3f, 0.3f))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(WidgetTextBuildType, STextBlock)
				.Font(NormalFixedFont)
				.ColorAndOpacity(FLinearColor( 0.6f, 0.6f, 0.6f))
			]
		]

		// BuildInfo
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("BuildInfo: ")))
				.Font(LabelFont)
				.ColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(WidgetTextBuildInfo, STextBlock)
				.Font(NormalFixedFont)
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
			]
		]

		// Map
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Map: ")))
				.Font(LabelFont)
				.ColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Bottom)
			[
				SAssignNew(WidgetTextMap, STextBlock)
				.Font(NormalFixedFont)
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
			]
		]
	;

	ToolBarBuilder.AddWidget(
		SNew(SBorder)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 1.0f))
		.VAlign(VAlign_Bottom)
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		.Visibility_Lambda([this]() -> EVisibility
		{
			return bShowNotificationBar ? EVisibility::Visible : EVisibility::Collapsed;
			//return EVisibility::Visible;
		})
		[
			SNew(SHorizontalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f)
			[
				BodyWidget
			]
		]
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedRef<SWidget> FSOrbEditorModule::GenerateBuildPresetsMenu()
{
	constexpr bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr, {});

	//const FString TempTExt = TEXT("Save All dirty packages");
	//MenuBuilder.AddMenuEntry(
	//	FText::FromString(TempTExt),
	//	FText::FromString(TempTExt),
	//	FSlateIcon(),
	//	FUIAction(FExecuteAction::CreateLambda([this]
	//	{
	//		FSoEditorUtilities::SaveAllDirtyPackages();
	//	}))
	//);
	//MenuBuilder.AddMenuSeparator();

	MenuBuilder.BeginSection("Build", FText::FromString(TEXT("Build")));
	{
		const FString BuildAllText = TEXT("Build All");
		MenuBuilder.AddMenuEntry(
			FText::FromString(BuildAllText),
			FText::FromString(BuildAllText),
			FSlateIcon(FSoEditorStyle::GetStyleSetName(), FSoEditorStyle::PROPERTY_BuildIcon),
			FUIAction(
				FExecuteAction::CreateLambda([this]
				{
					BuildAllType = ESoBuildAllType::All;
					StartBuildAll();
				}),
				FCanExecuteAction::CreateRaw(this, &Self::CanStartLightBuild)
			)
		);
	}
	{
		const FString BuildAllChaptersText = TEXT("Build All Chapters");
		MenuBuilder.AddMenuEntry(
			FText::FromString(BuildAllChaptersText),
			FText::FromString(BuildAllChaptersText),
			FSlateIcon(FSoEditorStyle::GetStyleSetName(), FSoEditorStyle::PROPERTY_BuildIcon),
			FUIAction(
				FExecuteAction::CreateLambda([this]
				{
					BuildAllType = ESoBuildAllType::AllChapters;
					StartBuildAllChapters(true);
				}),
				FCanExecuteAction::CreateRaw(this, &Self::CanStartLightBuild)
			)
		);
	}
	{
		const FString BuildAllEpisodesText = TEXT("Build All Episode");
		MenuBuilder.AddMenuEntry(
			FText::FromString(BuildAllEpisodesText),
			FText::FromString(BuildAllEpisodesText),
			FSlateIcon(FSoEditorStyle::GetStyleSetName(), FSoEditorStyle::PROPERTY_BuildIcon),
			FUIAction(
				FExecuteAction::CreateLambda([this]
				{
					BuildAllType = ESoBuildAllType::AllEpisodes;
					StartBuildAllEpisodes(true);
				}),
				FCanExecuteAction::CreateRaw(this, &Self::CanStartLightBuild)
			)
		);
	}
	MenuBuilder.EndSection();


	MenuBuilder.BeginSection("Chapter Presets", FText::FromString(TEXT("Chapter Presets")));
	for (int32 ChapterIndex = 0; ChapterIndex < BuildPresets.ChapterPresets.Num(); ChapterIndex++)
	{
		const FSoEditorChapterBuildPreset& ChapterPreset = BuildPresets.ChapterPresets[ChapterIndex];
		const FString MapShortName = FPackageName::GetShortName(ChapterPreset.ChapterPackageName);
		const FString ChapterName = FString::Printf(TEXT("%s"), *MapShortName);

		// Create sub menu for each chapter
		MenuBuilder.AddSubMenu(
			FText::FromString(ChapterName),
			FText::FromString(ChapterName),
			FNewMenuDelegate::CreateLambda([&ChapterPreset, this, ChapterIndex](FMenuBuilder& SubMenuBuilder)
			{
				// Build all levels in chapter
				SubMenuBuilder.BeginSection("Build", FText::FromString(TEXT("Build")));
				const FString BuildAllText = TEXT("Build All");
				SubMenuBuilder.AddMenuEntry(
					FText::FromString(BuildAllText),
					FText::FromString(BuildAllText),
					FSlateIcon(FSoEditorStyle::GetStyleSetName(), FSoEditorStyle::PROPERTY_BuildIcon),
					FUIAction(
						FExecuteAction::CreateLambda([this, ChapterIndex]
						{
							BuildAllType = ESoBuildAllType::OnlyLevelsInChapter;
							ChapterIndexToBuild = ChapterIndex;
							StartBuildAllChapters(true);
						}),
						FCanExecuteAction::CreateRaw(this, &Self::CanStartLightBuild)
					)
				);
				SubMenuBuilder.EndSection();

				// All levels with sky default
				SubMenuBuilder.BeginSection("Show", FText::FromString(TEXT("Show")));
				static const FName DefaultPresetForAll = TEXT("Clouds");
				const FString AllText = FString::Printf(TEXT("All (Sky = %s)"), *DefaultPresetForAll.ToString());
				SubMenuBuilder.AddMenuEntry(
					FText::FromString(AllText),
					FText::FromString(AllText),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this, ChapterIndex]
						{
							if (SetChapterPresetTo(ChapterIndex))
							{
								FString ErrorMessage;
								FSoEditorUtilities::MakeAllEditorLevelsVisible(ErrorMessage);
								SetSkyPresetTo(DefaultPresetForAll);
							}
						}),
						FCanExecuteAction::CreateRaw(this, &Self::CanStartLightBuild)
					)
				);

				// Create menu for levels in the chapter (act)
				for (int32 LevelsPresetIndex = 0; LevelsPresetIndex < ChapterPreset.LevelPresets.Num(); LevelsPresetIndex++)
				{
					const FSoEditorChapterBuildPresetLevels& LevelsPreset = ChapterPreset.LevelPresets[LevelsPresetIndex];

					// Accumulate levels
					FString LevelsString = LevelsPreset.Levels.Num() > 0 ? LevelsPreset.Levels[0].ToString() : TEXT("No levels");
					for (int32 LevelIndex = 1; LevelIndex < LevelsPreset.Levels.Num(); LevelIndex++)
						LevelsString += TEXT(" + ") + LevelsPreset.Levels[LevelIndex].ToString();

					// Set build presets
					const FString Text = FString::Printf(TEXT("%d. %s (Sky = %s)"), LevelsPresetIndex, *LevelsString, *LevelsPreset.SkyPreset.ToString());
					SubMenuBuilder.AddMenuEntry(
						FText::FromString(Text),
						FText::FromString(Text),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda([this, ChapterIndex, LevelsPresetIndex]
							{
								SetChapterLevelsPresetTo(ChapterIndex, LevelsPresetIndex);
							}),
							FCanExecuteAction::CreateRaw(this, &Self::CanStartLightBuild)
						)
					);
				}
				SubMenuBuilder.EndSection();
			}),
			false,
			FSlateIcon()
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Episode Presets", FText::FromString(TEXT("Episode Presets")));
	const FText EpisodeText = FText::FromString(TEXT("Episodes"));
	MenuBuilder.AddSubMenu(
		EpisodeText,
		EpisodeText,
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& SubMenuBuilder)
		{
			SubMenuBuilder.BeginSection("Show", FText::FromString(TEXT("Show")));
			for (int32 Index = 0; Index < BuildPresets.EpisodePresets.Num(); Index++)
			{
				const FSoEditorEpisodeBuildPreset& EpisodePreset = BuildPresets.EpisodePresets[Index];
				const FString MapShortName = FPackageName::GetShortName(EpisodePreset.EpisodePackageName);
				const FString EpisodeName = FString::Printf(TEXT("%s"), *MapShortName);

				const FString Text = FString::Printf(TEXT("%d. %s"), Index, *EpisodeName);

				// Create sub menu for each episode
				SubMenuBuilder.AddMenuEntry(
					FText::FromString(Text),
					FText::FromString(Text),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateLambda([this, Index]
						{
							SetEpisodePresetTo(Index);
						}),
						FCanExecuteAction::CreateRaw(this, &Self::CanStartLightBuild)
					)
				);
			}
			SubMenuBuilder.EndSection();
		}),
		false,
		FSlateIcon()
	);
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::UpdateNotificationWidgets()
{
	FString MapName;
	FString BuildType;
	FString BuildInfo;
	if (bIsUsingChapterPreset)
	{
		// Chapter
		BuildType = TEXT("Chapter");

		if (BuildPresets.ChapterPresets.IsValidIndex(ChapterIndexBuildPreset))
		{
			const FSoEditorChapterBuildPreset& ChapterBuildPreset = BuildPresets.ChapterPresets[ChapterIndexBuildPreset];
			MapName = ChapterBuildPreset.ChapterPackageName.ToString();

			if (ChapterBuildPreset.LevelPresets.IsValidIndex(ChapterLevelIndexBuildPreset))
			{
				const FSoEditorChapterBuildPresetLevels& LevelBuildPreset = ChapterBuildPreset.LevelPresets[ChapterLevelIndexBuildPreset];
				BuildInfo = FString::Printf(TEXT("ChapterIndex = %d, LevelsPresetIndex = %d, %s"), ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset, *LevelBuildPreset.ToString());
			}
		}
		else
		{
			MapName = TEXT("Invalid CurrentChapterBuildPreset");
		}
	}
	else
	{
		// Episode
		BuildType = TEXT("Episode");

		if (BuildPresets.EpisodePresets.IsValidIndex(EpisodeIndexBuildPreset))
		{
			const FSoEditorEpisodeBuildPreset& EpisodePreset = BuildPresets.EpisodePresets[EpisodeIndexBuildPreset];
			MapName = EpisodePreset.EpisodePackageName.ToString();

			BuildInfo = FString::Printf(TEXT("EpisodeIndex = %d"), EpisodeIndexBuildPreset);
		}
		else
		{
			MapName = TEXT("Invalid CurrentEpisodeBuildPreset");
		}
	}

	WidgetTextMap->SetText(MapName);
	WidgetTextBuildType->SetText(BuildType);
	WidgetTextBuildInfo->SetText(BuildInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnModuleChanged(FName ModuleName, EModuleChangeReason ChangeReason)
{
	// UE_LOG(LogTemp, Warning, TEXT("ModuleLoaded %s"), *ModuleName.ToString());
	// random module which loads late
	if (ModuleName != FName("EditorWidgets"))
		return;

	UWorld* EditorWorld = FSoEditorUtilities::GetEditorWorld();
	if (EditorWorld == nullptr)
		return;

	// random module which loads late
	if (ChangeReason == EModuleChangeReason::ModuleLoaded)
	{
		auto& Settings = USoGameSettings::Get();
		Settings.LoadSettings();
		if (Settings.IsDisplayedStatsEnabled())
			GEditor->Exec(EditorWorld, TEXT("stat fps"), *GLog);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::ConfigureLightingBuildOptions(const FLightingBuildOptions& Options)
{
	GConfig->SetBool(TEXT("LightingBuildOptions"), TEXT("OnlyBuildSelected"), Options.bOnlyBuildSelected, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("LightingBuildOptions"), TEXT("OnlyBuildCurrentLevel"), Options.bOnlyBuildCurrentLevel, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("LightingBuildOptions"), TEXT("OnlyBuildSelectedLevels"), Options.bOnlyBuildSelectedLevels, GEditorPerProjectIni);
	GConfig->SetBool(TEXT("LightingBuildOptions"), TEXT("OnlyBuildVisibility"), Options.bOnlyBuildVisibility, GEditorPerProjectIni);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::IsTickable() const
{
	return IsValid(GWorld) &&
		GWorld->bIsWorldInitialized &&
		GWorld->WorldType != EWorldType::Inactive &&
		CanStartLightBuild() &&
		(LightStatus == ESoLightBuildStatus::Succeeded || LightStatus == ESoLightBuildStatus::Succeeded_Wait || LightStatus == ESoLightBuildStatus::Loading_WaitLevelChange) &&
		!FSoEditorUtilities::IsAnyLevelInWorldLoading(GWorld);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::Tick(float DeltaTime)
{
	if (!IsTickable())
		return true;

	if (LightStatus == ESoLightBuildStatus::Loading_WaitLevelChange)
	{
		RunEditorBuildAll();
	}

	// Add some delay
	else if (LightStatus == ESoLightBuildStatus::Succeeded_Wait)
	{
		AfterLightCurrentSeconds += DeltaTime;
		if (AfterLightCurrentSeconds > AfterLightWaitSeconds)
		{
			AfterLightCurrentSeconds = 0.f;
			LightStatus = ESoLightBuildStatus::Succeeded;
		}
	}
	else if (LightStatus == ESoLightBuildStatus::Succeeded)
	{
		FinishedBuildNext();
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::RunEditorBuildAll()
{
	UE_LOG(LogSoEditor, Log, TEXT("RunEditorBuildAll"));
	LightStatus = ESoLightBuildStatus::Building;
	//CurrentWorld = FSoEditorUtilities::GetEditorWorld();

	// Reset build options
	ConfigureLightingBuildOptions(FLightingBuildOptions());

	FSoEditorUtilities::EditorBuildAll(GWorld);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::ConfirmUserBuildQuestion(const FString& Body)
{
	// User confirmation
	const EAppReturnType::Type Response = FPlatformMisc::MessageBoxExt(EAppMsgType::YesNo, *Body, TEXT("Continue building?"));
	return Response == EAppReturnType::Yes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::PreStartBuild()
{
	// Start building
	//CurrentWorld = FSoEditorUtilities::GetEditorWorld();
	SubscribeToLightBuildEvents();
	//LoadBuildPresets();

	// Force AutoApplyLighting on
	ULevelEditorMiscSettings* LevelEdSettings = GetMutableDefault<ULevelEditorMiscSettings>();
	bOldAutoApplyLightingEnable = LevelEdSettings->bAutoApplyLightingEnable;
	LevelEdSettings->bAutoApplyLightingEnable = true;

	// Disable autosaving
	UEditorLoadingSavingSettings* LoadingSavingSettings = GetMutableDefault<UEditorLoadingSavingSettings>();
	bOldAutoSaveEnable = LoadingSavingSettings->bAutoSaveEnable;
	LoadingSavingSettings->bAutoSaveEnable = false;

	UE_LOG(LogSoEditor, Log, TEXT("PreStartBuild: Saving"));
	bShowNotificationBar = true;
	FSoEditorUtilities::SaveAllDirtyPackages();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::StartBuildAllEpisodes(bool bAskConfirmation)
{
	UE_LOG(LogSoEditor, Log, TEXT("StartBuildAllEpisodes"));

	if (bAskConfirmation)
	{
		if (!ConfirmUserBuildQuestion(FString::Printf(TEXT("Build All Episodes for Warriorb\n%s"), *BuildPresets.ToStringEpisodes())))
			return;
	}

	PreStartBuild();
	bIsUsingChapterPreset = false;
	EpisodeIndexBuildPreset = 0;
	BuildCurrentEpisode();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::BuildCurrentEpisode()
{
	UE_LOG(LogSoEditor, Log, TEXT("BuildCurrentEpisode"));
	verify(!bIsUsingChapterPreset);
	verify(!GUnrealEd->WarnIfLightingBuildIsCurrentlyRunning());

	if (!BuildPresets.EpisodePresets.IsValidIndex(EpisodeIndexBuildPreset))
	{
		UE_LOG(LogSoEditor, Error, TEXT("CurrentEpisodeBuildPreset = %d is not a valid index in EpisodePresets :O"), EpisodeIndexBuildPreset);
		return;
	}

	// Save here because we are changing map most likely
	FSoEditorUtilities::SaveAllDirtyPackages();
	LightStatus = ESoLightBuildStatus::Loading_WaitLevelChange;
	SetEpisodePresetTo(EpisodeIndexBuildPreset);
	UpdateNotificationWidgets();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::FinishedBuildNextEpisode()
{
	UE_LOG(LogSoEditor, Log, TEXT("FinishedBuildNextEpisode"));
	verify(!bIsUsingChapterPreset);
	verify(!GUnrealEd->WarnIfLightingBuildIsCurrentlyRunning());

	if (BuildPresets.EpisodePresets.IsValidIndex(EpisodeIndexBuildPreset + 1))
	{
		// Build next
		EpisodeIndexBuildPreset++;
		BuildCurrentEpisode();
	}
	else
	{
		OnFinishedBuildingEpisodes();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::StartBuildAllChapters(bool bAskConfirmation)
{
	UE_LOG(LogSoEditor, Log, TEXT("StartBuildAllChapters"));

	if (bAskConfirmation)
	{
		FString ChaptersString;
		FString QuestionPlurals = TEXT("All chapters");
		if (BuildAllType == ESoBuildAllType::OnlyLevelsInChapter)
		{
			QuestionPlurals = TEXT("All levels of Chapter");
			ChaptersString = BuildPresets.ChapterPresets.IsValidIndex(ChapterIndexToBuild) ? BuildPresets.ChapterPresets[ChapterIndexToBuild].ToString() : TEXT("ERROR");
		}
		else
		{
			ChaptersString = BuildPresets.ToStringChapters();
		}

		if (!ConfirmUserBuildQuestion(FString::Printf(TEXT("Build %s for Warriorb\n%s"), *QuestionPlurals, *ChaptersString)))
			return;
	}

	PreStartBuild();
	bIsUsingChapterPreset = true;
	ChapterIndexBuildPreset = 0;
	ChapterLevelIndexBuildPreset = 0;
	BuildCurrentChapter();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::BuildCurrentChapter()
{
	UE_LOG(LogSoEditor, Log, TEXT("BuildCurrentChapter: CurrentChapterBuildPreset = %d,  CurrentChapterLevelBuildPreset = %d"), ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset);
	verify(bIsUsingChapterPreset);
	verify(!GUnrealEd->WarnIfLightingBuildIsCurrentlyRunning());

	// Build only this chapter
	if (BuildAllType == ESoBuildAllType::OnlyLevelsInChapter)
	{
		UE_LOG(LogSoEditor, Log, TEXT("BuildCurrentChapter: Only building ChapterIndex = %d"), ChapterIndexToBuild);
		ChapterIndexBuildPreset = ChapterIndexToBuild;
	}

	const FString PreviousMapName = GWorld->GetMapName();
	if (!BuildPresets.ChapterPresets.IsValidIndex(ChapterIndexBuildPreset))
	{
		UE_LOG(LogSoEditor, Error, TEXT("CurrentChapterBuildPreset = %d is not a valid index in ChapterPresets :O"), ChapterIndexBuildPreset);
		return;
	}

	const FSoEditorChapterBuildPreset& ChapterPreset = BuildPresets.ChapterPresets[ChapterIndexBuildPreset];
	if (!ChapterPreset.LevelPresets.IsValidIndex(ChapterLevelIndexBuildPreset))
	{
		UE_LOG(LogSoEditor, Error, TEXT("CurrentChapterLevelBuildPreset = %d is not a valid index in ChapterPresets[%d].LevelPresets :O"), ChapterLevelIndexBuildPreset, ChapterIndexBuildPreset);
		return;
	}

	// Can we save dirty packages?
	const FSoEditorChapterBuildPresetLevels& LevelPreset = ChapterPreset.LevelPresets[ChapterLevelIndexBuildPreset];
	if (LevelPreset.bSaveAllDirtyPackagesBefore)
	{
		UE_LOG(LogSoEditor, Log, TEXT("BuildCurrentChapter: Saving all dirty packages because bSaveAllDirtyPackagesBefore = true"));
		FSoEditorUtilities::SaveAllDirtyPackages();
	}
	if (PreviousChapterIndexBuildPreset != ChapterIndexBuildPreset)
	{
		UE_LOG(LogSoEditor, Log, TEXT("BuildCurrentChapter: Saving all dirty packages because we are about to change chapters"));
		FSoEditorUtilities::SaveAllDirtyPackages();
	}

	LightStatus = ESoLightBuildStatus::Loading_WaitLevelChange;
	SetChapterLevelsPresetTo(ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset);
	UpdateNotificationWidgets();
	PreviousChapterIndexBuildPreset = ChapterIndexBuildPreset;

	//CurrentWorld = FSoEditorUtilities::GetEditorWorld();
	//const FString CurrentMapName = FSoEditorUtilities::GetEditorWorld()->GetMapName();
	//if (CurrentMapName != PreviousMapName)
	//{
	//	CurrentWorld = nullptr;
	//}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::FinishedBuildNextChapter()
{
	UE_LOG(LogSoEditor, Log, TEXT("FinishedBuildNextChapter"));
	verify(bIsUsingChapterPreset);

	auto AdvanceLevelIndex = [this]() -> bool
	{
		const FSoEditorChapterBuildPreset& ChapterPreset = BuildPresets.ChapterPresets[ChapterIndexBuildPreset];
		const bool bCanAdvance = ChapterPreset.LevelPresets.IsValidIndex(ChapterLevelIndexBuildPreset + 1);
		if (bCanAdvance)
			ChapterLevelIndexBuildPreset++;

		return bCanAdvance;
	};

	auto AdvanceChapterIndex = [this]() -> bool
	{
		const bool bCanAdvance = BuildPresets.ChapterPresets.IsValidIndex(ChapterIndexBuildPreset + 1);
		if (bCanAdvance)
			ChapterIndexBuildPreset++;

		return bCanAdvance;
	};

	// Try advancing level
	const bool bDidAdvanceLevel = AdvanceLevelIndex();

	// Try advancing chapter
	bool bDidAdvanceChapter = false;
	const bool bCanAdvanceChapter = BuildAllType == ESoBuildAllType::AllChapters || BuildAllType == ESoBuildAllType::All;
	if (!bDidAdvanceLevel && bCanAdvanceChapter)
	{
		bDidAdvanceChapter = AdvanceChapterIndex();
	}

	if (bDidAdvanceLevel || bDidAdvanceChapter)
	{
		// Build next
		BuildCurrentChapter();
	}
	else
	{
		// Can't do anything quit
		OnFinishedBuildingChapters();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::StartBuildAll()
{
	UE_LOG(LogSoEditor, Log, TEXT("StartBuildAll"));

	// User confirmation
	const FString BuildPresetsStr = BuildPresets.ToString();
	if (!ConfirmUserBuildQuestion(FString::Printf(TEXT("You are about to start building all levels for Warriorb with the predefined presets\n%s"), *BuildPresetsStr)))
		return;

	// First Episodes then chapters
	StartBuildAllEpisodes(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSOrbEditorModule::CanStartLightBuild() const
{
	return GUnrealEd && !GUnrealEd->PlayWorld && !GUnrealEd->bIsSimulatingInEditor && !GUnrealEd->IsLightingBuildCurrentlyRunning()
		&& !GUnrealEd->IsLightingBuildCurrentlyExporting();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::FinishedBuildNext()
{
	UE_LOG(LogSoEditor,
		Log,
		TEXT("FinishedBuildNext: bIsUsingChapterPreset = %d, CurrentChapterBuildPreset = %d, CurrentChapterLevelBuildPreset = %d, CurrentEpisodeBuildPreset = %d"),
		bIsUsingChapterPreset, ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset, EpisodeIndexBuildPreset);

	if (bIsUsingChapterPreset)
	{
		FinishedBuildNextChapter();
	}
	else
	{
		FinishedBuildNextEpisode();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnFinishedBuildingEpisodes()
{
	// return back to chapters
	//bIsUsingChapterPreset = true;
	//CurrentChapterBuildPreset = 0;
	//CurrentChapterLevelBuildPreset = 0;
	//BuildCurrentChapter();

	if (BuildAllType == ESoBuildAllType::AllEpisodes)
	{
		UE_LOG(LogSoEditor, Log, TEXT("OnFinishedBuildingEpisodes. Done building. Saving all"));
		FSoEditorUtilities::AddNotificationError(TEXT("All Light builds SUCCEEDED"));
		ResetBuildSettings();
		FSoEditorUtilities::SaveAllDirtyPackages();
	}
	else if (BuildAllType == ESoBuildAllType::All)
	{
		// Next build chapters
		UE_LOG(LogSoEditor, Log, TEXT("OnFinishedBuildingEpisodes. Done building episodes. Moving to chapters. Saving all"));
		FSoEditorUtilities::SaveAllDirtyPackages();
		StartBuildAllChapters(false);
	}
	else
	{
		checkNoEntry();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnFinishedBuildingChapters()
{
	// Reset, everything is done
	auto Finished = [this]()
	{
		FSoEditorUtilities::AddNotificationError(TEXT("All Light builds SUCCEEDED"));
		ResetBuildSettings();
		FSoEditorUtilities::SaveAllDirtyPackages();
	};

	if (BuildAllType == ESoBuildAllType::OnlyLevelsInChapter)
	{
		UE_LOG(LogSoEditor, Log, TEXT("OnFinishedBuildingChapters. Done building all levels of ChapterIndex = %d. Saving all"), ChapterIndexBuildPreset);
		Finished();
	}
	else if (BuildAllType == ESoBuildAllType::AllChapters)
	{
		UE_LOG(LogSoEditor, Log, TEXT("OnFinishedBuildingChapters. Done building all chapters. Saving all"));
		Finished();
	}
	else if (BuildAllType == ESoBuildAllType::All)
	{
		UE_LOG(LogSoEditor, Log, TEXT("OnFinishedBuildingChapters. Done building all episodes + chapters. Saving all"));
		Finished();
	}
	else
	{
		checkNoEntry();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnMapOpened(const FString&  Filename, bool bAsTemplate)
{
	UE_LOG(LogSoEditor, Log, TEXT("Opened Map = %s"), *Filename);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnLightBuildStarted()
{
	if (bIsUsingChapterPreset)
	{
		UE_LOG(LogSoEditor, Warning, TEXT("Light build started for ChapterBuildPreset = %d, ChapterLevelBuildPreset = %d"), ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset);
	}
	else
	{
		UE_LOG(LogSoEditor, Warning, TEXT("Light build started for EpisodeBuildPreset = %d"), EpisodeIndexBuildPreset);
	}

	LightStatus = ESoLightBuildStatus::Building;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnLightBuildFailed()
{
	if (bIsUsingChapterPreset)
	{
		UE_LOG(LogSoEditor, Error, TEXT("Light build FAILED for ChapterBuildPreset = %d, ChapterLevelBuildPreset = %d"), ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset);
	}
	else
	{
		UE_LOG(LogSoEditor, Error, TEXT("Light build FAILED for EpisodeBuildPreset = %d"), EpisodeIndexBuildPreset);
	}

	LightStatus = ESoLightBuildStatus::Failed;
	ResetBuildSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnLightBuildSucceeded()
{
	if (bIsUsingChapterPreset)
	{
		UE_LOG(LogSoEditor, Log, TEXT("Light build SUCCEEDED for ChapterBuildPreset = %d, ChapterLevelBuildPreset = %d"), ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset);
	}
	else
	{
		UE_LOG(LogSoEditor, Log, TEXT("Light build SUCCEEDED for EpisodeBuildPreset = %d"), EpisodeIndexBuildPreset);
	}

	// NOTE, the light is still processing, Succeed set in OnLightingBuildKept
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::OnLightingBuildKept()
{
	if (bIsUsingChapterPreset)
	{
		UE_LOG(LogSoEditor, Log, TEXT("Light build KEPT for ChapterBuildPreset = %d, ChapterLevelBuildPreset = %d"), ChapterIndexBuildPreset, ChapterLevelIndexBuildPreset);
	}
	else
	{
		UE_LOG(LogSoEditor, Log, TEXT("Light build KEPT for EpisodeBuildPreset = %d"), EpisodeIndexBuildPreset);
	}
	LightStatus = ESoLightBuildStatus::Succeeded_Wait;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::SubscribeToLightBuildEvents()
{
	if (bSubscribedToLightEvents)
		return;

	FWorldDelegates::OnPostWorldCreation.AddLambda([this](UWorld* World)
	{
		UE_LOG(LogSoEditor, Log, TEXT("OnPostWorldCreation = %s"), *World->GetMapName());
	});

	FEditorDelegates::OnMapOpened.AddRaw(this, &Self::OnMapOpened);
	FEditorDelegates::OnLightingBuildStarted.AddRaw(this, &Self::OnLightBuildStarted);
	FEditorDelegates::OnLightingBuildKept.AddRaw(this, &Self::OnLightingBuildKept);
	FEditorDelegates::OnLightingBuildFailed.AddRaw(this, &Self::OnLightBuildFailed);
	FEditorDelegates::OnLightingBuildSucceeded.AddRaw(this, &Self::OnLightBuildSucceeded);
	bSubscribedToLightEvents = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::UnsubscribeFromLightBuildEvents()
{
	if (!bSubscribedToLightEvents)
		return;

	FEditorDelegates::OnMapOpened.RemoveAll(this);
	FEditorDelegates::OnLightingBuildStarted.RemoveAll(this);
	FEditorDelegates::OnLightingBuildKept.RemoveAll(this);
	FEditorDelegates::OnLightingBuildFailed.RemoveAll(this);
	FEditorDelegates::OnLightingBuildSucceeded.RemoveAll(this);
	bSubscribedToLightEvents = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSOrbEditorModule::ResetBuildSettings()
{
	bShowNotificationBar = false;
	LightStatus = ESoLightBuildStatus::None;
	BuildAllType = ESoBuildAllType::All;
	ChapterIndexBuildPreset = INDEX_NONE;
	ChapterLevelIndexBuildPreset = INDEX_NONE;
	EpisodeIndexBuildPreset = INDEX_NONE;
	ChapterIndexToBuild = INDEX_NONE;
	bIsUsingChapterPreset = true;
	UnsubscribeFromLightBuildEvents();

	// Restore
	ULevelEditorMiscSettings* LevelEdSettings = GetMutableDefault<ULevelEditorMiscSettings>();
	LevelEdSettings->bAutoApplyLightingEnable = bOldAutoApplyLightingEnable;

	UEditorLoadingSavingSettings* LoadingSavingSettings = GetMutableDefault<UEditorLoadingSavingSettings>();
	LoadingSavingSettings->bAutoSaveEnable = bOldAutoSaveEnable;
}
