// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIChallengeCompleted.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"

#include "Levels/SoLevelHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameInstance.h"
#include "Character/SoCharacter.h"
#include "UI/General/Buttons/SoUIButtonImage.h"
#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/SoUIExternalLink.h"

#include "Localization/SoLocalization.h"
#include "Basic/Helpers/SoPlatformHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallengeCompleted::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (ButtonsArray)
	{
		ButtonsArray
			->SetVerticalLayout(false)
			->SetFakeVirtualContainer(true)
			->SetSkipDeactivatedChildren(false);

		ButtonsArray->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ButtonImageToolTipExit)
		ButtonImageToolTipExit->SetVisibility(bDemoEnding ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (TextDemoThankYou)
		TextDemoThankYou->SetVisibility(bDemoEnding ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ButtonDiscord)
		ButtonDiscord->SetVisibility(bDemoEnding ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ButtonTwitter)
		ButtonTwitter->SetVisibility(bDemoEnding ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ButtonSteam)
		ButtonSteam->SetVisibility(bDemoEnding && !USoPlatformHelper::IsConsole() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (ButtonExit)
		ButtonExit->SetVisibility(bDemoEnding ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallengeCompleted::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);

	if (ButtonExit)
	{
		ButtonExit->OnOpenLinkEvent().BindUObject(this, &ThisClass::OnExitPressed);
	}

	// NOTE: because it is a fake virtual container
	ButtonsArray->AddContainerChild(ButtonDiscord);
	ButtonsArray->AddContainerChild(ButtonTwitter);

	if (!USoPlatformHelper::IsConsole())
		ButtonsArray->AddContainerChild(ButtonSteam);

	ButtonsArray->AddContainerChild(ButtonExit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIChallengeCompleted::OnExitPressed()
{
	bOpened = false;
	SetVisibility(ESlateVisibility::Collapsed);
	USoGameInstance::Get(this).TeleportToMainMenu(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIChallengeCompleted::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	// Ignore all close requests
	if (!bEnable)
		return false;

	bOpened = bEnable;
	SetVisibility(ESlateVisibility::Visible);

	// Init the buttons
	ButtonImageToolTipExit->SetUICommand(CommandLeave, true);
	ButtonImageToolTipExit->SetButtonText(FROM_STRING_TABLE_INTERACTION("leave"));

	// Fill episode data
	const FName EpisodeName = USoLevelHelper::GetEpisodeNameFromObject(this);
	FSoEpisodeMapParams EpisodeData;
	USoLevelHelper::GetEpisodeData(EpisodeName, EpisodeData);
	Subtitle->SetText(EpisodeData.CompletedSubtitle);
	Description->SetText(EpisodeData.CompletedDescription);

	// Override music
	if (ASoCharacter* Char = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		Char->SetMusicOverride(MusicToPlay, true, this);
	}

	if (bDemoEnding)
	{
		ButtonsArray->SetSelectedIndex(0);
	}

	// Enable mouse
	USoPlatformHelper::AllowMouse(this);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIChallengeCompleted::HandleCommand_Implementation(ESoUICommand Command)
{
	if (IsAnyAnimationPlaying())
		return true;

	// Forward to children
	if (bDemoEnding)
	{
		ButtonsArray->Navigate(Command);
	}
	else
	{
		if (Command == CommandLeave)
		{
			OnExitPressed();
		}
		else if (Command == ESoUICommand::EUC_MainMenuBack)
		{
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIChallengeCompleted::Update_Implementation(float DeltaSeconds)
{
	return bOpened;
}
