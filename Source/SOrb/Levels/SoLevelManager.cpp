// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoLevelManager.h"

#include "Engine/LevelStreaming.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#include "SplineLogic/SoPlayerSpline.h"
#include "SplineLogic/SoMarker.h"
#include "Engine/Engine.h"
#include "UObject/SoftObjectPath.h"
#include "SoLevelHelper.h"
#include "Basic/SoGameSingleton.h"

DEFINE_LOG_CATEGORY(LogSoLevelManager);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoLevelManager& FSoLevelManager::Get()
{
	static FSoLevelManager Instance;
	return Instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoLevelManager::Update(const FSoSplinePoint& PlayerLocation, float DeltaSeconds)
{
	// Mark current levels
	bool bAllLoaded = false;
	ClaimSplineLocation(PlayerLocation, bAllLoaded);

	for (int32 i = ActiveLevels.Num() - 1; i >= 0; --i)
	{
		// Level not seen in the past second, mark it to be hidden
		if (ActiveLevels[i].Time > 1.0f && !LoggedToHideLevels.Contains(ActiveLevels[i].Name))
		{
			LoggedToHideLevels.Add(ActiveLevels[i].Name);
			UE_LOG(LogSoLevelManager, Display, TEXT("Level %s is marked as inactive, will be turned off in %2.2f sec"), *ActiveLevels[i].Name.ToString(), HideTime);
		}

		ActiveLevels[i].Time += DeltaSeconds;

		// Hide level
		if (ActiveLevels[i].Time > HideTime)
		{
			HideLevel(PlayerLocation.GetSpline(), ActiveLevels[i].Name);
			OnClaimedLevelUnloaded(PlayerLocation.GetSpline(), ActiveLevels[i].Name);
			ActiveLevels.RemoveAtSwap(i);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelManager::ClaimSplineLocation(const FSoSplinePoint& ClaimedLocation, bool& bOutAllLoaded)
{
	bOutAllLoaded = true;

	const ASoPlayerSpline* Spline = Cast<ASoPlayerSpline>(ClaimedLocation.GetSpline());
	if (Spline == nullptr)
		return false;

	bool bAllClaimedActive = true;
	const float SplineDistance = ClaimedLocation.GetDistance();
	for (const auto& Claim : Spline->GetLevelClaims())
	{
		if (!Claim.DoesClaim(SplineDistance))
			continue;

		ULevelStreaming* Level = UGameplayStatics::GetStreamingLevel(Spline, Claim.LevelName);
		if (Level == nullptr)
			break;

		const bool bThisClaimActive = Level->IsLevelLoaded() && Level->IsLevelVisible();
		bAllClaimedActive = bAllClaimedActive && bThisClaimActive;

		FSoLevelEntry* LevelEntry = ActiveLevels.FindByPredicate([&Claim](const FSoLevelEntry& Entry) { return Entry.Name == Claim.LevelName; });
		if (LevelEntry != nullptr)
		{
			// Found level, unhide it if marked
			LevelEntry->Time = 0.0f;
			LoggedToHideLevels.Remove(Claim.LevelName);
		}
		else
		{
			if (Level->IsLevelLoaded())
				UE_LOG(LogSoLevelManager, Display, TEXT("Level is requested to be visible: %s"), *Claim.LevelName.ToString())
			else
				UE_LOG(LogSoLevelManager, Display, TEXT("Level will load: %s"), *Claim.LevelName.ToString());

			ActiveLevels.Add({ Claim.LevelName, 0.0f });
		}

		if (!bThisClaimActive)
		{
			const bool bLoaded = ShowLevel(ClaimedLocation.GetSpline(), Claim.LevelName);
			bOutAllLoaded = bLoaded && bOutAllLoaded;
		}
	}

	return bAllClaimedActive;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoLevelManager::Empty()
{
	ActiveLevels.Empty();
	DependentLevelCounters.Empty();
	LoggedToHideLevels.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoLevelManager::ClaimAndLoadAllLevels(UWorld* WorldPtr)
{
	if (WorldPtr == nullptr)
		return;

	for (ULevelStreaming* Level : WorldPtr->GetStreamingLevels())
	{
		if (!IsValid(Level))
			continue;

		//Level->SetShouldBeVisible(true);
		if (!Level->IsLevelLoaded())
		{
			const FLatentActionInfo Info;
			UGameplayStatics::LoadStreamLevel(WorldPtr, Level->PackageNameToLoad, true, true, Info);

			ActiveLevels.Add({ Level->PackageNameToLoad, 0.0f });
		}
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelManager::TeleportToChapter(const UObject* ContextObject, FName ChapterName)
{
	const FString ThisContext = FString::Printf(TEXT("TeleportToChapter(ChapterName = %d)"), *ChapterName.ToString());

	FSoChapterMapParams ChapterData;
	if (!USoLevelHelper::GetChapterData(ChapterName, ChapterData))
	{
		UE_LOG(LogSoLevelManager, Error, TEXT("%s - Could not get any chapter data for ChapterName"), *ThisContext);
		return false;
	}

	UE_LOG(
		LogSoLevelManager,
		Log,
		TEXT("%s - FriendlyName = `%s`"),
		*ThisContext, *ChapterData.NameText.ToString()
	);
	verify(ChapterData.IsValid());

	Empty();
	if (!OpenLevel(ContextObject, ChapterData.GetMapName()))
	{
		UE_LOG(LogSoLevelManager, Error, TEXT("%s - Could open ChapterName"), *ThisContext);
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelManager::TeleportToEpisode(const UObject* ContextObject, FName EpisodeName)
{
	const FString ThisContext = FString::Printf(TEXT("TeleportToEpisode(EpisodeName = %d)"), *EpisodeName.ToString());

	FSoEpisodeMapParams EpisodeData;
	if (!USoLevelHelper::GetEpisodeData(EpisodeName, EpisodeData))
	{
		UE_LOG(LogSoLevelManager, Error, TEXT("%s - Could not get any episode data for EpisodeName"), *ThisContext);
		return false;
	}

	UE_LOG(
		LogSoLevelManager,
		Log,
		TEXT("%s -  to Checkpoint = `%s`"),
		*ThisContext, *EpisodeData.CheckpointLocation.ToString()
	);
	verify(EpisodeData.IsValid());

	Empty();
	if (!OpenLevel(ContextObject, EpisodeData.GetMapName()))
	{
		UE_LOG(LogSoLevelManager, Error, TEXT("%s - Could find and open EpisodeName"), *ThisContext);
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelManager::TeleportToMainMenuLevel(const UObject* ContextObject)
{
	if (!ContextObject)
		return false;

	const FSoftObjectPath& MainMenuLevel = USoLevelHelper::GetMainMenuLevel();
	Empty();
	if (!OpenLevel(ContextObject, FName(*MainMenuLevel.GetAssetName())))
	{
		UE_LOG(LogSoLevelManager, Error, TEXT("TeleportToMainMenuLevel - Could not teleport to maine menu level = %s"), *MainMenuLevel.GetAssetName());
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoLevelManager::HideActiveLevel(const UObject* ContextObject, FName LevelName)
{
	ActiveLevels.RemoveAllSwap([&LevelName](const FSoLevelEntry& Entry) { return Entry.Name == LevelName; });
	HideLevel(ContextObject, LevelName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelManager::OpenLevel(const UObject* ContextObj, FName LevelName)
{
	// Adapted from UGameplayStatics::OpenLevel
	if (!GEngine)
		return false;

	UWorld* World = GEngine->GetWorldFromContextObject(ContextObj, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
		return false;

	const ETravelType TravelType = TRAVEL_Absolute;
	const FString Command = LevelName.ToString();
	FWorldContext &WorldContext = GEngine->GetWorldContextFromWorldChecked(World);

	FURL TestURL(&WorldContext.LastURL, *Command, TravelType);
	if (TestURL.IsLocalInternal())
	{
		// make sure the file exists if we are opening a local file
		if (!GEngine->MakeSureMapNameIsValid(TestURL.Map))
		{
			UE_LOG(LogSoLevelManager, Warning, TEXT("WARNING: The map '%s' does not exist."), *TestURL.Map);
		}
	}

	GEngine->SetClientTravel(World, *Command, TravelType);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelManager::ShowLevel(const UObject* ContextObj, FName LevelName)
{
	if (ULevelStreaming* Level = UGameplayStatics::GetStreamingLevel(ContextObj, LevelName))
	{
#if PLATFORM_SWITCH
		if (FSoNameArray* LevelListPtr = USoGameSingleton::Get().LevelUnloadMapForSwitch.Find(LevelName))
		{
			for (FName LevelToUnload : LevelListPtr->Names)
				UnloadLevelIfNotVisible(ContextObj, LevelToUnload);
		}
#endif

		if (Level->IsLevelLoaded())
		{
			Level->SetShouldBeVisible(true);
			return true;
		}
		else
		{
			FLatentActionInfo Info;
			UGameplayStatics::LoadStreamLevel(ContextObj, LevelName, true, false, Info);
			return false;
		}

	}

	UE_LOG(LogSoLevelManager, Warning, TEXT("Failed to show level: `%s` - level not loaded"), *LevelName.ToString());
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoLevelManager::HideLevel(const UObject* ContextObj, FName LevelName)
{
	if (ULevelStreaming* Level = UGameplayStatics::GetStreamingLevel(ContextObj, LevelName))
	{
		UE_LOG(LogSoLevelManager, Display, TEXT("Level became hidden: %s"), *LevelName.ToString());
		Level->SetShouldBeVisible(false);
		return true;
	}

	UE_LOG(LogSoLevelManager, Warning, TEXT("Failed to hide level: `%s` - level not loaded"), *LevelName.ToString());
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoLevelManager::UnloadLevelIfNotVisible(const UObject* ContextObj, FName LevelName)
{
	if (ULevelStreaming* Level = UGameplayStatics::GetStreamingLevel(ContextObj, LevelName))
	{
		if (Level->IsLevelLoaded() && !Level->IsLevelVisible())
		{
			UE_LOG(LogSoLevelManager, Display, TEXT("Level will be unloaded: %s"), *LevelName.ToString());

			FLatentActionInfo Info;
			UGameplayStatics::UnloadStreamLevel(ContextObj, LevelName, Info, false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoLevelManager::OnClaimedLevelUnloaded(const UObject* ContextObj, FName Level)
{
	// NOTE USED
// 	if (const FSoDependentLevels* LevelList = USoGameSingleton::GetDependentLevels(Level))
// 	{
// 		for (const FName& Name : LevelList->LevelNames)
// 		{
// 			if (!DependentLevelCounters.Contains(Name))
// 			{
// 				UE_LOG(LogSoLevelManager, Error, TEXT("A dependent level would be unloaded but it doesn't have a counter - this was considered impossible!"));
// 				DependentLevelCounters.Add(Name, 0);
// 			}
// 			else
// 			{
// 				DependentLevelCounters[Name] -= 1;
//
// #if WITH_EDITOR
// 				if (DependentLevelCounters[Name] < 0)
// 					UE_LOG(LogSoLevelManager, Error, TEXT("Counter for dependent level %s is smaller than 0 - something is broken :/"), *Name.ToString());
// #endif
// 				if (DependentLevelCounters[Name] == 0)
// 					HideLevel(ContextObj, Name);
// 			}
// 		}
// 	}
}
