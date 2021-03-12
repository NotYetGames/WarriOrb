// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Levels/SoEpisodeLevelTypes.h"

// #include "SoWorldStateEpisodeTypes.generated.h"


// Keep track of version in save files
struct FSoWorldStateEpisodeVersion
{
	enum Type
	{
		Initial = 0,
		RemovedEpisodeName,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
};

// // Holds the current episode
// USTRUCT(BlueprintType)
// struct FSoWorldStateEpisode
// {
// 	GENERATED_USTRUCT_BODY()
// public:
// 	void CheckAndFixIntegrity();
// 	FORCEINLINE void UpdateToLatestVersion()
// 	{
// 		Version = FSoWorldStateEpisodeVersion::LatestVersion;
// 	}
//
// 	// Update SaveTime to reflect the current time.
// 	FORCEINLINE void SetSaveTimeToNow()
// 	{
// 		// UTC time so that we can convert back easily to local time if the user changes time zone
// 		SaveTime = FName(*FDateTime::UtcNow().ToIso8601());
// 	}
//
// public:
// 	UPROPERTY()
// 	int32 Version = FSoWorldStateEpisodeVersion::LatestVersion;
//
// 	// Time this save was made at, UTC time
// 	UPROPERTY()
// 	FName SaveTime;
//
// 	UPROPERTY()
// 	ESoEpisodeProgress Progress = ESoEpisodeProgress::Started;
//
// 	UPROPERTY(BlueprintReadOnly)
// 	float TotalPlayTimeSeconds = 0.f;
// };
//
//
// // Holds all the episodes
// USTRUCT(BlueprintType)
// struct FSoWorldStateEpisodesTable
// {
// 	GENERATED_USTRUCT_BODY()
// public:
// 	void CheckAndFixIntegrity();
//
// 	FORCEINLINE void UpdateToLatestVersion()
// 	{
// 		Version = FSoWorldStateEpisodeVersion::LatestVersion;
// 		for (auto& KeyValue : EpisodesMap)
// 			KeyValue.Value.UpdateToLatestVersion();
// 	}
//
// 	FSoWorldStateEpisode& GetCurrentEpisode();
// 	void SetCurrentEpisodeState(const FSoWorldStateEpisode& NewState);
//
// 	FORCEINLINE const FSoWorldStateEpisode& GetConstCurrentEpisode()
// 	{
// 		return const_cast<FSoWorldStateEpisodesTable*>(this)->GetCurrentEpisode();
// 	}
//
// 	FORCEINLINE FSoWorldStateEpisode& GetOrAddEpisode(FName EpisodeName) { return EpisodesMap.FindOrAdd(EpisodeName); }
// 	FORCEINLINE FSoWorldStateEpisode* GetOrAddEpisodePtr(FName EpisodeName) { return &GetOrAddEpisode(EpisodeName); }
// 	FORCEINLINE void SetSaveTimeToNow() { GetCurrentEpisode().SetSaveTimeToNow(); }
//
// public:
// 	UPROPERTY()
// 	int32 Version = FSoWorldStateEpisodeVersion::LatestVersion;
//
// 	UPROPERTY()
// 	TMap<FName, FSoWorldStateEpisode> EpisodesMap;
//
// 	// Tells us about the current/last used episode name from the EpisodesArray
// 	UPROPERTY()
// 	FName EpisodeName = NAME_None;
// };
