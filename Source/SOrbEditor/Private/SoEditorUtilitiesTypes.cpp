// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEditorUtilitiesTypes.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorChapterBuildPresetLevels::ToString(const bool bWithSky) const
{
	FString Sky;
	if (bWithSky)
	{
		Sky = FString::Printf(TEXT(", SkyPreset = %s"), *SkyPreset.ToString());
	}

	FString LevelsString;
	const int32 NumLevels = Levels.Num();
	for (int32 LevelIndex = 0; LevelIndex < NumLevels; LevelIndex++)
	{
		LevelsString += Levels[LevelIndex].ToString();
		if (LevelIndex != NumLevels - 1)
		{
			LevelsString += TEXT(", ");
		}
	}

	return FString::Printf(TEXT("Levels = {%s}%s"), *LevelsString, *Sky);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorChapterBuildPreset::ToString(const FString Prefix, const FString PrefixArray) const
{
	FString LevelsString;
	const int32 NumLevels = LevelPresets.Num();
	for (int32 LevelIndex = 0; LevelIndex < NumLevels; LevelIndex++)
	{
		LevelsString += FString::Printf(TEXT("%s%s%d. %s"), *Prefix, *PrefixArray, LevelIndex, *LevelPresets[LevelIndex].ToString());
		if (LevelIndex != NumLevels - 1)
		{
			LevelsString += TEXT("\n");
		}
	}

	return FString::Printf(TEXT("%sChapterPackageName = %s\n%sLevelPresets:\n%s"), *Prefix, *ChapterPackageName.ToString(), *Prefix, *LevelsString);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorEpisodeBuildPreset::ToString() const
{
	return FString::Printf(TEXT("EpisodePackageName = %s"), *EpisodePackageName.ToString());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorBuildPresets::ToString(const FString PrefixArray) const
{
	return FString::Printf(TEXT("%s\n\n%s "), *ToStringEpisodes(PrefixArray), *ToStringChapters(PrefixArray));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorBuildPresets::ToStringEpisodes(const FString PrefixArray) const
{
	FString EpisodeString;
	const int32 NumEpisodes = EpisodePresets.Num();
	for (int32 Index = 0; Index < NumEpisodes; Index++)
	{
		EpisodeString += FString::Printf(TEXT("%s%s"), *PrefixArray, *EpisodePresets[Index].ToString());
		if (Index != NumEpisodes - 1)
		{
			EpisodeString += TEXT("\n");
		}
	}

	return FString::Printf(TEXT("EpisodePresets:\n%s"), *EpisodeString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString FSoEditorBuildPresets::ToStringChapters(const FString PrefixArray) const
{
	FString ChaptersString;
	const int32 NumChapters = ChapterPresets.Num();
	for (int32 ChapterIndex = 0; ChapterIndex < NumChapters; ChapterIndex++)
	{
		ChaptersString += FString::Printf(TEXT("%s"), *ChapterPresets[ChapterIndex].ToString(PrefixArray, TEXT("\t")));
		if (ChapterIndex != NumChapters - 1)
		{
			ChaptersString += TEXT("\n\n");
		}
	}

	return FString::Printf(TEXT("ChapterPresets:\n%s "), *ChaptersString);
}
