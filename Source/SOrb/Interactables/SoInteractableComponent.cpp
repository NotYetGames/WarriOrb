// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoInteractableComponent.h"
#include "Components/CapsuleComponent.h"
#include "SoInteractable.h"
#include "Character/SoCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoInteractableComponent, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
USoInteractableComponent::USoInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionProfileName(FName("TriggerPawnOnly"));
	SetGenerateOverlapEvents(true);

	bActive = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts or when spawned
void USoInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddUniqueDynamic(this, &USoInteractableComponent::OnOverlapBegin);
	OnComponentEndOverlap.AddUniqueDynamic(this, &USoInteractableComponent::OnOverlapEnd);

	if (!bActive)
		SetCollisionEnabled(ECollisionEnabled::NoCollision);


	if (GetOwner() == nullptr || !GetOwner()->GetClass()->ImplementsInterface(USoInteractable::StaticClass()))
		UE_LOG(LogSoInteractableComponent, Warning, TEXT("Owner of USoInteractableComponent must implement the ISoInteractable interface!"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInteractableComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (!bActive)
		return;

	if (Cast<UCapsuleComponent>(OtherComp) == nullptr)
		return;

	ASoCharacter* SoChar = Cast<ASoCharacter>(OtherActor);
	if (SoChar != nullptr)
	{
		SoCharacter = SoChar;
		OnCharOverlapBegin.Broadcast();
		SoCharacter->AddInteractable(GetOwner());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInteractableComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bActive)
		return;

	if (Cast<UCapsuleComponent>(OtherComp) == nullptr)
		return;

	if (OtherActor == SoCharacter && SoCharacter != nullptr)
	{
		SoCharacter->RemoveInteractable(GetOwner());
		SoCharacter = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInteractableComponent::ActivateInteractable()
{
	bActive = true;
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoInteractableComponent::DeactivateInteractable()
{
	bActive = false;

	if (SoCharacter)
	{
		SoCharacter->RemoveInteractable(GetOwner());
		SoCharacter = nullptr;
	}

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
