// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIChallenges.h"

#include "Components/TextBlock.h"
#include "Basic/SoGameSingleton.h"
#include "SaveFiles/SoWorldState.h"
#include "Components/Image.h"
#include "Basic/SoGameInstance.h"
#include "Levels/SoLevelHelper.h"
#include "Basic/SoAudioManager.h"

#include "UI/General/Commands/SoUICommandTooltipArray.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "Online/Analytics/SoAnalytics.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "Localization/SoLocalization.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUIChallenges, All, All);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshCommandTooltips();

	// TODO find why this makes a white overlay over our preview
	// CreateDefaultChallenges();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(bOpened ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::NativeDestruct()
{
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::Open_Implementation(bool bOpen)
{
	bOpened = bOpen;

	// Reset on open
	if (bOpen)
		CreateChallengesFromWorldStateChallenges();

	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::InstantOpen(bool bOpen)
{
	StopAllAnimations();
	ISoUIEventHandler::Execute_Open(this, bOpen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIChallenges::OnUICommand_Implementation(ESoUICommand Command)
{
	switch (Command)
	{
	case ESoUICommand::EUC_Up:
	case ESoUICommand::EUC_Down:
		// Navigate along the buttons
		ChallengeButtons->Navigate(Command);
		OnEpisodeChanged(ChallengeButtons->GetSelectedIndex());
		return true;

	case UICommand_StartEpisode:
		USoAudioManager::PlaySound2D(this, SFXOpenSelected, true);
		StartEpisode(GetSelectedEpisodeName());
		return true;

	case ESoUICommand::EUC_MainMenuBack:
		// Close this widget
		ISoUIEventHandler::Execute_Open(this, false);
		return true;

	default:
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::OnEpisodeChanged_Implementation(int32 EpisodeButtonIndex)
{
	RefreshCommandTooltips();

	const FName SelectedEpisodeName = GetSelectedEpisodeName();
	if (!USoLevelHelper::IsValidEpisodeName(SelectedEpisodeName))
	{
		UE_LOG(LogSoUIChallenges, Warning, TEXT("SelectedEpisodeName = %s is invalid"), *SelectedEpisodeName.ToString());
		return;
	}

	FSoEpisodeMapParams EpisodeData;
	verify(USoLevelHelper::GetEpisodeData(SelectedEpisodeName, EpisodeData));

	if (ChallengeImage)
		ChallengeImage->SetBrushFromTexture(EpisodeData.Image);

	if (DescriptionValue)
		DescriptionValue->SetText(EpisodeData.Description);

	//if (CheckpointValue)
	//{
	//	CheckpointContainer->SetVisibility(ESlateVisibility::Hidden);
	//	//if (MapParams.CheckpointLocation == NAME_None)
	//	// TODO
	//}

	// TODO
	// const FSoWorldStateEpisode& UserData = FSoWorldState::Get().GetEpisode(SelectedEpisodeName);
	// if (StatusValue)
	// {
	// 	if (UserData.Progress == ESoEpisodeProgress::Started)
	// 	{
	// 		StatusValue->SetText(FROM_STRING_TABLE_UI("never_completed"));
	// 	}
	// 	else
	// 	{
	// 		// Completed
	// 		FFormatNamedArguments StatusArguments;
	// 		StatusArguments.Add(TEXT("time"), USoDateTimeHelper::GetTextFromSeconds(UserData.TotalPlayTimeSeconds, false));
	//
	// 		StatusValue->SetText(FText::Format(FROM_STRING_TABLE_UI("completed_in_time"), StatusArguments));
	// 	}
	// }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::RefreshCommandTooltips()
{
	if (CommandTooltips == nullptr)
		return;

	// Restore to default
	CommandTooltips->SetVisibility(ESlateVisibility::Visible);
	CommandTooltips->Clear();

	CommandTooltips->AddTooltipFromUICommand(FROM_STRING_TABLE_UI("start"), UICommand_StartEpisode);

	// Nothing to show
	if (CommandTooltips->IsEmpty())
		CommandTooltips->SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::StartEpisode(FName EpisodeName)
{
	if (!USoLevelHelper::IsValidEpisodeName(EpisodeName))
	{
		UE_LOG(LogSoUIChallenges, Warning, TEXT("Selected EpisodeName = %s is not a valid"), *EpisodeName.ToString());
		return;
	}

	auto& GameInstance = USoGameInstance::Get(this);

	// Analytics
	GameInstance.GetAnalytics()->RecordUIEpisodeStart();

	// Load
	if (!GameInstance.LoadGameForEpisode(EpisodeName, true))
		GameInstance.ResumeGame(true);

	ISoUIEventHandler::Execute_Open(this, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::CreateDefaultChallenges()
{
	if (ChallengeButtons == nullptr)
		return;

	MapEpisodeButtonToEpisodeName.SetNumZeroed(NumEpisodes);
	TArray<FText> ChallengeTexts;
	for (int32 Index = 0; Index < NumEpisodes; Index++)
	{
		FFormatOrderedArguments Args;
		Args.Add(Index);
		ChallengeTexts.Add(FText::Format(FROM_STRING_TABLE_UI("custom_map_x"), Args));
	}

	ChallengeButtons->CreateButtons(ChallengeTexts);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallenges::CreateChallengesFromWorldStateChallenges()
{
	if (ChallengeButtons == nullptr)
		return;

	MapEpisodeButtonToEpisodeName.Empty();
	TArray<FText> ButtonsTexts;

	// TODO
	// FName EpisodeName = FSoWorldState::Get().GetEpisodeName();
	// if (!USoLevelHelper::IsValidEpisodeName(EpisodeName))
	// 	EpisodeName = USoLevelHelper::GetFirstValidEpisodeName();

	int32 SelectEpisodeButtonIndex = 0;
	for (const auto& Episode : USoLevelHelper::GetAllEpisodesData())
	{
		const FName EpisodeName = Episode.GetMapName();
		ButtonsTexts.Add(Episode.DisplayName);
		MapEpisodeButtonToEpisodeName.Add(EpisodeName);

		checkNoEntry();
		if (EpisodeName == EpisodeName)
			SelectEpisodeButtonIndex = MapEpisodeButtonToEpisodeName.Num() - 1;
	}

	check(ButtonsTexts.Num() == MapEpisodeButtonToEpisodeName.Num());
	NumEpisodes = ButtonsTexts.Num();
	ChallengeButtons->CreateButtons(ButtonsTexts);

	// Select and activate current button
	// ChallengeButtons->SetSelectedIndex(SelectEpisodeButtonIndex);
	// ChallengeButtons->ActivateChildAt(SelectEpisodeButtonIndex);
	OnEpisodeChanged(ChallengeButtons->GetSelectedIndex());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoUIChallenges::GetSelectedEpisodeName() const
{
	const int32 ButtonIndex = ChallengeButtons->GetSelectedIndex();
	return MapEpisodeButtonToEpisodeName.IsValidIndex(ButtonIndex) ? MapEpisodeButtonToEpisodeName[ButtonIndex] : NAME_None;
}
