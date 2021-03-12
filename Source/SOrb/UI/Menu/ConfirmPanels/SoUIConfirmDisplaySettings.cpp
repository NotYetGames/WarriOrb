// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIConfirmDisplaySettings.h"

#include "Engine/Engine.h"
#include "Components/TextBlock.h"

#include "Localization/SoLocalization.h"
#include "Settings/SoGameSettings.h"
#include "UI/General/Commands/SoUICommandTooltipArray.h"
#include "UI/General/Buttons/SoUIButtonImageArray.h"
#include "UI/General/SoUIUserWidget.h"
#include "Basic/SoAudioManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ResetSecondsToDefault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::NativeConstruct()
{
	Super::NativeConstruct();

	ButtonsArray->OnNavigateOnPressedHandleChildEvent().BindLambda([this](int32 SelectedChild, USoUIUserWidget* SoUserWidget)
	{
		// SFX
		SoUserWidget->NavigateOnPressed(true);

		if (USoUIButtonImageArray* ButtonTooltips = GetButtonArrayAsButtonImagesArray())
			OnPressedButtonTooltipsCommand(ButtonTooltips->GetSelectedButtonCommand());

	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bOpened)
	{
		CurrentSeconds -= InDeltaTime;

		if (CurrentSeconds > 0.2f)
		{
			OverlayTime->SetText(FText::AsNumber(FMath::RoundToInt(CurrentSeconds)));
		}
		else
		{
			ISoUIEventHandler::Execute_Open(this, false);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::Open_Implementation(bool bOpen)
{
	Super::Open_Implementation(bOpen);
	ResetSecondsToDefault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIConfirmDisplaySettings::OnUICommand_Implementation(ESoUICommand Command)
{
	if (!bOpened)
		return false;

	switch (Command)
	{
	case ESoUICommand::EUC_Left:
	case ESoUICommand::EUC_Right:
	case ESoUICommand::EUC_Up:
	case ESoUICommand::EUC_Down:
	case ESoUICommand::EUC_MainMenuEnter:
		ButtonsArray->Navigate(Command);
		return true;

	default:
		return OnPressedButtonTooltipsCommand(Command);
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::RefreshButtonsArray()
{
	// Restore to default
	TArray<FSoUITextCommandPair> CommandsToBeDisplayed;
	for (const ESoUIConfirmDisplaySettingsType Option : AnswerButtonOption)
	{
		switch (Option)
		{
		case ESoUIConfirmDisplaySettingsType::Confirm:
			CommandsToBeDisplayed.Add({ UICommand_ConfirmSettings, FROM_STRING_TABLE_UI("confirm") });
			break;

		case ESoUIConfirmDisplaySettingsType::Cancel:
			CommandsToBeDisplayed.Add({ UICommand_CancelSettings, FROM_STRING_TABLE_UI("cancel") });
			break;

		default:
			checkNoEntry();
			break;
		}
	}

	UpdateEnsureOnlyTheseCommandsExist(CommandsToBeDisplayed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIConfirmDisplaySettings::OnPressedButtonTooltipsCommand(ESoUICommand Command)
{
	Super::OnPressedButtonTooltipsCommand(Command);

	switch (Command)
	{
	case UICommand_ConfirmSettings:
		OnConfirmedVideoMode();
		break;

	case UICommand_CancelSettings:
		OnRevertVideoMode();
		break;

	default:
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::OnConfirmedVideoMode()
{
	auto& UserSettings = USoGameSettings::Get();

	// Only need to save the settings
	UserSettings.ConfirmVideoMode();
	UserSettings.ValidateSettings();
	UserSettings.SaveSettings();

	// Call BP
	ReceiveOnConfirmedVideoMode();

	// Close this as we handled it
	ISoUIEventHandler::Execute_Open(this, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::OnRevertVideoMode()
{
	auto& UserSettings = USoGameSettings::Get();

	// Only need to to directly apply the settings and update the option
	UserSettings.RevertVideoMode();
	UserSettings.ApplyDisplaySettings(true);

	// Close this as we handled it
	ISoUIEventHandler::Execute_Open(this, false);

	// Call BP
	ReceiveOnRevertVideoMode();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIConfirmDisplaySettings::ResetSecondsToDefault()
{
	CurrentSeconds = DefaultSeconds;

	if (OverlayTime)
		OverlayTime->SetText(FText::AsNumber(FMath::RoundToInt(CurrentSeconds)));
}
