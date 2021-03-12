// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoADefault.h"
#include "Kismet/GameplayStatics.h"

#include "Character/SoCharacter.h"
#include "Basic/SoAudioManager.h"
#include "Character/SoWizard.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoADefault::USoADefault() :
	USoActivity(EActivity::EA_Default)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADefault::OnLanded()
{
	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFLegLand, FTransform(Orb->GetActorTransform()));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADefault::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Orb->InterruptedWizardDialogue != nullptr)
		Orb->SoWizard->ResumeDialogueWithPlayer();
}
