// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInteractableActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"

#include "Character/SoCharacter.h"
#include "SaveFiles/SoWorldState.h"
#include "Basic/SoGameMode.h"
#include "SaveFiles/SoWorldStateBlueprint.h"
#include "Basic/Helpers/SoStaticHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoInteractableActor::ASoInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	RootComponent = Collision;

	Collision->SetGenerateOverlapEvents(true);
	bActive = true;
	bActiveByDefault = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	bActiveByDefault = bActive;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ASoInteractableActor::OnOverlapBegin);
	Collision->OnComponentEndOverlap.AddUniqueDynamic(this, &ASoInteractableActor::OnOverlapEnd);

	if (!bActive)
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bSerializeState)
	{
		// Never change it to use the interface because that is used by child BPs!!!
		ASoGameMode::Get(this).OnPostLoad.AddDynamic(this, &ASoInteractableActor::OnReload);
		OnReload();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (bSerializeState)
		ASoGameMode::Get(this).OnPostLoad.RemoveDynamic(this, &ASoInteractableActor::OnReload);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::OnReload_Implementation()
{
	if (bSerializeState)
	{
		const bool bChanged = USoWorldState::IsActorNameInSet(this);

		if (bChanged == bActiveByDefault)
			Deactivate();
		else
			Activate();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoInteractableActor::ShouldBeRegistredAsInteractable_Implementation()
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (!bActive)
		return;

	if (Cast<UCapsuleComponent>(OtherComp) == nullptr)
		return;

	if (!ShouldBeRegistredAsInteractable())
		return;

	ASoCharacter* pChar = Cast<ASoCharacter>(OtherActor);
	if (pChar)
	{
		PlayersInArea.AddUnique(pChar);
		pChar->AddInteractable(this);
		OnCharEntered();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bActive)
		return;

	if (Cast<UCapsuleComponent>(OtherComp) == nullptr)
		return;

	ASoCharacter* pChar = Cast<ASoCharacter>(OtherActor);
	if (pChar)
	{
		pChar->RemoveInteractable(this);
		PlayersInArea.Remove(pChar);
		OnCharLeft();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::Activate()
{
	bActive = true;
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (bSerializeState)
	{
		if (bActiveByDefault)
			USoWorldState::RemoveActorNameFromSet(this);
		else
			USoWorldState::AddActorNameToSet(this);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoInteractableActor::Deactivate()
{
	bActive = false;
	for (auto* Ptr : PlayersInArea)
	{
		Ptr->RemoveInteractable(this);
		Ptr = nullptr;
	}
	PlayersInArea.Empty();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bSerializeState)
	{
		if (bActiveByDefault)
			USoWorldState::AddActorNameToSet(this);
		else
			USoWorldState::RemoveActorNameFromSet(this);
	}
}
