// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoADead.h"

#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"

#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"
#include "Character/SoSpringArmComponent.h"
#include "Character/SoCharStates/SoATeleport.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Basic/SoGameMode.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoADead::USoADead() :
	USoActivity(EActivity::EA_Dead)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADead::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);

	// Died :(
	// update is needed before we can save the values, because if the character is rotating in air right now they may be wrong
	// would be nicer to get the values from the function or simply calculate them, but this works perfectly and less work so why not
	USoActivity::UpdateCamera(0.0f);
	SavedCamRotation = Orb->CameraBoom->GetComponentRotation();
	SavedCamWorldLocation = Orb->CameraBoom->GetComponentLocation();

	SetDisabledState();
	Orb->SoBreak->Activate(true);

	Orb->OnMortalDeath.Broadcast();

	Orb->OnDeathBP(bWithSoulkeeper);
	bBanTimeOver = false;

	for (int32 i = 0; i < Orb->Cooldowns.Num(); ++i)
		Orb->CooldownEnded.Broadcast(i, -1.0f, Orb->Cooldowns[i].Object);
	Orb->Cooldowns.Empty();

	Counter = 0.0f;

	bRevived = false;
	bTryToRespawn = false;

	// Save game
	Orb->AutoSave(60.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADead::Tick(float DeltaSeconds)
{
	if (Counter < GetInputBanTime())
	{
		Counter += DeltaSeconds;
		if (Counter >= GetInputBanTime())
			bBanTimeOver = true;
	}

	if (bTryToRespawn)
	{
		bRevived = true;
		Orb->Revive(bWithSoulkeeper, true, bWouldLeaveSpline);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADead::InteractKeyPressed(bool bPrimary)
{
	if (bBanTimeOver)
	{
		if (bPrimary)
		{
			if (ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(Orb->SoMovement->GetSplineLocation().GetSpline()))
				if (PlayerSpline->GetResPointOverride(true))
				{
					bWouldLeaveSpline = true;
					bTryToRespawn = true;
				}
		}
		else
		{
			bWouldLeaveSpline = false;
			bTryToRespawn = true;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADead::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);

	if (NewActivity != Orb->SoATeleport)
	{
		SetEnabledState();
		// must be some cheat, e.g. switch to super edit mode
		if (!bRevived)
			Orb->OnRespawn(false);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADead::UpdateCamera(float DeltaSeconds)
{
	Orb->CameraBoom->SetWorldLocation(SavedCamWorldLocation);
	Orb->CameraBoom->SetWorldRotation(SavedCamRotation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoADead::OverrideCameraData(const FRotator& Rotation, const FVector& Location)
{
	SavedCamWorldLocation = Location;
	SavedCamRotation = Rotation;
	UpdateCamera(0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoADead::GetInputBanTime() const
{
	return USoDateTimeHelper::NormalizeTime(InputBanTime);
}
