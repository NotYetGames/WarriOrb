// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInteractableLever.h"

#include "Animation/AnimSequenceBase.h"

#include "Character/SoCharStates/SoActivityTypes.h"
#include "SoInteractableComponent.h"
#include "SplineLogic/SoSplineHelper.h"
#include "Character/SoCharStates/SoALeverPush.h"
#include "SaveFiles/SoWorldState.h"
#include "Logic/SoControllable.h"
#include "Basic/SoGameMode.h"
#include "Basic/SoGameInstance.h"
#include "SaveFiles/SoWorldStateBlueprint.h"
#include "DlgDialogueParticipant.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoDateTimeHelper.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoMathHelper.h"

const FName ASoInteractableLever::PercentName = FName("Percent");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoInteractableLever::ASoInteractableLever()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SoInteractableComponent = CreateDefaultSubobject<USoInteractableComponent>(TEXT("InteractableComponent"));
	RootComponent = SoInteractableComponent;

	SoInteractableComponent->ActivateInteractable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	USoSplineHelper::UpdateSplineLocationRef(this, SplineLocationPtr, true, true);
	PulledStatePercent = DefaultPulledStatePercent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::BeginPlay()
{
	LeverActivity = ELS_Rest;
	PulledStatePercent = DefaultPulledStatePercent;

	OnReload();
	ASoGameMode::Get(this).OnPostLoad.AddDynamic(this, &ASoInteractableLever::OnReload);

	SetActorTickEnabled(false);
	Super::BeginPlay();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ASoGameMode::Get(this).OnPostLoad.RemoveDynamic(this, &ASoInteractableLever::OnReload);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::OnReload()
{
	if (USoWorldState::IsActorNameInSet(this))
	{
		PulledStatePercent = 1.0f;
		if (bTriggerOnlyOnce)
		{
			SoInteractableComponent->DeactivateInteractable();
			bTriggerBlock = true;
		}
	}
	else
	{
		PulledStatePercent = DefaultPulledStatePercent;
		if (bSavePulledStatePercent)
			USoWorldState::ReadFloatValue(this, PercentName, PulledStatePercent);
		if (bTriggerOnlyOnce)
		{
			SoInteractableComponent->ActivateInteractable();
			bTriggerBlock = false;
		}
	}
	UpdateState(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void ASoInteractableLever::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DeltaSeconds = USoDateTimeHelper::DenormalizeTime(DeltaSeconds);

	switch (LeverActivity)
	{
		case ELS_Pulled:
		{
			USoALeverPush* LeverPush = SoCharacter->SoALeverPush;
			if (LeverPush != nullptr)
			{
				SetPulledStatePercent(LeverPush->GetLeverData().ActualValue);

				if (PulledStatePercent + KINDA_SMALL_NUMBER >= 1.0f && !bTriggerBlock)
				{
					USoTriggerHelper::TriggerAll(TriggerData);
					USoTriggerHelper::TriggerActorArray(QuickTriggerList, QuickTriggerListValue);
					OnTargetsTriggeredBP();

					bTriggerBlock = PulledStatePercent > LeverResetThreshold;

					if (bTriggerOnlyOnce)
					{
						if (NameToAddToPlayer != NAME_None)
							IDlgDialogueParticipant::Execute_ModifyBoolValue(SoCharacter, NameToAddToPlayer, true);

						SoInteractableComponent->DeactivateInteractable();
						if (SoCharacter->GetActivity() == EActivity::EA_LeverPush)
							SoCharacter->SoActivity->SwitchActivity(nullptr);

						if (!bAutomaticPull)
							SoCharacter->SetMovementStop();

						LeverActivity = ESoLeverActivity::ELS_Rest;
						SetActorTickEnabled(false);
						OnInteractEndBP();
						USoWorldState::AddActorNameToSet(this);
						UpdateState(true);
						return;
					}
				}

				if (SoCharacter->GetActivity() != EActivity::EA_LeverPush)
					SwitchToReset();
			}
			else
				SwitchToReset();

			UpdateState(true);
		}
		break;

		case ELS_Reset:
		{
			if (ResetSpeed > 0.0f)
			{
				if (bUseImprovedResetMethod)
				{
					ResetTime = FMath::Max(ResetTime - DeltaSeconds, 0.0f);
					SetPulledStatePercent(USoMathHelper::InterpolateAcceleration(0.0f, 1.0f, ResetTime * ResetSpeed) * ResetStartPercent);
				}
				else
					SetPulledStatePercent(FMath::FInterpTo(PulledStatePercent, 0.0f, DeltaSeconds, ResetSpeed));

				if (PulledStatePercent < LeverResetThreshold)
					bTriggerBlock = false;

				if ((PulledStatePercent < LeverResetThreshold && bAutomaticPull) ||
					(bUseImprovedResetMethod && PulledStatePercent < KINDA_SMALL_NUMBER))
				{
					SetPulledStatePercent(0.0f);
					LeverActivity = ELS_Rest;
				}
				UpdateState(true);
			}
			else
			{
				SetActorTickEnabled(false);
				OnInteractEndBP();
			}
		}
		break;

		case ELS_Rest:
			SetActorTickEnabled(false);
			OnInteractEndBP();

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::Interact_Implementation(ASoCharacter* Character)
{
	// only allow interact if the lever is in default state
	if (bAutomaticPull && LeverActivity != ELS_Rest)
		return;

	SoCharacter = Character;

	if (SoCharacter->GetActivity() != EActivity::EA_LeverPush && CharacterAnimation != nullptr)
	{
		// Old stuff, was used before lever went rest only:
		// only allow a new interaction if the reset threshold is already reached
		//if (bAutomaticPull && LeverActivity == ELS_Reset && bTriggerBlock)
		//	return;

		FSoLeverData LeverData{ PulledStatePercent,
								1.0f,
								PullSpeed,
								CharFaceDir,
								CharacterAnimation,
								CharacterAnimation->SequenceLength / CharacterAnimation->RateScale,
								bAutomaticPull,
								WaitAfterLeverPull };

		SoCharacter->StartLeverPush(LeverData, SplineLocationPtr.Extract(), GetActorLocation().Z);

		if (SoCharacter->GetActivity() == EActivity::EA_LeverPush)
		{
			LeverActivity = ELS_Pulled;
			SetActorTickEnabled(true);
			OnInteractBP();

			if (bTriggerOnlyOnce && bAutomaticPull)
				SoCharacter->RemoveInteractable(this);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::UpdateState(bool bFromTick)
{
	OnStateUpdated(PulledStatePercent, bFromTick);

	if (ControllableTarget != nullptr)
		ISoControllable::Execute_OnControledValueChange(ControllableTarget, PulledStatePercent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::SwitchToReset()
{
	LeverActivity = ELS_Reset;

	if (bUseImprovedResetMethod)
	{
		ResetTime = 1.0f / ResetSpeed;
		ResetStartPercent = PulledStatePercent;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLever::SetPulledStatePercent(float NewPercent)
{
	PulledStatePercent = NewPercent;
	if (bSavePulledStatePercent)
		USoWorldState::WriteFloatValue(this, PercentName, PulledStatePercent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText ASoInteractableLever::GetInteractionText_Implementation() const
{
	USoALeverPush* LeverPush = SoCharacter == nullptr ? nullptr : Cast<USoALeverPush>(SoCharacter->SoActivity);

	if (bAutomaticPull)
		return DisplayText;

	if (LeverPush == nullptr || LeverPush->IsFinished())
		return DisplayText;

	return DisplayTextUsed;
}
