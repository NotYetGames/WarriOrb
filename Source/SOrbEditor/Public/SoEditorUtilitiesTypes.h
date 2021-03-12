// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "ClassViewerFilter.h"
#include "IAssetTypeActions.h"

#include "SoEditorUtilitiesTypes.generated.h"

class ASoSky;
class ULevelStreaming;
class ULevel;


USTRUCT()
struct FSoEditorChapterBuildPresetLevels
{
	GENERATED_USTRUCT_BODY()

public:
	FString ToString(const bool bWithSky = true) const;

public:
	// Sky preset used
	UPROPERTY()
	FName SkyPreset;

	// Levels to be visible
	UPROPERTY()
	TArray<FName> Levels;

	// Should we save before entering this task
	UPROPERTY()
	bool bSaveAllDirtyPackagesBefore = false;
};


USTRUCT()
struct FSoEditorChapterBuildPreset
{
	GENERATED_USTRUCT_BODY()

public:
	FString ToString(const FString Prefix = TEXT(""), const FString PrefixArray = TEXT("\t")) const;

public:
	UPROPERTY()
	FName ChapterPackageName;

	UPROPERTY()
	TArray<FSoEditorChapterBuildPresetLevels> LevelPresets;
};


USTRUCT()
struct FSoEditorEpisodeBuildPreset
{
	GENERATED_USTRUCT_BODY()

public:
	FString ToString() const;

public:
	UPROPERTY()
	FName EpisodePackageName;
};

USTRUCT()
struct FSoEditorBuildPresets
{
	GENERATED_USTRUCT_BODY()
public:
	FString ToString(const FString PrefixArray = TEXT("\t")) const;
	FString ToStringEpisodes(const FString PrefixArray = TEXT("\t")) const;
	FString ToStringChapters(const FString PrefixArray = TEXT("\t")) const;

public:
	// Custom Maps presets
	UPROPERTY()
	TArray<FSoEditorEpisodeBuildPreset> EpisodePresets;

	// Chapter presets
	UPROPERTY()
	TArray<FSoEditorChapterBuildPreset> ChapterPresets;
};


/** Filter used in the class picker to only show non abstract children of class  */
class FSoChildrenOfClassFilterViewer : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet<const UClass*> AllowedChildrenOfClasses;

	bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass,
		TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		return !InClass->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
	}

	bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
		const TSharedRef<const IUnloadedBlueprintData > InUnloadedClassData,
		TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}

private:
	/** Disallowed class flags. */
	EClassFlags DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;
};
