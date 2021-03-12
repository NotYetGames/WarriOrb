// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "SoLevelTypes.h"
#include "SoEpisodeLevelTypes.h"

#include "SoLevelHelper.generated.h"


class AActor;
struct FSoSplinePoint;
class UParticleSystem;

/**
 *  Blueprint/static interface for SoLevelManager
 */
UCLASS()
class SORB_API USoLevelHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//
	// Other
	//

	// Gets an array of levels that are still loading
	static bool GetAllLevelsThatAreLoading(const UObject* WorldContextObject, TArray<FName>& OutLevelNames);

	// What it says
	static bool GetAllVisibleLevels(const UObject* WorldContextObject, TArray<FName>& OutLevelNames);

	/** Is any level loading for the provided World. */
	UFUNCTION(BlueprintPure, Category = LevelManager, meta = (WorldContext = "WorldContextObject"))
	static bool IsAnyLevelLoading(const UObject* WorldContextObject);

	/** return value false: there is at least one not yet loaded claimed level */
	static bool AreLevelsAtSplineLocationLoaded(const FSoSplinePoint& ClaimedLocation);

	// Is any of the level names inside Levels visible?
	static bool IsAnyLevelVisible(const UObject* ContextObject, const TSet<FName>& Levels);

	static FString GetMapNameStringFromObject(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = LevelManager, meta = (WorldContext = "WorldContextObject"))
	static FORCEINLINE FName GetMapNameFromObject(const UObject* WorldContextObject)
	{
		const FString MapName = GetMapNameStringFromObject(WorldContextObject);
		return MapName.IsEmpty() ? NAME_None : FName(*MapName);
	}

	/** return the name of the default level for the pretag, or NAME_None if there is any */
	static FName GetLevelNameFromPreTag(const FString& PreTag)
	{
		if (const FSoftObjectPath * AssetRefPtr = GetLevelFromPreTag(PreTag))
			return FName(*AssetRefPtr->GetAssetName());

		return NAME_None;
	}

	/** returns a ptr to map entry or nullptr for the pretag, only valid until the map isn't changed */
	static const FSoftObjectPath* GetLevelFromPreTag(const FString& PreTag);

	// Some nice weather
	static UParticleSystem* GetWeatherVFXForLevel(FName LevelName);

	//
	// Menu
	//

	// Are we in the menu level?
	static bool IsInMenuLevel(const UObject* WorldContextObject);
	static const FSoftObjectPath& GetMainMenuLevel();

	//
	// Demo
	//
	static const FSoftObjectPath& GetDemoEpisodeLevel();
	static FName GetDemoMapName();

	//
	// Chapter
	//

	// NOTE: Chapter Name is just the World->MapName and only works this ways because we use the following hierarchy
	// SoAct1 (Chapter 1): MapName == PersistentLevel
	//	 - SubLevel1
	//	 - SubLevel2

	/**
	 * Gets the Level name from the actor.
	 * NOTE: For some actors this is the same as the MapName because the actor is placed in the persistent level.
	 */
	UFUNCTION(BlueprintPure, Category = LevelManager)
	static FName GetLevelNameFromActor(const AActor* Actor);

	// Gets the ChapterName from the Actor. Only can get the chapter from the actor world
	UFUNCTION(BlueprintPure, Category = LevelManager)
	static FName GetChapterNameFromActor(const AActor* Actor);

	/** Gets the chapter name from the object */
	UFUNCTION(BlueprintPure, Category = LevelManager, meta = (WorldContext = "WorldContextObject"))
	static FORCEINLINE FName GetChapterNameFromObject(const UObject* WorldContextObject) { return GetMapNameFromObject(WorldContextObject); }

	// Is this object in the chapter level
	static FORCEINLINE bool IsInChapterLevel(const UObject* WorldContextObject)
	{
		return IsValidChapterName(GetChapterNameFromObject(WorldContextObject));
	}

	// Converts a ChapterName to a friendly chapter name
	// Eg: convert from `SoAct1` to `Chapter 1`
	static FText ChapterNameToFriendlyText(FName ChapterName);

	UFUNCTION(BlueprintPure, Category = Chapter)
	static bool GetChapterData(FName ChapterName, FSoChapterMapParams& OutData);

	static const TArray<FSoChapterMapParams>& GetAllChaptersData();
	static bool IsValidChapterName(FName ChapterName);

	UFUNCTION(BlueprintPure, Category = Chapter)
	static FName GetFirstChapterName();

	// Gets the next chapter name following ChapterName
	UFUNCTION(BlueprintPure, Category = Chapter)
	static FName GetNextChapterNameAfter(FName ChapterName);

	UFUNCTION(BlueprintPure, Category = Chapter)
	static FName GetLastChapterName();


	//
	// Episode
	//

	/** Gets the episode name from the world context object */
	UFUNCTION(BlueprintPure, Category = LevelManager, meta = (WorldContext = "WorldContextObject"))
	static FORCEINLINE FName GetEpisodeNameFromObject(const UObject* WorldContextObject) { return GetMapNameFromObject(WorldContextObject); }

	// Is this object in the episode level
	static FORCEINLINE bool IsInEpisodeLevel(const UObject* WorldContextObject)
	{
		return IsValidEpisodeName(GetEpisodeNameFromObject(WorldContextObject));
	}

	// Converts an Episode Name to a friendly name
	static FText GetEpisodeNameDisplayText(FName EpisodeName);

	static bool IsValidEpisodeName(FName EpisodeName);
	static FName GetFirstValidEpisodeName();
	static const TArray<FSoEpisodeMapParams>& GetAllEpisodesData();

	UFUNCTION(BlueprintPure, Category = Episode)
	static bool GetEpisodeData(FName EpisodeName, FSoEpisodeMapParams& OutData);

	//
	// Maps (Acts + Episodes)
	//
	static bool IsValidMapName(FName MapName) { return IsValidChapterName(MapName) || IsValidEpisodeName(MapName); }

	UFUNCTION(BlueprintPure, Category = Episode)
	static bool GetMapEnterParams(FName MapName, FSoLevelEnterParams& OutData);

	static bool GetMapImageTitleDescription(FName MapName, TSoftObjectPtr<UTexture2D>& OutImage, FText& OutTitle, FText& OutDescription);
};
