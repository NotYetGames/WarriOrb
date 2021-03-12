// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoALillian.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"


#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Interactables/SoInteractable.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoALillian::USoALillian() :
	USoActivity(EActivity::EA_Lillian)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoALillian::OnEnter(USoActivity* OldActivity)
{
	Super::OnEnter(OldActivity);

	Orb->GetMesh()->SetVisibility(false);
	Orb->LillianMesh->SetVisibility(true);
	Orb->LillianMesh->SetComponentTickEnabled(true);
	Orb->LillianMesh->bNoSkeletonUpdate = false;

	Orb->SoNewSmoke->SetVisibility(false);
	Orb->SoNewLights->SetVisibility(false);
	Orb->SoGlowSphere->SetVisibility(false);

	Orb->SoSword->SetVisibility(false, true);
	Orb->SoOffHandWeapon->SetVisibility(false, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoALillian::OnExit(USoActivity* NewActivity)
{
	Super::OnExit(NewActivity);

	if (NewActivity->GetID() != EActivity::EA_InUI &&
		NewActivity->GetID() != EActivity::EA_Teleport&&
		NewActivity->GetID() != EActivity::EA_WaitForActivitySwitch)
	{
		Orb->GetMesh()->SetVisibility(true);
		Orb->LillianMesh->SetVisibility(false);
		Orb->LillianMesh->SetComponentTickEnabled(false);
		Orb->LillianMesh->bNoSkeletonUpdate = true;

		Orb->SoNewSmoke->SetVisibility(true);
		Orb->SoNewLights->SetVisibility(true);
		Orb->SoGlowSphere->SetVisibility(true);

		Orb->SelectWeapon(Orb->SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoALillian::OnInteract(AActor* Interactable)
{
	if (Interactable != nullptr)
		ISoInteractable::Execute_Interact(Interactable, Orb);
}