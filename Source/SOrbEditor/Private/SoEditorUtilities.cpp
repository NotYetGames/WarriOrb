// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEditorUtilities.h"

#include "SOrbEditorModule.h"
#include "Editor.h"
#include "Kismet2/SClassPickerDialog.h"
#include "AssetToolsModule.h"
#include "EditorLevelUtils.h"
#include "FileHelpers.h"
#include "EngineUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/LevelStreaming.h"
#include "AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "Misc/FeedbackContext.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "EditorBuildUtils.h"
#include "Engine/World.h"
#include "HierarchicalLOD.h"
#include "Engine/ObjectLibrary.h"

#include "Objects/SoSky.h"
#include "SoEngineFileHelpers.h"
#include "Items/ItemTemplates/SoItemTemplate.h"
#include "Effects/SoEffectBase.h"
#include "Character/SoCharacterStrike.h"


DEFINE_LOG_CATEGORY_STATIC(LogSoEditorUtilities, All, All);

extern FSwarmDebugOptions GSwarmDebugOptions;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorUtilities::PickChildrenOfClass(const FText& TitleText, UClass*& OutChosenClass, UClass* Class)
{
	// Create filter
	TSharedPtr<FSoChildrenOfClassFilterViewer> Filter = MakeShareable(new FSoChildrenOfClassFilterViewer);
	Filter->AllowedChildrenOfClasses.Add(Class);

	// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::DefaultView;
	Options.ClassFilter = Filter;
	//Options.bShowDisplayNames = true;

	return SClassPickerDialog::PickClass(TitleText, Options, OutChosenClass, Class);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSharedPtr<IAssetTypeActions> FSoEditorUtilities::GetAssetTypeActionsForClass(UClass* Class)
{
	check(Class);
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TSharedPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Class).Pin();
	check(AssetTypeActions.IsValid());
	return AssetTypeActions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoEditorUtilities::EditorBuildAll(UWorld* InWorld)
{
	//InWorld->AddToRoot();
	//if (!InWorld->bIsWorldInitialized)
	//{
	//	UWorld::InitializationValues IVS;
	//	IVS.RequiresHitProxies(false);
	//	IVS.ShouldSimulatePhysics(false);
	//	IVS.EnableTraceCollision(false);
	//	IVS.CreateNavigation(false);
	//	IVS.CreateAISystem(false);
	//	IVS.AllowAudioPlayback(false);
	//	IVS.CreatePhysicsScene(false);
	//	InWorld->InitWorld(IVS);
	//	InWorld->PersistentLevel->UpdateModelComponents();
	//	InWorld->UpdateWorldComponents(true, false);
	//}

	//
	//TArray<FString> SubPackages;
	//// force load all of the map to be loaded so we can get good rebuilt lighting
	//if (InWorld->WorldComposition)
	//{
	//	InWorld->WorldComposition->CollectTilesToCook(SubPackages);
	//}
	//for (const auto& SubPackage : SubPackages)
	//{
	//	UPackage* Package = LoadPackage(nullptr, *SubPackage, 0);
	//	check(Package->IsFullyLoaded());
	//}
	//InWorld->RemoveFromRoot();

	// NOTE: this follows the FBuildOptions::BuildAll path

	// BuildGeometry
	//GUnrealEd->Exec(InWorld, TEXT("MAP REBUILD ALLVISIBLE"));

	// BuildHierarchicalLOD
	// Copied from  FEditorBuildUtils::TriggerHierarchicalLODBuilder
	// Invoke HLOD generator, with either preview or full build
	// NOTE: takes too much time
	// InWorld->HierarchicalLODBuilder->BuildMeshesForLODActors(false);

	// BuildTextureStreaming
	FEditorBuildUtils::EditorBuildTextureStreaming(InWorld);

	// BuildAIPaths
	//FEditorBuildUtils::TriggerNavigationBuilder(InWorld, CurrentBuildId);

	// Lighting
	static constexpr bool bAllowLightingDialog = false;
	FEditorBuildUtils::EditorBuild(InWorld, FBuildOptions::BuildLighting, bAllowLightingDialog);

	// NOTE: GEditor->BuildReflectionCaptures is called in light build after finishing
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorUtilities::SaveAllDirtyPackages(const int32 NumAttemptsSave)
{
	UE_LOG(LogSoEditorUtilities, Verbose, TEXT("SaveAllDirtyPackages"));
	verify(GWorld->bIsWorldInitialized);
	verify(!FSoEditorUtilities::IsAnyLevelInWorldLoading(GWorld));
	verify(GWorld->WorldType != EWorldType::Inactive);

	//constexpr bool bPromptUserToSave = false;
	constexpr bool bSaveMapPackages = true;
	constexpr bool bSaveContentPackages = true;
	constexpr bool bUseDialog = false;
	//return FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave);

	// Try the first time
	bool bReturnStatus = SoEngineFileHelpers::SaveDirtyPackages(bSaveMapPackages, bSaveContentPackages, bUseDialog);

	// Retry again
	int32 CurrentAttempt = 0;
	while (!bReturnStatus && CurrentAttempt < NumAttemptsSave)
	{
		CurrentAttempt++;
		UE_LOG(LogSoEditorUtilities, Log, TEXT("SaveAllDirtyPackages: Retrying Attempt = %d"), CurrentAttempt);
		bReturnStatus = SoEngineFileHelpers::SaveDirtyPackages(bSaveMapPackages, bSaveContentPackages, bUseDialog);
	}

	return bReturnStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoEditorUtilities::WaitForLevelsInEditorToLoad()
{
	UWorld* World = GetEditorWorld();
	if (World == nullptr)
		return;

	for (const ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (!IsValid(StreamingLevel))
			continue;

		while (StreamingLevel->IsStreamingStatePending());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorUtilities::IsAnyLevelInWorldLoading(const UWorld* World)
{
	if (!World)
		return false;

	for (const ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (!IsValid(StreamingLevel))
			continue;

		if (StreamingLevel->IsStreamingStatePending() || World->IsVisibilityRequestPending())
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorUtilities::MakeEditorLevelsVisible(const TArray<FName>& LevelNamesToBeVisible, FString& OutErrorMessage, bool bHideLevelsNotInArray)
{
	UWorld* World = GetEditorWorld();
	if (World == nullptr)
	{
		OutErrorMessage = TEXT("No valid editor world found");
		return false;
	}

	int32 FoundLevelsNum = 0;

	MarkAllLevelsToBeLoaded();
	WaitForLevelsInEditorToLoad();
	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (!IsValid(StreamingLevel))
			continue;

		// Use the short package name
		const FString LevelName = GetLevelShortName(StreamingLevel);

		// Check if we should make it visible
		bool bFoundLevel = false;
		for (FName FindLevelName : LevelNamesToBeVisible)
		{
			if (FindLevelName.ToString().Equals(LevelName, ESearchCase::IgnoreCase))
			{
				bFoundLevel = true;
				FoundLevelsNum++;
				break;
			}
		}

		ULevel* Level = StreamingLevel->GetLoadedLevel();
		if (!IsValid(Level)) // Something is gravely wrong :O
		{
			OutErrorMessage = FString::Printf(TEXT("Level is not valid for LevelName = `%s`. But it was loaded, right, right?"), *LevelName);
			return false;
		}

		// Set visibility
		if (bFoundLevel)
			UEditorLevelUtils::SetLevelVisibility(Level, true, false);
		else if (bHideLevelsNotInArray)
			UEditorLevelUtils::SetLevelVisibility(Level, false, false);
	}
	WaitForLevelsInEditorToLoad();

	// Did not find all levels :(
	if (LevelNamesToBeVisible.Num() != FoundLevelsNum)
	{
		OutErrorMessage = FString::Printf(TEXT("Could not find all levels. Found only %d out of %d levels"), FoundLevelsNum, LevelNamesToBeVisible.Num());
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorUtilities::MakeAllEditorLevelsVisible(FString& OutErrorMessage)
{
	UWorld* World = GetEditorWorld();
	if (World == nullptr)
	{
		OutErrorMessage = TEXT("No valid editor world found");
		return false;
	}

	int32 FoundLevelsNum = 0;

	MarkAllLevelsToBeLoaded();
	WaitForLevelsInEditorToLoad();
	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (!IsValid(StreamingLevel))
			continue;

		ULevel* Level = StreamingLevel->GetLoadedLevel();
		if (!IsValid(Level)) // Something is gravely wrong :O
		{
			const FString LevelName = GetLevelShortName(StreamingLevel);
			OutErrorMessage = FString::Printf(TEXT("Level is not valid for LevelName = `%s`. But it was loaded, right, right?"), *LevelName);
			return false;
		}

		// Set visibility
		UEditorLevelUtils::SetLevelVisibility(Level, true, false);
	}
	WaitForLevelsInEditorToLoad();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoEditorUtilities::MarkAllLevelsToBeLoaded()
{
	UWorld* World = GetEditorWorld();
	if (World == nullptr)
		return;

	// Load all streaming levels
	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (!IsValid(StreamingLevel))
			continue;

		StreamingLevel->SetShouldBeLoaded(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UWorld* FSoEditorUtilities::GetEditorWorld()
{
	if (!GEditor)
		return nullptr;

	return GEditor->GetEditorWorldContext(true).World();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoEditorUtilities::GetEditorSoSky(ASoSky*& OutSky, FString& OutErrorMessage)
{
	OutSky = nullptr;
	UWorld* World = GetEditorWorld();
	if (World == nullptr)
	{
		OutErrorMessage = TEXT("No valid editor world found");
		return false;
	}

	// Collect skies
	TArray<ASoSky*> FoundSkies;
	for (TActorIterator<ASoSky> It(World, ASoSky::StaticClass()); It; ++It)
	{
		ASoSky* FoundSky = *It;
		if (IsValid(FoundSky))
		{
			FoundSkies.Add(FoundSky);
		}
	}

	// No skies
	if (FoundSkies.Num() == 0)
	{
		OutErrorMessage = TEXT("No SoSky found in the editor world. Are you sure there is a sky?");
		return false;
	}

	// More than one sky
	if (FoundSkies.Num() > 1)
	{
		OutErrorMessage = FString::Printf(TEXT("More Than one SoSky was found in the world. There are `%d` skies :( "), FoundSkies.Num());
		return false;
	}

	OutSky = FoundSkies[0];
	if (!IsValid(OutSky))
	{
		OutSky = nullptr;
		OutErrorMessage = TEXT("The found sky is no longer valid. what?");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoEditorUtilities::AddNotificationError(const FString& ErrorMessage)
{
	UE_LOG(LogSoEditor, Error, TEXT("%s"), *ErrorMessage);

	FNotificationInfo Notification(FText::FromString(ErrorMessage));
	Notification.ExpireDuration = 20.0f;
	Notification.bUseSuccessFailIcons = true;
	Notification.bFireAndForget = false;
	Notification.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Warning"));

	auto NotificationItem = FSlateNotificationManager::Get().AddNotification(Notification);
	NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
	NotificationItem->ExpireAndFadeout();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorUtilities::GetLevelShortName(const ULevelStreaming* StreamingLevel)
{
	if (!StreamingLevel)
		return FString();

	return FPackageName::GetShortName(StreamingLevel->GetWorldAssetPackageFName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorUtilities::GetLevelShortName(const ULevel* Level)
{
	if (!Level)
		return FString();

	if (const UPackage* Package = Level->GetOutermost())
		return FPackageName::GetShortName(Package->GetName());

	return FString();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorUtilities::GetLongPackageNameFromObject(const UObject* Object)
{
	if (!Object)
		return FString();

	if (const UPackage* Package = Object->GetOutermost())
		return Package->GetName();

	return FString();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<USoItemTemplate*> FSoEditorUtilities::GetAllItemsFromMemory()
{
	TArray<USoItemTemplate*> Array;
	for (TObjectIterator<USoItemTemplate> Itr; Itr; ++Itr)
	{
		USoItemTemplate* Item = *Itr;
		if (IsValid(Item))
		{
			Array.Add(Item);
		}
	}
	return Array;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoEditorUtilities::LoadAllItemsIntoMemory()
{
	// NOTE: All paths must NOT have the forward slash "/" at the end.
	// If they do, then this won't load Dialogues that are located in the Content root directory
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(USoItemTemplate::StaticClass(), true, GIsEditor);
	TArray<FString> PathsToSearch = { TEXT("/Game") };

	ObjectLibrary->AddToRoot();
	ObjectLibrary->LoadAssetDataFromPaths(PathsToSearch);
	const int32 Count = ObjectLibrary->LoadAssetsFromAssetData();
	ObjectLibrary->RemoveFromRoot();

	return Count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<USoCharacterStrike*> FSoEditorUtilities::GetAllCharacterStrikesFromMemory()
{
	TArray<USoCharacterStrike*> Array;
	for (TObjectIterator<USoCharacterStrike> Itr; Itr; ++Itr)
	{
		USoCharacterStrike* CharStrike = *Itr;
		if (IsValid(CharStrike))
		{
			Array.Add(CharStrike);
		}
	}
	return Array;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoEditorUtilities::LoadAllCharacterStrikesIntoMemory()
{
	// NOTE: All paths must NOT have the forward slash "/" at the end.
	// If they do, then this won't load Dialogues that are located in the Content root directory
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(USoCharacterStrike::StaticClass(), true, GIsEditor);
	TArray<FString> PathsToSearch = { TEXT("/Game") };

	ObjectLibrary->AddToRoot();
	ObjectLibrary->LoadAssetDataFromPaths(PathsToSearch);
	const int32 Count = ObjectLibrary->LoadAssetsFromAssetData();
	ObjectLibrary->RemoveFromRoot();

	return Count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TArray<UObject*> FSoEditorUtilities::GetAllEffectInstances()
{
	TArray<UObject*> Array;

	// Search in blueprints
	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> BlueprintsAssetsData;
	if (AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), BlueprintsAssetsData, true))
	{
		for (const FAssetData& AssetData : BlueprintsAssetsData)
		{
			UObject* Object = AssetData.GetAsset();
			if (const UBlueprint* Blueprint = Cast<UBlueprint>(Object))
			{
				if (Blueprint->GeneratedClass->IsChildOf(USoEffectBase::StaticClass()))
				{
					Array.Add(Object);
				}
			}
		}
	}

	// NOTE: this won't work for Blueprints that the parent our class, because the blueprint aren't loaded into memory
	for (TObjectIterator<USoEffectBase> Itr; Itr; ++Itr)
	{
		if (Itr->GetClass()->HasAnyClassFlags(EClassFlags::CLASS_Abstract))
			continue;

		USoEffectBase* Effect = *Itr;
		if (IsValid(Effect))
		{
			Array.Add(Effect);
		}
	}
	return Array;
}
