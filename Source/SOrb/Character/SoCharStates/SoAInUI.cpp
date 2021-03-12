// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAInUI.h"

#include "DlgManager.h"
#include "DlgContext.h"
#include "DlgDialogue.h"

#include "SoADefault.h"
#include "SoARoll.h"

#include "Character/SoWizard.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "UI/InGame/Spells/SoUISpellCastSwitch.h"
#include "Basic/SoGameInstance.h"
#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoAInUI, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoAInUI::USoAInUI() :
	USoActivity(EActivity::EA_InUI)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::OnEnter(USoActivity* OldActivity)
{
	if (OldActivity != nullptr)
	{
		bLastCollisionWasDecreased = OldActivity->ShouldDecreaseCollision();
		bArmed = OldActivity->IsArmedState();
	}

	Super::OnEnter(OldActivity);

	if (!bRollAllowed)
		Orb->bRollPressed = false;

	Orb->bLockForwardVecPressed = false;
	Orb->bUmbrellaPressed = false;
	Orb->bMiddleMousePressed = false;
	Orb->bCtrlPressed = false;
	Orb->bLeftMouseBtnPressed = false;
	Orb->bRightMouseBtnPressed = false;

	Orb->bShowInteractionTip = false;
	Orb->OnInteractionLabelChanged.Broadcast();

	bDialogueWasLast = ActiveUIActivity != nullptr && ActiveUIActivity->GetClass() == DialoguePanelClass;
	bArmAllowed = true;
	if (ActiveUIActivity != nullptr)
	{
		bArmAllowed = ActiveUIActivity->IsArmedStateAllowedOnLeave();
		bArmed = bArmed && bArmAllowed;
	}

	if (bDialogueWasLast || ActiveUIActivity->GetClass() == RestClass)
	{
		Save();
	}

	Orb->SetGameInputBlockedByUI(USoInputHelper::IsLastInputFromGamepad(this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::OnExit(USoActivity* NewActivity)
{
	if (ActiveUIActivity != nullptr)
	{
		ActiveUIActivity->SetInGameActivityEnabled(nullptr, false);

		if (ActiveUIActivity->ShouldHideUIElements())
			Orb->ChangeUIVisibility.Broadcast(true);
	}
	else
		Orb->ChangeUIVisibility.Broadcast(true);

	if (bDialogueWasLast)
		Save();

	ActiveUIActivity = nullptr;

	Super::OnExit(NewActivity);

	Orb->bShowInteractionTip = true;
	Orb->OnInteractionLabelChanged.Broadcast();

	Orb->SetGameInputBlockedByUI(false);
	bRollAllowed = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::OnPostExit(USoActivity* NewActivity)
{
	OnUILeft.Broadcast();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (InputBlockCounter > -1.0f)
		InputBlockCounter -= DeltaSeconds;

	if (ActiveUIActivity == nullptr || !ActiveUIActivity->Update(DeltaSeconds))
	{
		if (LastIngameActivity != nullptr)
			Enter(nullptr, LastIngameActivity->GetClass());
		else if (ActivityToEnterAfterLeave != nullptr)
		{
			SwitchActivity(ActivityToEnterAfterLeave);
			ActivityToEnterAfterLeave = nullptr;
		}
		else
		{
			if (bRollAllowed && Orb->bRollPressed)
				SwitchActivity(Orb->SoARoll);
			else
				SwitchToRelevantState(bArmAllowed);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::HandleUICommand(FKey Key, ESoUICommand Command)
{
	//UE_LOG(LogSoCharActivity, Warning, TEXT("USoAInUI::HandleUICommand Key = %s, Command = %s"), *Key.GetFName().ToString(), *FSoInputActionName::UICommandToActionName(Command).ToString());

	if (!CanHandleUICommand(Command) || InputBlockCounter > 0.0f)
		return;

	bool bHandleInMainMenu = Orb->IsMainMenuOpened();
	if (!bHandleInMainMenu)
	{
		// ignore time dilation if the game speed is boosted for debug
		if (USoPlatformHelper::IsGamePaused(Orb) ||
			(USoGameSettings::Get().IsTemporaryGameSpeedChanged() && USoGameSettings::Get().GetTemporaryGameSpeed() < 1.0f))
		{
			// Only works if activity works in paused game or dilated time
			if (ActiveUIActivity &&
				(ActiveUIActivity->IsActiveInPausedGame() || ActiveUIActivity->IsActiveInDilatedTime(USoGameSettings::Get().GetGameSpeed())))
			{
				bHandleInMainMenu = !ActiveUIActivity->HandleCommand(Command);
			}
			else
			{
				bHandleInMainMenu = true;
			}
		}
		else
		{
			// Running
			bHandleInMainMenu = ActiveUIActivity == nullptr || !ActiveUIActivity->HandleCommand(Command);
		}
	}

	// Let the Menu Handle it
	if (bHandleInMainMenu)
		ForwardCommandToMainMenu(Command);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAInUI::IsSaveAllowed() const
{
	if (ActiveUIActivity == nullptr)
		return true;

	return ActiveUIActivity->GetClass() != DialoguePanelClass || bForceAllowSave;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::ToggleCharacterPanels()
{
	if (Orb->IsMainMenuOpened())
		return;

	// if it was already open let's just close it
	if (ActiveUIActivity != nullptr && ActiveUIActivity->GetClass() == CharacterPanelsClass)
		SwitchToRelevantState(bArmAllowed);
	else if (Orb->SoActivity != this && Orb->SoActivity->CanOpenCharacterPanels())
		Enter(nullptr, CharacterPanelsClass);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::ToggleSpells(bool bQuickSelectionMode)
{
	if (Orb->IsMainMenuOpened())
		return;

	// Only if not current state
	if (ActiveUIActivity == nullptr && Orb->GetPlayerCharacterSheet()->CanUseSpells())
	{
		USoUISpellCastSwitch* CastSwitch = Cast<USoUISpellCastSwitch>(GetInGameUIActivity(SpellCastSwitchClass));
		if (!CastSwitch)
			return;

		CastSwitch->SetQuickSelectionMode(bQuickSelectionMode);
		if (!CastSwitch->CanBeOpened())
		{
			//UE_LOG(LogTemp, Warning, TEXT("Can't open USoUISpellCastSwitch yet"));
			return;
		}

		bRollAllowed = true;
		StartSpellCastSwitch();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAInUI::IsSpellCastSwitchOpen() const
{
	if (USoInGameUIActivity* UIActivity = GetInGameUIActivity(SpellCastSwitchClass))
		return UIActivity->IsOpened();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAInUI::Enter(UObject* Source, const TSubclassOf<USoInGameUIActivity>& UIActivityClass, bool bForceOpen)
{
	USoInGameUIActivity* UIActivity = GetInGameUIActivity(UIActivityClass);
	if (UIActivity == nullptr)
	{
		UE_LOG(LogSoAInUI, Warning, TEXT("Failed to enter to UI Activity: activity object is not setup correctly"))
		return false;
	}

	if (Orb->SoActivity != nullptr && (Orb->SoActivity->CanBeInterrupted(EActivity::EA_InUI) || bForceOpen))
	{
		bool bUIIsHidden = false;
		// Hide previous activity
		if (Orb->SoActivity == this && ActiveUIActivity != nullptr)
		{
			if (!ActiveUIActivity->ShouldKeepMusicFromPreviousUIActivity())
				Orb->UpdateMusic(true);
			ActiveUIActivity->SetInGameActivityEnabled(UIActivity, false);

			bUIIsHidden = ActiveUIActivity->ShouldHideUIElements();
		}

		if (ActiveUIActivity == Source && ActiveUIActivity != nullptr)
			LastIngameActivity = ActiveUIActivity;
		else
			LastIngameActivity = nullptr;

		// Switch to the new activity
		ActiveUIActivity = UIActivity;
		Orb->SoActivity->SwitchActivity(this);
		ActiveUIActivity->SetInGameActivityEnabled(Source, true);

		if (ActiveUIActivity->ShouldHideUIElements())
			Orb->ChangeUIVisibility.Broadcast(false);
		else if (bUIIsHidden)
			Orb->ChangeUIVisibility.Broadcast(true);

		return true;
	}

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAInUI::StartDialogue(UDlgDialogue* Dialogue, const TArray<UObject*>& Participants, bool bForced)
{
	if ((!bForced && !Orb->SoActivity->CanBeInterrupted(EActivity::EA_InUI)) ||
		Dialogue == nullptr)
		return false;

	TArray<UObject*> ParticipantList = Participants;

	if (Orb->SoWizard != nullptr && Dialogue->HasParticipant(IDlgDialogueParticipant::Execute_GetParticipantName(Orb->SoWizard)))
		ParticipantList.Add(Orb->SoWizard);

	USoInGameUIActivity** UIActivity = InGameActivities.Find(DialoguePanelClass);
	if (UIActivity != nullptr && (*UIActivity) != nullptr && Dialogue->HasParticipant(IDlgDialogueParticipant::Execute_GetParticipantName(*UIActivity)))
		ParticipantList.Add(*UIActivity);

	ParticipantList.Add(Orb);

	UDlgContext* DlgContext = UDlgManager::StartDialogue(Dialogue, ParticipantList);
	if (DlgContext != nullptr)
	{
		Enter(DlgContext, DialoguePanelClass, bForced);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::StartDialogue2(UDlgDialogue* Dialogue, UObject* Participant, bool bForced)
{
	TArray<UObject*> Participants;
	Participants.Add(Participant);
	StartDialogue(Dialogue, Participants, bForced);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::RegisterInGameActivity(USoInGameUIActivity* InGameActivity)
{
	if (InGameActivity == nullptr)
		return;

	if (InGameActivities.Find(InGameActivity->GetClass()) != nullptr)
		UE_LOG(LogSoAInUI, Warning, TEXT("Failed to register ingame activity: class %s already registered!"), *InGameActivity->GetClass()->GetName());

	InGameActivities.Add(InGameActivity->GetClass(), InGameActivity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::UnregisterInGameActivity(USoInGameUIActivity* InGameActivity)
{
	if (InGameActivity == nullptr)
		return;

	USoInGameUIActivity** UIActivity = InGameActivities.Find(InGameActivity->GetClass());

	if (UIActivity == nullptr || (*UIActivity) != InGameActivity)
		UE_LOG(LogSoAInUI, Warning, TEXT("Failed to unregister ingame activity: class %s not found or the object does not match!"), *InGameActivity->GetClass()->GetName())
	else
		InGameActivities.Remove(InGameActivity->GetClass());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoInGameUIActivity* USoAInUI::GetInGameUIActivity(const TSubclassOf<USoInGameUIActivity>& Class) const
{
	USoInGameUIActivity* const* UIActivity = InGameActivities.Find(Class);
	if (UIActivity == nullptr)
		return nullptr;

	return *UIActivity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAInUI::IsInDialogue() const
{
	if (ActiveUIActivity != nullptr)
	{
		return (ActiveUIActivity->GetClass() == DialoguePanelClass ||
			(Orb->SoWizard != nullptr && Orb->SoWizard->GetWizardDialogueUIActivityClass() == ActiveUIActivity->GetClass()));
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAInUI::Save()
{
	bForceAllowSave = true;
	Orb->AutoSave(60.0f);
	bForceAllowSave = false;
}
