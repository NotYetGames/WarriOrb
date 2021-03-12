// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoOnlineRichPresence.h"
#include "Basic/SoGameInstance.h"

#if WARRIORB_WITH_STEAM
#include "INotYetSteamModule.h"
#endif
#include "SoOnlineHelper.h"
#include "Levels/SoLevelHelper.h"
#include "Character/SoPlayerProgress.h"


DEFINE_LOG_CATEGORY_STATIC(LogSoOnlineRichPresence, All, All);

const FName USoOnlineRichPresence::TokenMenu(TEXT("#menu"));
const FName USoOnlineRichPresence::TokenChapter(TEXT("#chapter"));
const FName USoOnlineRichPresence::TokenEpisode(TEXT("#challenge"));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoOnlineRichPresence::USoOnlineRichPresence()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoOnlineRichPresence::SetOwner(USoPlayerProgress* InOwner)
{
	SoOwner = InOwner;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoOnlineRichPresence::OnSplineChanged()
{
	UpdateFromCurrentState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoOnlineRichPresence::OnChapterChanged()
{
	UpdateFromCurrentState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoOnlineRichPresence::UpdateFromCurrentState()
{
	// DISABLED for now
	return;

#if WARRIORB_WITH_STEAM
	const INYSteamSubsystemPtr Steam = INotYetSteamModule::Get().GetSteamSubsystem();
	if (!Steam.IsValid())
		return;

	const bool bHasAnyProvider = Steam->IsEnabled();
#else
	const bool bHasAnyProvider = false;
#endif

	if (!bHasAnyProvider)
	{
		return;
	}

	const auto& GameInstance = USoGameInstance::Get(SoOwner);
	TSharedPtr<const FUniqueNetId> PlayerID = USoOnlineHelper::GetUniqueNetIDFromObject(SoOwner);
	if (!PlayerID.IsValid())
		return;

	TMap<FString, FText> Substitutions;
	bool bShouldContainSubstitutions = false;
	FName TokenToUse;

	// Fill Token and substitutions depending on where we are
	if (GameInstance.IsMenu())
	{
		TokenToUse = TokenMenu;
	}
	else if (GameInstance.IsEpisode())
	{
		bShouldContainSubstitutions = true;
		TokenToUse = TokenEpisode;

		// Fill current Episode FText
		const FName EpisodeName = USoLevelHelper::GetEpisodeNameFromObject(SoOwner);
		Substitutions.Add(TEXT("challenge"), USoLevelHelper::GetEpisodeNameDisplayText(EpisodeName));
	}
	else if (GameInstance.IsChapter())
	{
		bShouldContainSubstitutions = true;
		TokenToUse = TokenChapter;

		const FName ChapterName = USoLevelHelper::GetChapterNameFromObject(SoOwner);
		Substitutions.Add(TEXT("chapter"), USoLevelHelper::ChapterNameToFriendlyText(ChapterName));
	}

	// Weird
	if (TokenToUse.IsNone())
		return;

	if (bShouldContainSubstitutions && Substitutions.Num() == 0)
		return;

#if WARRIORB_WITH_STEAM
	INYSteamPresencePtr Presence = Steam->GetPresence();
	if (!Presence.IsValid())
		return;

	Presence->ClearRichPresence(*PlayerID);
	Presence->SetRichPresenceSteamDisplay(*PlayerID, TokenToUse, Substitutions);
#endif // WARRIORB_WITH_STEAM
}
