// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIConfirmCollectAnalytics.h"

#include "Engine/Engine.h"
#include "Settings/SoGameSettings.h"
#include "UI/General/Commands/SoUICommandTooltipArray.h"
#include "Localization/SoLocalization.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmCollectAnalytics::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmCollectAnalytics::NativeConstruct()
{
	Super::NativeConstruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIConfirmCollectAnalytics::OnUICommand_Implementation(ESoUICommand Command)
{
	if (!bOpened)
		return false;

	return OnPressedButtonTooltipsCommand(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIConfirmCollectAnalytics::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	auto& GameSettings = USoGameSettings::Get();
	bool bCommandHandled = false;
	switch (Command)
	{
	case UICommand_DoNotSend:
		GameSettings.SetCollectGameAnalyticsEnabled(false);
		bCommandHandled = true;
		break;

	case UICommand_SendAutomatically:
		GameSettings.SetCollectGameAnalyticsEnabled(true);
		bCommandHandled = true;
		break;

	default:
		break;
	}

	if (bCommandHandled)
	{
		ISoUIEventHandler::Execute_Open(this, false);
		GameSettings.ApplySettings(true);
	}

	return bCommandHandled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmCollectAnalytics::RefreshButtonsArray()
{
	// Restore to default
	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	CommandsToBeDisplayed.Add({ UICommand_DoNotSend, FROM_STRING_TABLE_UI("send_no") });
	CommandsToBeDisplayed.Add({ UICommand_SendAutomatically, FROM_STRING_TABLE_UI("send_automatically") });
	UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}
