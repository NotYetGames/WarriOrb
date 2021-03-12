// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAItemUsage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Items/ItemTemplates/SoUsableItemTemplate.h"
#include "Items/SoItem.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoAudioManager.h"
#include "Projectiles/SoProjectileSpawnerComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAItemUsage::USoAItemUsage() :
	USoActivity(EActivity::EA_ItemUsage)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
	JumpType = ESoJumpType::EJT_BounceIfPressed;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAItemUsage::OnEnter(USoActivity* OldActivity)
{
	const FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Item0);
	const USoUsableItemTemplate* UsableTemplate = Cast<const USoUsableItemTemplate>(Item.Template);
	if (UsableTemplate == nullptr)
	{
		UE_LOG(LogSoItem, Error, TEXT("Tried to use item but the item in the slot is not usable!"));
		return;
	}

	UsableType = UsableTemplate->UsableType;

	bPingState = !bPingState;
	UAnimSequenceBase** AnimPtrTarget = bPingState ? &PingAnimation : &PongAnimation;
	UAnimSequenceBase** AnimPtr = Animations.Find(UsableType);
	if (AnimPtr != nullptr)
		(*AnimPtrTarget) = *AnimPtr;

	if (AnimPtr == nullptr || *AnimPtr == nullptr)
	{
		UE_LOG(LogSoItem, Error, TEXT("USoAItemUsage::OnEnter: AnimPtr for Item = `%s` is nullptr (could not be found)"), *UsableTemplate->GetPathName());
		Counter = 0.f;
	}
	else
	{
		Counter = USoStaticHelper::GetScaledAnimLength(*AnimPtr);
	}

	MeshShowThreshold = -1.0f;
	switch (UsableType)
	{
		case ESoUsableItemType::EUIT_Throwable:
			MeshShowThreshold = Counter - 0.3f;
		case ESoUsableItemType::EUIT_Potion:
			break;

		case ESoUsableItemType::EUIT_Hammer:
			MeshShowThreshold = Counter - 0.3f;
			break;
		default:
			break;
	}

	USoActivity::OnEnter(OldActivity);

	Orb->SoItemMesh->SetStaticMesh(UsableTemplate->GetStaticMesh());
	bCanBeUsed = Item.Amount > 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAItemUsage::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);
	Orb->SoItemMesh->SetVisibility(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAItemUsage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Counter -= DeltaSeconds;

	if (Counter < MeshShowThreshold)
	{
		Orb->SoItemMesh->SetVisibility(true);
		MeshShowThreshold = -1.0f;
	}

	if (Counter < 0.0f)
		return SwitchToRelevantState(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAItemUsage::OnAnimEvent(EAnimEvents Event)
{
	if (Event == EAnimEvents::EAE_OnItemLaunched)
	{
		FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Item0);
		USoUsableItemTemplate* UsableTemplate = Cast<USoUsableItemTemplate>(Item.Template);
		if (UsableTemplate != nullptr)
		{
			USoAudioManager::PlaySoundAtLocation(Orb, UsableTemplate->SFXOnUse, Orb->GetActorTransform());

			switch (UsableType)
			{
				case ESoUsableItemType::EUIT_Potion:

					if (bCanBeUsed)
					{
						Orb->GetPlayerCharacterSheet()->ApplyPotionEffects(UsableTemplate);
						Orb->GetPlayerCharacterSheet()->DecreaseItemCountOnSlot(ESoItemSlot::EIS_Item0);
					}
					break;

				case ESoUsableItemType::EUIT_Throwable:
					if (bCanBeUsed && UsableTemplate->ProjectileType != nullptr)
					{
						Orb->SoItemMesh->SetVisibility(false);

						const FVector StartPos = Orb->GetActorLocation();
						const FVector EndPos = Orb->ProjectileSpawner->GetComponentLocation();

						FCollisionQueryParams TraceParams;
						TraceParams.AddIgnoredActor(Orb);
						FHitResult Hit;
						static const FName ProfileName = FName("BlockAllDynamic");
						// check trace so character can't throw items through walls
						if (!GetWorld()->LineTraceSingleByProfile(Hit, StartPos, EndPos, ProfileName, TraceParams))
						{
							Orb->GetPlayerCharacterSheet()->DecreaseItemCountOnSlot(ESoItemSlot::EIS_Item0);

							Orb->ProjectileSpawner->SetProjectileClass(UsableTemplate->ProjectileType);
							Orb->ProjectileSpawner->SpawnProjectile();
						}
					}
				default:
					break;
			}
		}
	}
	else
		USoActivity::OnAnimEvent(Event);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAItemUsage::UseItemFromSlot0()
{
	if (UsableType == ESoUsableItemType::EUIT_Hammer)
		SwitchToRelevantState(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAItemUsage::Move(float Value)
{
	if (UsableType == ESoUsableItemType::EUIT_Throwable && !Orb->SoMovement->IsMovingOnGround())
		Super::Move(Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAItemUsage::ShouldBounceOnHit(float& OutStepUpHitVelocity, float& OutBounceDamping, const FHitResult* HitResult) const
{
	if (HitResult != nullptr && HitResult->GetActor() != nullptr && HitResult->GetActor()->ActorHasTag(NoBounceSurface))
		return false;

	if (Orb->bRollPressed)
	{
		OutStepUpHitVelocity = Orb->GetRollJumpHeight(0) / 2.f;
		OutBounceDamping = DefaultBounceDampening;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAItemUsage::OnBounce(bool bWallJump, float NewStoredMoveValue, const FVector& HitPoint, const FVector& HitNormal)
{
	if (fabs(NewStoredMoveValue) > KINDA_SMALL_NUMBER)
	{
		Orb->ForcedMovementCounter = bWallJump ? Orb->GetForcedToMoveTimeAfterWallJump() : Orb->GetForcedToMoveTimeAfterHit();
		Orb->ForcedMovementValue = NewStoredMoveValue;
	}

	Orb->SpawnBounceEffects(bWallJump, HitPoint);
}
