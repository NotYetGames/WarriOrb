// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUICharacterPanels.h"

#include "Components/TextBlock.h"

#include "UI/General/Buttons/SoUIButtonArray.h"
#include "Settings/Input/SoInputHelper.h"
#include "UI/SoUIHelper.h"
#include "Basic/SoAudioManager.h"
#include "Settings/SoGameSettings.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICharacterPanels::NativeConstruct()
{
	// first so BP can use it
	SubPanels = { CharacterSheet, Inventory, Spells, Notes };

	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	for (USoInGameUIActivity* SubPanel : SubPanels)
		SubPanel->SetInGameActivityEnabled(nullptr, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUICharacterPanels::SetInGameActivityEnabled_Implementation(UObject* Source, bool bEnable)
{
	if (bOpened == bEnable)
		return bOpened;

	bShouldIgnoreMenuBackCommand = false;
	bOpened = bEnable;

	// Block game UI only if gamepad
	USoInputHelper::SetGameInputBlockedByUI(this, bOpened && USoInputHelper::IsLastInputFromGamepad(this));

	if (bOpened)
	{
		USoAudioManager::PlaySound2D(this, SFXOpen);
		USoGameSettings::Get().SetTemporaryGameSpeed(0.1f);
	}
	else
	{
		USoAudioManager::PlaySound2D(this, SFXClose);

		// Reset game speed to setting values
		USoGameSettings::Get().ApplyGameSettings(true);
	}

	SetVisibility(bOpened ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// Select current Menu
	int32 SelectedMenu = 0;
	if (bOpened)
	{
		// Open previous menu
		if (PanelToOpenOnNextStartOverride != nullptr)
			SelectedMenu = SubPanels.Find(PanelToOpenOnNextStartOverride);
		if (!SubPanels.IsValidIndex(SelectedMenu))
			SelectedMenu = 0;

		SubMenus->SetSelectedIndex(SelectedMenu);
		PanelToOpenOnNextStartOverride = nullptr;
	}
	else
		SelectedMenu = SubMenus->GetSelectedIndex();

	SubPanels[SelectedMenu]->SetInGameActivityEnabled(Source, bOpened);
	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUICharacterPanels::HandleCommand_Implementation(ESoUICommand Command)
{
	Command = USoUIHelper::TryTranslateMenuCommandDirectionToGame(Command);

	// Ignore MenuBack Command
	if (bShouldIgnoreMenuBackCommand && Command == ESoUICommand::EUC_MainMenuBack)
	{
		bShouldIgnoreMenuBackCommand = false;
		return true;
	}

	switch (Command)
	{
		case ESoUICommand::EUC_TopLeft:
			GetSelectedSubPanel()->SetInGameActivityEnabled(nullptr, false);
			SubMenus->Navigate(ESoUICommand::EUC_Left);
			GetSelectedSubPanel()->SetInGameActivityEnabled(nullptr, true);
			USoAudioManager::PlaySound2D(this, SFXPanelSwitch);
			break;

		case ESoUICommand::EUC_TopRight:
			GetSelectedSubPanel()->SetInGameActivityEnabled(nullptr, false);
			SubMenus->Navigate(ESoUICommand::EUC_Right);
			GetSelectedSubPanel()->SetInGameActivityEnabled(nullptr, true);
			USoAudioManager::PlaySound2D(this, SFXPanelSwitch);
			break;

		default:
			// Let sub panel handle command but if it is not handled by sub panel and it is a close command we close the current widget
			const bool bHandledBySubPanel = GetSelectedSubPanel()->HandleCommand(Command);

			// Subpanel wants to handle ActionBack Command most likely
			if (!bShouldIgnoreMenuBackCommand)
				bShouldIgnoreMenuBackCommand = GetSelectedSubPanel()->ShouldParentIgnoreTheNextMenuBack();

			if (!bHandledBySubPanel && IsMenuBackCommand(Command))
				SetInGameActivityEnabled(nullptr, false);

			break;
	}

	// it is a top level UI Element, will handle the input no matter what (worst case it hides itself)
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUICharacterPanels::Update_Implementation(float DeltaSeconds)
{
	return bOpened;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUICharacterPanels::IsMenuBackCommand(ESoUICommand Command) const
{
	if (Command == ESoUICommand::EUC_MainMenuBack)
		return true;

	// Only make back button work on gamepads
	if (USoInputHelper::IsLastInputFromGamepad(this) && Command == ESoUICommand::EUC_ActionBack)
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoInGameUIActivity* USoUICharacterPanels::GetSelectedSubPanel() const
{
	const int32 PanelIndex = SubMenus->GetSelectedIndex();
	verify(SubPanels.IsValidIndex(PanelIndex));
	USoInGameUIActivity* Panel = SubPanels[PanelIndex];
	verify(Panel);

	return Panel;
}
