// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUISettings.h"

#include "GameFramework/GameUserSettings.h"
#include "Components/Image.h"

#include "UI/General/Buttons/SoUIButtonArray.h"
#include "UI/General/Commands/SoUICommandImage.h"

#include "Basic/SoAudioManager.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"
#include "UI/General/Buttons/SoUIButtonImage.h"

#include "UI/Menu/Settings/SoUISettingsDisplay.h"
#include "UI/Menu/Settings/SoUISettingsGame.h"
#include "UI/Menu/Settings/SoUISettingsAudio.h"
#include "UI/Menu/Settings/SoUISettingsController.h"
#include "UI/Menu/Settings/SoUISettingsBrightness.h"
#include "UI/Menu/Settings/SoUISettingsKeyboard.h"
#include "Localization/SoLocalization.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUISettings::USoUISettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TrackedInput.KeyDownThresholdSeconds = 0.2f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	CreateChildrenAndSetTexts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::NativeConstruct()
{
	Super::NativeConstruct();

	SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (TopShortcutLeft)
		TopShortcutLeft->SetUICommand(UICommand_TopLeft);

	if (TopShortcutRight)
		TopShortcutRight->SetUICommand(UICommand_TopRight);


	SubMenus->OnNavigateOnPressedHandleChildEvent().BindLambda([this](int32 SelectedChild, USoUIUserWidget* SoUserWidget)
	{
		// SFX
		SoUserWidget->NavigateOnPressed(true);

		CloseOldPanelAndOpenNew(SubMenus->GetPreviousSelectedIndex(), SelectedChild);
	});

	SetVisibility(bOpened ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!SoCharacter || !IsValid() || !SoCharacter->IsAnyUIInputPressed())
	{
		TrackedInput.Reset();
		return;
	}

	TrackedInput.Tick(InDeltaTime, [&]()
	{
		if (SoCharacter->IsUIInputPressed(UICommand_TopLeft))
			NavigateTopMenu(UICommand_TopLeft);
		else if (SoCharacter->IsUIInputPressed(UICommand_TopRight))
			NavigateTopMenu(UICommand_TopRight);
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettings::OnUICommand_Implementation(ESoUICommand Command)
{
	// Did navigate top menu
	if (NavigateTopMenu(Command))
		return true;

	// Forward the UI commands to the sub panel
	UUserWidget* CurrentSubPanel = SubPanels[SubMenus->GetSelectedIndex()];
	check(CurrentSubPanel);
	const bool bHandled = ISoUIEventHandler::Execute_OnUICommand(CurrentSubPanel, Command);

	// Close the settings if the current panel was closed
	if (!ISoUIEventHandler::Execute_IsOpened(CurrentSubPanel))
		ISoUIEventHandler::Execute_Open(this, false);

	return bHandled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettings::NavigateTopMenu(ESoUICommand Command)
{
	// Navigation on the buttons at top
	// 1. Close old panel
	// 2. Navigate on buttons
	// 3. Open new panel

	bool bNavigateTopMenu = false;
	ESoUICommand TopMenuCommand = ESoUICommand::EUC_Left;

	if (Command == UICommand_TopLeft)
	{
		bNavigateTopMenu = true;
		TopMenuCommand = ESoUICommand::EUC_Left;
	}
	else if (Command == UICommand_TopRight)
	{
		bNavigateTopMenu = true;
		TopMenuCommand = ESoUICommand::EUC_Right;
	}

	if (!bNavigateTopMenu)
		return false;

	const int32 OldPanelIndex = SubMenus->GetSelectedIndex();
	if (!CanPanelAtIndexBeInterrupted(OldPanelIndex))
		return false;

	SubMenus->Navigate(TopMenuCommand);
	CloseOldPanelAndOpenNew(OldPanelIndex, SubMenus->GetSelectedIndex());
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettings::CloseOldPanelAndOpenNew(int32 OldPanelIndex, int32 NewPanelIndex)
{
	if (!CanPanelAtIndexBeInterrupted(OldPanelIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("USoUISettings: CloseOldPanelAndOpenNew can close panel = %d"), OldPanelIndex);
		return false;
	}

	CloseAllPanels();
	OpenPanelAtIndex(NewPanelIndex);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::Open_Implementation(bool bOpen)
{
	bOpened = bOpen;
	if (bOpen)
	{
		// Open the first panel
		OpenPanelAtIndex(0, true);
	}
	else
	{
		// close all the panels
		CloseAllPanels();
	}

	SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::InstantOpen(bool bOpen)
{
	StopAllAnimations();
	ISoUIEventHandler::Execute_Open(this, bOpen);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::CreateChildrenAndSetTexts()
{
	if (SubMenus == nullptr)
		return;

	RefreshSubPanelOptions();
	SubPanels.Empty();
	TArray<FText> SubMenuTexts;
	for (const ESoUISettingsSubPanelOption Option : SubPanelOptions)
	{
		switch (Option)
		{
			case ESoUISettingsSubPanelOption::Audio:
				SubMenuTexts.Add(FROM_STRING_TABLE_UI("settings_audio"));
				SubPanels.Add(Audio);
				break;
			case ESoUISettingsSubPanelOption::Game:
				SubMenuTexts.Add(FROM_STRING_TABLE_UI("settings_game"));
				SubPanels.Add(Game);
				break;
			case ESoUISettingsSubPanelOption::Keyboard:
				SubMenuTexts.Add(FROM_STRING_TABLE_UI("settings_keyboard"));
				SubPanels.Add(Keyboard);
				break;
			case ESoUISettingsSubPanelOption::Controller:
#if PLATFORM_SWITCH
				SubMenuTexts.Add(FROM_STRING_TABLE_UI("settings_controller_switch"));
#else
				SubMenuTexts.Add(FROM_STRING_TABLE_UI("settings_controller"));
#endif
				SubPanels.Add(Controller);
				break;
			case ESoUISettingsSubPanelOption::Brightness:
				SubMenuTexts.Add(FROM_STRING_TABLE_UI("settings_brightness"));
				SubPanels.Add(Brightness);
				break;
			case ESoUISettingsSubPanelOption::Display:
			default:
				SubMenuTexts.Add(FROM_STRING_TABLE_UI("settings_display"));
				SubPanels.Add(Display);
				break;
		}
	}
	SubMenus->CreateButtons(SubMenuTexts);

	UpdateWidgetsVisibilities();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::OpenPanelAtIndex(int32 PanelIndex, bool bSelectSubmenuButton)
{
	UUserWidget* Panel = GetPanelAtIndex(PanelIndex);
	if (!Panel)
		return;

	if (bSelectSubmenuButton)
		SubMenus->SetSelectedIndex(PanelIndex);

	ISoUIEventHandler::Execute_Open(Panel, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::ClosePanelAtIndex(int32 PanelIndex)
{
	UUserWidget* Panel = GetPanelAtIndex(PanelIndex);
	if (!Panel)
		return;

	ISoUIEventHandler::Execute_Open(Panel, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::CloseAllPanels()
{
	for (UUserWidget* Panel : SubPanels)
		if (ISoUIEventHandler::Execute_IsOpened(Panel))
			ISoUIEventHandler::Execute_Open(Panel, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUISettings::CanPanelAtIndexBeInterrupted(int32 PanelIndex) const
{
	UUserWidget* Panel = GetPanelAtIndex(PanelIndex);
	if (!Panel)
		return true;

	return ISoUIEventHandler::Execute_CanBeInterrupted(Panel);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::RefreshSubPanelOptions()
{
	static constexpr int32 IndexSubPanelOptionKeyboard = 4;

	SubPanelOptions = {};

	if (!USoPlatformHelper::IsConsole())
		SubPanelOptions.Add(ESoUISettingsSubPanelOption::Display);

	SubPanelOptions.Append({
		ESoUISettingsSubPanelOption::Brightness,
		ESoUISettingsSubPanelOption::Audio,
		ESoUISettingsSubPanelOption::Game,
	});

	if (USoPlatformHelper::CanHaveKeyboard())
		SubPanelOptions.Add(ESoUISettingsSubPanelOption::Keyboard);

	SubPanelOptions.Add(ESoUISettingsSubPanelOption::Controller);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISettings::UpdateWidgetsVisibilities()
{
	if (Display)
	{

	}
	if (Keyboard)
	{

	}
}
