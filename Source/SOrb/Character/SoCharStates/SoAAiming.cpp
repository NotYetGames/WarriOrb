// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoAAiming.h"

#include "Components/SkeletalMeshComponent.h"

#include "SoADefault.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Items/SoItem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoAAiming::USoAAiming() :
	USoActivity(EActivity::EA_Aiming)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAAiming::OnEnter(USoActivity* OldActivity)
{
	USoActivity::OnEnter(OldActivity);

	// Orb->SoCrossbow->SetVisibility(true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAAiming::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);

	// Orb->SoCrossbow->SetVisibility(false);

	// ??? TODO
	// bLeftMouseBtnPressed = false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void USoAAiming::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	// TODO: update code (hand removed)

	// const FVector HandVecBeforeProjection = Orb->Hand->GetActorLocation() - Orb->GetMesh()->GetBoneLocation(FName("DEF_shoulder_R"), EBoneSpaces::WorldSpace);

	// temp fix:
	const FVector HandVecBeforeProjection = Orb->GetActorForwardVector();

	const FVector Direction = FVector(FVector2D(Orb->SoMovement->GetSplineLocation().GetDirection()), 0).GetSafeNormal();
	const int DirModifier = (FVector2D(Direction) | FVector2D(HandVecBeforeProjection).GetSafeNormal()) > 0 ? 1 : -1;
	// has to be projected, because the socket isn't in the spline's plane
	const FVector HandVec = FVector::VectorPlaneProject(HandVecBeforeProjection, FVector::CrossProduct(Direction, FVector(0, 0, 1)));

	const float CosA = FVector::DotProduct(HandVec.GetSafeNormal(), FVector(0, 0, 1));
	const bool bBehind = (FVector2D(Orb->GetActorForwardVector()) | FVector2D(Direction * DirModifier)) < 0;


	if (bBehind)
		Orb->DynamicAnimValue = FMath::Acos(CosA) / (2 * PI);
	else
		Orb->DynamicAnimValue = 1.0f - FMath::Acos(CosA) / (2 * PI);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAAiming::StrikePressed()
{
	FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Item0);
	if (Item.Template != nullptr && Item.Amount > 0)
	{
		//USoUsableItemTemplate* UsableTempalte = Cast<USoUsableItemTemplate>(Item.Template);
		//if (UsableTempalte != nullptr && UsableTempalte->ProjectileClass)
		//{
		//	const FSoSplinePoint CharSplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Orb);
		//	const FVector CrossbowHeadPos = Orb->SoCrossbow->GetSocketLocation("BulletStartPos");
		//	// let's try to find the spline point near the crossbow end
		//	const int32 SplineDir = CharSplineLocation.GetDirectionModifierFromVector(Orb->GetActorForwardVector());
		//	const FSoSplinePoint BulletStartSplineLocation = CharSplineLocation.GetEstimatedSplineLocation(CrossbowHeadPos, SplineDir * 300.0f);
		//	const FVector BulletStartPos = USoSplineHelper::GetWorldLocationFromSplinePointZ(BulletStartSplineLocation, CrossbowHeadPos.Z);
		//	FVector BulletDirection = BulletStartSplineLocation.GetDirectionFromVector(Orb->GetActorForwardVector());
		//	BulletDirection.Z = 0;
		//
		//	ASoBullet* Projectile = Orb->GetWorld()->SpawnActor<ASoBullet>(UsableTempalte->ProjectileClass, BulletStartPos, BulletDirection.Rotation());
		//	// TODO: bullet velocity?!
		//	Projectile->FireOnSplineNew(BulletStartSplineLocation, 600 * BulletDirection);
		//
		//	Item.Amount -= 1;
		//	Orb->GetPlayerCharacterSheet()->UpdateSlot(ESoItemSlot::EIS_Item0);
		//}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAAiming::RightBtnPressed()
{
	SwitchActivity(Orb->SoADefault);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAAiming::TakeWeaponAway()
{
	SwitchActivity(Orb->SoADefault);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAAiming::UseItemFromSlot0()
{
	// TODO: switch to armed state?
	SwitchActivity(Orb->SoADefault);
}
