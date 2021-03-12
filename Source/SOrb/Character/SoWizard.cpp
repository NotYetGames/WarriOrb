// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoWizard.h"

#include "TimerManager.h"
#include "EngineUtils.h"

#include "DlgManager.h"
#include "DlgContext.h"

#include "SoCharacter.h"
#include "DlgDialogue.h"
#include "Character/SoCharStates/SoAInUI.h"

#include "SoLocalization.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoWizard, Log, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
USoWizard::USoWizard()
{
	DialogueData.PositionIndex = 2;
	DialogueData.ParticipantName = TEXT("Wizard");
	DialogueData.ParticipantDisplayName = FROM_STRING_TABLE_DIALOGUE("npc_wizard");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWizard::Initialize(ASoCharacter* PlayerCharacter)
{
	Character = PlayerCharacter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWizard::StartDialogueWithPlayer(UDlgDialogue* Dialogue)
{
	if (Dialogue == nullptr || Character == nullptr || Character->bDebugSkipDialogues)
		return false;

	UDlgContext* DlgContext = nullptr;
	switch (Dialogue->GetParticipantsNum())
	{
		case 1:
		{
			if (!Dialogue->HasParticipant(DialogueData.ParticipantName))
				UE_LOG(LogSoWizard, Warning, TEXT("Failed to start dialogue = `%s`: the Wizard must participate!"), *Dialogue->GetPathName())
			else
				DlgContext = UDlgManager::StartMonologue(Dialogue, this);
		}
		break;

		case 2:
		{
			if (!Dialogue->HasParticipant(DialogueData.ParticipantName) ||
				Character == nullptr ||
				!Dialogue->HasParticipant(IDlgDialogueParticipant::Execute_GetParticipantName(Character)))
			{
				UE_LOG(LogSoWizard, Warning, TEXT("Failed to start dialogue = `%s`: the Wizard and the Character must participate!"), *Dialogue->GetPathName());
			}
			else
				DlgContext = UDlgManager::StartDialogue2(Dialogue, this, Character);
		}
		break;

		default:
			UE_LOG(LogSoWizard, Warning, TEXT("Failed to start dialogue = `%s`: invalid participant count!"), *Dialogue->GetPathName());
	}

	if (DlgContext == nullptr)
		return false;

	Character->SoAInUI->Enter(DlgContext, WizardDialogueUIActivityClass, true);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWizard::ResumeDialogueWithPlayer()
{
	if (Character->GetInterruptedWizardDialogue() != nullptr)
		Character->SoAInUI->Enter(Character->GetInterruptedWizardDialogue(), WizardDialogueUIActivityClass, true);
}
