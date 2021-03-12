// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoSwingCenter.h"
#include "Character/SoCharacter.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets default values
ASoSwingCenter::ASoSwingCenter(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("Collision"));
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetupAttachment(RootComponent);

	Collision->SetGenerateOverlapEvents(true);

	Collision->OnComponentBeginOverlap.AddDynamic(this, &ASoSwingCenter::OnOverlapBegin);
	Collision->OnComponentEndOverlap.AddDynamic(this, &ASoSwingCenter::OnOverlapEnd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when the game starts or when spawned
void ASoSwingCenter::BeginPlay()
{
	Super::BeginPlay();

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSwingCenter::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (OtherComp == OtherActor->GetRootComponent())
	{
		ASoCharacter* Char = Cast<ASoCharacter>(OtherActor);
		if (Char != nullptr)
			Char->AddSwingCenter(this);

		CharEnteredArea();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoSwingCenter::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherComp == OtherActor->GetRootComponent())
	{
		ASoCharacter* Char = Cast<ASoCharacter>(OtherActor);
		if (Char != nullptr)
			Char->RemoveSwingCenter(this);

		CharLeftArea();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoSwingCenter::GetSwingCenter_Implementation()
{
	return RootComponent->GetComponentLocation();
}
