// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoACarry.h"

#include "EngineMinimal.h" // ANY_PACKAGE enum search

#include "FMODEvent.h"

#include "SoADefault.h"
#include "Interactables/SoCarryable.h"
#include "Interactables/SoInteractableActor.h"
#include "Character/SoCharacter.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Basic/SoAudioManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoACarry, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoACarry::USoACarry() :
	USoActivity(EActivity::EA_Carrying)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarry::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);
	Orb->OnInteractionLabelChanged.Broadcast();

	bCanDrop = false;

	FAttachmentTransformRules Rules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
	Rules.RotationRule = EAttachmentRule::KeepWorld;
	Orb->CarriedStuff->AttachToComponent(Orb->GetMesh(), Rules, FName("Item"));
	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXCarryStart, FTransform(Orb->GetActorTransform()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarry::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);

	Orb->bShowInteractionTip = true;

	if (NewActivity != Orb->SoACarryDrop)
	{
		Orb->CarriedStuff->ForceDrop();
		Orb->CarriedStuff = nullptr;
	}

	Orb->OnInteractionLabelChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarry::Tick(float DeltaSeconds)
{
	if (Orb->CarriedStuff != nullptr)
	{
		bCanDrop = CanDrop();
		if (bCanDrop != Orb->bShowInteractionTip)
		{
			Orb->bShowInteractionTip = bCanDrop;
			Orb->OnInteractionLabelChanged.Broadcast();
		}
	}

	Super::Tick(DeltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// let's drop some junk
void USoACarry::OnInteract(AActor* Interactable)
{
	if (bCanDrop)
	{
		SwitchActivity(Orb->SoACarryDrop);
		ISoInteractable::Execute_Interact(Orb->CarriedStuff, Orb);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// let's drop some junk
void USoACarry::StrikePressed()
{
	if (bCanDrop)
	{
		SwitchActivity(Orb->SoACarryDrop);
		ISoInteractable::Execute_Interact(Orb->CarriedStuff, Orb);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoACarry::CanDrop() const
{
	const FVector Start = Orb->GetActorLocation() + Orb->GetActorForwardVector() * 96;
	const FVector End = Start + Orb->GetActorForwardVector();

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Orb);
	TraceParams.AddIgnoredActor(Orb->CarriedStuff);

	static const FName BlockAllName = FName("BlockAllDynamic");

	FHitResult HitResult;
	FCollisionShape Shape;
	Shape.SetBox({ 20,20,20 });
	return (!GetWorld()->SweepSingleByProfile(HitResult,
											  Start,
											  End,
											  FQuat::Identity,
											  BlockAllName,
											  Shape,
											  TraceParams));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarry::OnTeleportRequest()
{
	SwitchActivity(Orb->SoADefault);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoACarry::CanInteract() const
{
	// should be always true but whatever
	return Orb->Interactables.Num() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoACarryDrop::USoACarryDrop() :
	USoActivity(EActivity::EA_DropCarryable)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarryDrop::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);
	Orb->OnInteractionLabelChanged.Broadcast();

	DirectionSign = 1.0f;
	Counter = 0.0f;
	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXCarryStop, FTransform(Orb->GetActorTransform()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarryDrop::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);

	if (NewActivity != Orb->SoACarry)
	{
		const FVector Pos = Orb->GetMesh()->GetComponentTransform().TransformPosition(FVector(0.0f, 106.54f, 47.3f));
		Orb->CarriedStuff->SetActorLocation(Pos);
		Orb->CarriedStuff->Dropped(!Orb->SoMovement->IsMovingOnGround() && Orb->GetVelocity().Z > -100.0f);
		Orb->CarriedStuff = nullptr;
	}
	else
		Orb->CarriedStuff->CancelDrop();

	Orb->OnInteractionLabelChanged.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarryDrop::Tick(float DeltaSeconds)
{
	// UE_LOG(LogTemp, Warning, TEXT("Old Counter: %f DirectionSign: %f DeltaSeconds %f"), Counter, DirectionSign, DeltaSeconds);
	Counter += DeltaSeconds * DirectionSign;
	if (Counter >= Duration)
		SwitchActivity(Orb->SoADefault);
	else if (Counter < 0.0f)
		SwitchActivity(Orb->SoACarry);

	Super::Tick(DeltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarryDrop::OnAnimEvent(EAnimEvents Event)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAnimEvents"), true);
	const FString EnumName = EnumPtr ? EnumPtr->GetNameStringByIndex(static_cast<int32>(Event)) : "Invalid EAnimEvents Value";
	UE_LOG(LogSoACarry, Warning, TEXT("USoACarryDrop::OnAnimEvent animation event: %s"), *EnumName);

	if (Event == EAnimEvents::EAE_OnReleaseCarriedStuffInterrupted)
		DirectionSign = -1.0f;
	else
		USoActivity::OnAnimEvent(Event);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoACarryDrop::OnTeleportRequest()
{
	SwitchActivity(Orb->SoADefault);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//USoACarryPickUp::USoACarryPickUp() :
//	USoActivity(EActivity::EA_PickUpCarryable)
//{
//}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void USoACarryPickUp::OnEnter(USoActivity* OldActivity)
// {
// 	if (Orb->CarriedStuff != nullptr)
// 	{
// 		const FVector ObjectLocation = Orb->CarriedStuff->GetActorLocation();
// 		const FVector CharLocation = Orb->GetActorLocation();
// 
// 		const FVector CharToObjectDir = (ObjectLocation - CharLocation).GetSafeNormal();
// 		const FVector CharForward = Orb->GetActorForwardVector();
// 
// 		if ((CharToObjectDir | CharForward) < 0)
// 		{
// 			Orb->SetActorRotation((-CharForward).Rotation());
// 			UpdateCamera(0.0f);
// 		}
// 	}
// 
// 	USoActivity::OnEnter(OldActivity);
// 	Orb->OnInteractionLabelChanged.Broadcast();
// 	Counter = 0.0f;
// 	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXCarryStart, FTransform(Orb->GetActorTransform()));
// }
// 
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void USoACarryPickUp::OnExit(USoActivity* NewActivity)
// {
// 	USoActivity::OnExit(NewActivity);
// 
// 	if (NewActivity != Orb->SoACarry && NewActivity != Orb->SoACarryDrop)
// 	{
// 		Orb->CarriedStuff->ForceDrop();
// 		Orb->CarriedStuff = nullptr;
// 	}
// 	Orb->OnInteractionLabelChanged.Broadcast();
// }
// 
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void USoACarryPickUp::Tick(float DeltaSeconds)
// {
// 	const float NewCounter = Counter + DeltaSeconds;
// 
// 	if (PickedUpTime > Counter && PickedUpTime <= NewCounter && Orb->CarriedStuff != nullptr)
// 	{
// 		FAttachmentTransformRules Rules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
// 		Rules.RotationRule = EAttachmentRule::KeepWorld;
// 		Orb->CarriedStuff->AttachToComponent(Orb->GetMesh(), Rules, FName("Item"));
// 	}
// 
// 	if (NewCounter >= Duration)
// 		SwitchActivity(Orb->SoACarry);
// 	else
// 		Counter = NewCounter;
// 
// 	Super::Tick(DeltaSeconds);
// }
// 
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// void USoACarryPickUp::OnTeleportRequest()
// {
// 	SwitchActivity(Orb->SoADefault);
// }
