// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInteractableLeverSwitch.h"

#include "Animation/AnimSequenceBase.h"

#include "Basic/SoGameMode.h"
#include "Basic/SoGameInstance.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SoInteractableComponent.h"
#include "SplineLogic/SoSplineHelper.h"
#include "SaveFiles/SoWorldState.h"
#include "SaveFiles/SoWorldStateBlueprint.h"
#include "Logic/SoTriggerable.h"
#include "Character/SoCharStates/SoALeverPush.h"
#include "Character/SoCharStates/SoActivityTypes.h"
#include "Character/SoCharacter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoInteractableLeverSwitch::ASoInteractableLeverSwitch()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SoInteractableComponent = CreateDefaultSubobject<USoInteractableComponent>(TEXT("InteractableComponent"));
	RootComponent = SoInteractableComponent;

	SoInteractableComponent->ActivateInteractable();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLeverSwitch::OnConstruction(const FTransform& Transform)
{
	USoSplineHelper::UpdateSplineLocationRef(this, SplineLocationPtr, true, true);
	bSet = bSetByDefault;
	PulledStatePercent = bSetByDefault ? 1.0f : 0.0f;

	Super::OnConstruction(Transform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLeverSwitch::BeginPlay()
{
	if (bSerializeState)
	{
		OnReload();
		ASoGameMode::Get(this).OnPostLoad.AddDynamic(this, &ASoInteractableLeverSwitch::OnReload);
	}
	else
		OnStateChangeBP();

	SetActorTickEnabled(false);
	Super::BeginPlay();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLeverSwitch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (bSerializeState)
	{
		ASoGameMode::Get(this).OnPostLoad.RemoveDynamic(this, &ASoInteractableLeverSwitch::OnReload);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLeverSwitch::OnReload()
{
	bSet = bSetByDefault;

	if (USoWorldState::IsActorNameInSet(this))
		bSet = !bSet;

	PulledStatePercent = bSet ? 1.0f : 0.0f;
	OnStateChangeBP();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called every frame
void ASoInteractableLeverSwitch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (SoCharacter == nullptr)
	{
		SetActorTickEnabled(false);
		return;
	}

	USoALeverPush* LeverPush = SoCharacter->SoALeverPush;
	if (LeverPush != nullptr)
	{
		const FSoLeverData& LeverData = LeverPush->GetLeverData();
		PulledStatePercent = LeverData.ActualValue;
		OnStateChangeBP();

		if (fabs(PulledStatePercent - LeverData.TargetValue) < KINDA_SMALL_NUMBER)
		{
			TArray<AActor*>& TriggerData = bSet ? TriggerOnSet : TriggerOnReset;
			for (AActor* Actor : TriggerData)
				if (Actor != nullptr && Actor->GetClass()->ImplementsInterface(USoTriggerable::StaticClass()))
					ISoTriggerable::Execute_Trigger(Actor, { bSet ? SetValue : ResetValue, {} });

			OnInteractionEndBP();

			if (bSerializeState)
			{
				if (bSetByDefault == bSet)
					USoWorldState::RemoveActorNameFromSet(this);
				else
					USoWorldState::AddActorNameToSet(this);
			}
		}
		else
			return;
	}

	SetActorTickEnabled(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableLeverSwitch::Interact_Implementation(ASoCharacter* Character)
{
	SoCharacter = Character;

	if (SoCharacter->GetActivity() != EActivity::EA_LeverPush && CharacterAnimation != nullptr)
	{
		bSet = !bSet;

		const FSoLeverData LeverData{ PulledStatePercent,
								bSet ? 1.0f : 0.0f,
								PullSpeed * (bSet ? 1.0f : -1.0f),
								CharFaceDir,
								CharacterAnimation,
								CharacterAnimation->SequenceLength / CharacterAnimation->RateScale,
								true,
								WaitAfterLeverPull };

		SoCharacter->StartLeverPush(LeverData, SplineLocationPtr.Extract(), GetActorLocation().Z);

		if (SoCharacter->GetActivity() == EActivity::EA_LeverPush)
		{
			SetActorTickEnabled(true);
			OnCharacterInteractBP();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText ASoInteractableLeverSwitch::GetInteractionText_Implementation() const
{
	return bSet ? DisplayTextInSetState : DisplayTextInResetState;
}
