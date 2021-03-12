// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAWeaponInArm.h"

#include "Components/CapsuleComponent.h"

#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Character/SoWizard.h"
#include "Items/SoItem.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"
#include "Basic/SoAudioManager.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoAWeaponInArm::USoAWeaponInArm() :
	USoActivity(EActivity::EA_WeaponInArm)
{
	LookDirection = ESoLookDirection::ELD_Input;
	bArmed = true;
	// bBounceJumps = true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);

	RefreshWeapon();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::Tick(float DeltaSeconds)
{
	bLowerArms = true;

	const FVector CapsuleHalfZOffset = FVector(0.0f, 0.0f, Orb->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const FVector Start = Orb->GetActorLocation() + CapsuleHalfZOffset;
	const FVector End = Start + CapsuleHalfZOffset * 2;

	FCollisionQueryParams QuaryParams;
	QuaryParams.AddIgnoredActor(Orb);
	bLowerArms = (Orb->GetWorld()->LineTraceTestByChannel(Start, End, ECollisionChannel::ECC_WorldDynamic, QuaryParams));

	if (Orb->InterruptedWizardDialogue != nullptr)
		Orb->SoWizard->ResumeDialogueWithPlayer();

	Super::Tick(DeltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::RefreshWeapon()
{
	const FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	const USoWeaponTemplate* WeaponTemplate = Cast<const USoWeaponTemplate>(Item.Template);
	if (WeaponTemplate == nullptr)
		return;

	Orb->SelectWeapon(Item);
	ActiveWeaponType = WeaponTemplate->GetWeaponType();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::ToggleWeapons()
{
	Super::ToggleWeapons();
	RefreshWeapon();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAWeaponInArm::CanBeInterrupted(EActivity DesiredState) const
{
	return true;
	//return DesiredState == EActivity::EA_ShieldMode ||
	//	   DesiredState == EActivity::EA_ItemThrow ||
	//	   DesiredState == EActivity::EA_ItemUsage;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player don't wanna fight anymore
void USoAWeaponInArm::TakeWeaponAway()
{
	if (Orb->bForceWeaponOn)
		return;

	SwitchToRelevantState(false);
	USoAudioManager::PlaySoundAtLocation(this, Orb->SFXArmedStop, Orb->GetActorTransform(), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::OnLanded()
{
	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXArmedLand, FTransform(Orb->GetActorTransform()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::StartFloating()
{
	Super::StartFloating();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAWeaponInArm::StopFloating()
{
	Super::StopFloating();
	RefreshWeapon();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
