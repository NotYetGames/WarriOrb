// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoWorldStateEpisodeTypes.h"

// #include "Levels/SoLevelHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoWorldStateEpisodes, All, All);

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void FSoWorldStateEpisode::CheckAndFixIntegrity()
// {
// 	if (Version < FSoWorldStateEpisodeVersion::Initial || Version > FSoWorldStateEpisodeVersion::LatestVersion)
// 	{
// 		UE_LOG(LogSoWorldStateEpisodes, Warning, TEXT("StateTable: Failed integrity, Version not in range [0, LatestVersion]. Resseting Version to LatestVersion"));
// 		Version = FSoWorldStateEpisodeVersion::LatestVersion;
// 	}
//
// 	if (TotalPlayTimeSeconds < 0.f)
// 	{
// 		UE_LOG(LogSoWorldStateEpisodes, Warning, TEXT("Failed integrity, PlayTimeSeconds < 0. Resseting to 0"));
// 		TotalPlayTimeSeconds = 0.f;
// 	}
//
// 	// Rest if not even completed
// 	if (Progress == ESoEpisodeProgress::Started)
// 	{
// 		TotalPlayTimeSeconds = 0.f;
// 	}
// }
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void FSoWorldStateEpisodesTable::CheckAndFixIntegrity()
// {
// 	if (Version < FSoWorldStateEpisodeVersion::Initial || Version > FSoWorldStateEpisodeVersion::LatestVersion)
// 	{
// 		UE_LOG(LogSoWorldStateEpisodes, Warning, TEXT("StateTable: Failed integrity, Version not in range [0, LatestVersion]. Resseting Version to LatestVersion"));
// 		Version = FSoWorldStateEpisodeVersion::LatestVersion;
// 	}
//
// 	for (auto& KeyValue : EpisodesMap)
// 		KeyValue.Value.CheckAndFixIntegrity();
// }
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FSoWorldStateEpisode& FSoWorldStateEpisodesTable::GetCurrentEpisode()
// {
// 	static FSoWorldStateEpisode Empty;
// 	if (USoLevelHelper::IsValidEpisodeName(EpisodeName))
// 		return GetOrAddEpisode(EpisodeName);
//
// 	return Empty;
// }
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void FSoWorldStateEpisodesTable::SetCurrentEpisodeState(const FSoWorldStateEpisode& NewState)
// {
// 	// Completed
// 	if (USoLevelHelper::IsValidEpisodeName(EpisodeName) && NewState.Progress == ESoEpisodeProgress::Completed)
// 	{
// 		FSoWorldStateEpisode* CurrentEpisode = GetOrAddEpisodePtr(EpisodeName);
// 		if (CurrentEpisode->Progress == ESoEpisodeProgress::Completed)
// 		{
// 			// Better time?
// 			if (CurrentEpisode->TotalPlayTimeSeconds > NewState.TotalPlayTimeSeconds)
// 				*CurrentEpisode = NewState;
// 		}
// 		else
// 		{
// 			// First time
// 			*CurrentEpisode = NewState;
// 		}
// 	}
// }
