// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoLevelHelper.h"

#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystem.h"

#include "SplineLogic/SoPlayerSpline.h"
#include "Basic/SoGameSingleton.h"


DEFINE_LOG_CATEGORY_STATIC(LogSoLevelHelper, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoLevelHelper::GetLevelNameFromActor(const AActor* Actor)
{
	if (Actor == nullptr)
		return NAME_None;

	if (const ULevel* Level = Actor->GetLevel())
	{
		if (const UObject* Outer = Level->GetOuter())
			return Outer->GetFName();

		return Level->GetFName();
	}

	return NAME_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoLevelHelper::GetChapterNameFromActor(const AActor* Actor)
{
	return Actor == nullptr ? NAME_None : GetChapterNameFromObject(Actor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::GetAllLevelsThatAreLoading(const UObject* WorldContextObject, TArray<FName>& OutLevelNames)
{
	if (!GEngine)
		return false;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
		return false;

	for (const ULevelStreaming* Level : World->GetStreamingLevels())
	{
		if (!IsValid(Level))
			continue;

		// Wait until level is completely loaded (loaded/visible/Correct LOD loaded)
		if (Level->IsStreamingStatePending())
		{
			OutLevelNames.Add(Level->GetWorldAssetPackageFName());
		}
	}

	return OutLevelNames.Num() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::GetAllVisibleLevels(const UObject* WorldContextObject, TArray<FName>& OutLevelNames)
{
	if (!GEngine)
		return false;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
		return false;

	for (const ULevelStreaming* Level : World->GetStreamingLevels())
	{
		if (!IsValid(Level))
			continue;

		if (Level->IsLevelVisible())
		{
			OutLevelNames.Add(Level->GetWorldAssetPackageFName());
		}
	}

	return OutLevelNames.Num() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Blueprint interface
bool USoLevelHelper::IsAnyLevelLoading(const UObject* WorldContextObject)
{
	if (!GEngine)
		return false;

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
		return false;

	if (World->IsVisibilityRequestPending())
		return true;

	//if (!World->AreAlwaysLoadedLevelsLoaded())
	//	return false;

	for (const ULevelStreaming* Level : World->GetStreamingLevels())
	{
		if (!IsValid(Level))
			continue;

		// Warn about failed to load levels
		if (Level->GetCurrentState() == ULevelStreaming::ECurrentState::FailedToLoad)
		{
			const FString LevelName = Level->GetWorldAssetPackageName();
			UE_LOG(LogSoLevelHelper, Warning, TEXT("Failed to load level name = %s"), *LevelName);
		}

		// Wait until level is completely loaded (loaded/visible/Correct LOD loaded)
		const bool bIsStreamingStatePending = Level->IsStreamingStatePending();
		if (bIsStreamingStatePending)
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::AreLevelsAtSplineLocationLoaded(const FSoSplinePoint& ClaimedLocation)
{
	const ASoPlayerSpline* Spline = Cast<ASoPlayerSpline>(ClaimedLocation.GetSpline());
	if (Spline == nullptr)
		return false;

	const float SplineDistance = ClaimedLocation.GetDistance();
	for (const auto& Claim : Spline->GetLevelClaims())
	{
		if (!Claim.DoesClaim(SplineDistance) || Claim.LevelName == USoLevelHelper::GetLevelNameFromActor(Spline))
			continue;

		ULevelStreaming* Level = UGameplayStatics::GetStreamingLevel(Spline, Claim.LevelName);
		if (Level == nullptr)
			return false;

		if (Level->GetCurrentState() == ULevelStreaming::ECurrentState::FailedToLoad)
			return false;

		if (Level->IsStreamingStatePending())
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::IsAnyLevelVisible(const UObject* ContextObject, const TSet<FName>& Levels)
{
	if (!ContextObject)
		return false;

	for (const FName Name : Levels)
		if (ULevelStreaming* Level = UGameplayStatics::GetStreamingLevel(ContextObject, Name))
			if (Level->IsLevelLoaded() && Level->IsLevelVisible())
				return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UParticleSystem* USoLevelHelper::GetWeatherVFXForLevel(FName LevelName)
{
	for (const auto& Entry : USoGameSingleton::Get().LevelsWeatherVFXArray)
	{
		if (Entry.GetLevelName() == LevelName)
		{
			return Entry.ParticlesPtr.Get();
		}
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoLevelHelper::GetMapNameStringFromObject(const UObject* WorldContextObject)
{
	if (!GEngine)
		return FString();

	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		return *World->GetName();

	return FString();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoftObjectPath* USoLevelHelper::GetLevelFromPreTag(const FString& PreTag)
{
	if (const FSoLevelParams * DataPtr = USoGameSingleton::Get().LevelDataForTags.Find(PreTag))
		return &DataPtr->Level;

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoftObjectPath& USoLevelHelper::GetMainMenuLevel()
{
	return USoGameSingleton::Get().MainMenuLevel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::IsInMenuLevel(const UObject* WorldContextObject)
{
	return GetMapNameStringFromObject(WorldContextObject) == GetMainMenuLevel().GetAssetName();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FSoftObjectPath& USoLevelHelper::GetDemoEpisodeLevel()
{
	return USoGameSingleton::Get().DemoEpisodeLevel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoLevelHelper::GetDemoMapName()
{
	return FName(*GetDemoEpisodeLevel().GetAssetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoLevelHelper::ChapterNameToFriendlyText(FName ChapterName)
{
	FSoChapterMapParams Data;
	if (GetChapterData(ChapterName, Data))
		return Data.NameText;

	return FText::GetEmpty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::GetChapterData(FName ChapterName, FSoChapterMapParams& OutData)
{
	for (const auto& Chapter : USoGameSingleton::Get().ChaptersArray)
	{
		if (Chapter.GetMapName() == ChapterName)
		{
			OutData = Chapter;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TArray<FSoChapterMapParams>& USoLevelHelper::GetAllChaptersData()
{
	return USoGameSingleton::Get().ChaptersArray;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::IsValidChapterName(FName ChapterName)
{
	for (const auto& Chapter : USoGameSingleton::Get().ChaptersArray)
	{
		if (Chapter.GetMapName() == ChapterName)
		{
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoLevelHelper::GetFirstChapterName()
{
	return FName(*USoGameSingleton::Get().FirstChapterLevel.GetAssetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoLevelHelper::GetNextChapterNameAfter(FName ChapterName)
{
	FSoChapterMapParams ChapterParams;
	if (!GetChapterData(ChapterName, ChapterParams))
	{
		UE_LOG(
			LogSoLevelHelper,
			Warning,
			TEXT("GetNextChapterNameAfter: Failed to find next chapter for ChapterName = %s"),
			*ChapterName.ToString()
		);
		return NAME_None;
	}

	return FName(*ChapterParams.NextLevel.GetAssetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoLevelHelper::GetLastChapterName()
{
	return FName(*USoGameSingleton::Get().LastChapterLevel.GetAssetName());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoLevelHelper::GetEpisodeNameDisplayText(FName EpisodeName)
{
	FSoEpisodeMapParams Data;
	if (GetEpisodeData(EpisodeName, Data))
		return Data.DisplayName;

	return FText::GetEmpty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::IsValidEpisodeName(FName EpisodeName)
{
	for (const auto& Episode : USoGameSingleton::Get().EpisodesArray)
	{
		if (Episode.GetMapName() == EpisodeName)
		{
			return true;
		}
	}

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoLevelHelper::GetFirstValidEpisodeName()
{
	if (USoPlatformHelper::IsDemo())
		return GetDemoMapName();

	// Normal game
	for (const auto& Episode : USoGameSingleton::Get().EpisodesArray)
	{
		if (Episode.IsValid())
		{
			return Episode.GetMapName();
		}
	}

	return NAME_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const TArray<FSoEpisodeMapParams>& USoLevelHelper::GetAllEpisodesData()
{
	return USoGameSingleton::Get().EpisodesArray;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::GetEpisodeData(FName EpisodeName, FSoEpisodeMapParams& OutData)
{
	for (const auto& Episode : USoGameSingleton::Get().EpisodesArray)
	{
		if (Episode.GetMapName() == EpisodeName)
		{
			OutData = Episode;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::GetMapEnterParams(FName MapName, FSoLevelEnterParams& OutData)
{
	OutData = {};

	// Chapter
	if (IsValidChapterName(MapName))
	{
		FSoChapterMapParams ChapterData;
		if (!GetChapterData(MapName, ChapterData))
			return false;

		OutData = ChapterData.LevelEnterParams;
		return true;
	}

	// Episode
	if (IsValidEpisodeName(MapName))
	{
		FSoEpisodeMapParams EpisodeData;
		if (!GetEpisodeData(MapName, EpisodeData))
			return false;

		OutData = EpisodeData.LevelEnterParams;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoLevelHelper::GetMapImageTitleDescription(FName MapName, TSoftObjectPtr<UTexture2D>& OutImage, FText& OutTitle, FText& OutDescription)
{
	OutImage.Reset();
	OutTitle = FText::GetEmpty();
	OutDescription = FText::GetEmpty();

	// Chapter
	if (IsValidChapterName(MapName))
	{
		FSoChapterMapParams ChapterData;
		if (!GetChapterData(MapName, ChapterData))
			return false;

		OutTitle = ChapterData.NameText;
		OutImage = ChapterData.LevelEnterParams.LoadingImagePtr;
		OutDescription = ChapterData.LevelEnterParams.LoadingDescription;
		return true;
	}

	// Episode
	if (IsValidEpisodeName(MapName))
	{
		FSoEpisodeMapParams EpisodeData;
		if (!GetEpisodeData(MapName, EpisodeData))
			return false;

		OutImage = EpisodeData.LevelEnterParams.LoadingImagePtr;
		OutTitle = EpisodeData.DisplayName;
		OutDescription = EpisodeData.LevelEnterParams.LoadingDescription;
		return true;
	}

	return false;
}
