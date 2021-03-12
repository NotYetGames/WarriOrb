// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAStrike.h"

#include "TimerManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
// #include "DrawDebugHelpers.h"

#include "SoASlide.h"
#include "CharacterBase/SoCharacterMovementComponent.h"
#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Character/SoCharacterStrike.h"
#include "Character/SoCharStates/SoAWeaponInArm.h"
#include "Projectiles/SoProjectileSpawnerComponent.h"
#include "Items/SoItem.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"
#include "Basic/SoAudioManager.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoGameMode.h"
#include "Enemy/SoEnemy.h"
#include "Items/SoItemHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set base flags
USoAStrike::USoAStrike() :
	USoActivity(EActivity::EA_Striking)
{
	LookDirection = ESoLookDirection::ELD_Frozen;
	bArmed = true;
	bStaminaReg = false;
	JumpType = ESoJumpType::EJT_BounceIfPressed;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::OnEnter(USoActivity* OldActivity)
{
	const FSoItem& Item = Orb->SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	USoActivity::OnEnter(OldActivity);
	Orb->ApplyStaticEffects(Item, true);

	Strike();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::OnExit(USoActivity* NewActivity)
{
	USoActivity::OnExit(NewActivity);

	if (!NewActivity->IsArmedState())
		Orb->OnArmedStateLeft();

	Orb->SoMovement->ClearRootMotionDesc();
	Orb->gTimerManager->ClearTimer(StrikePressedRecentlyTimer);

	bIsInStrike = false;
	StrikeComboChainIndex = -1;

	const FSoItem& Item = Orb->SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	Orb->ApplyStaticEffects(Item, false);

	Orb->SoMovement->GravityScale = 2.0f;

	if (AppliedEffect != ESoStatusEffect::ESE_NumOf)
	{
		ISoMortal::Execute_OnStatusEffectChanged(Orb, AppliedEffect, false);
		AppliedEffect = ESoStatusEffect::ESE_NumOf;
	}
	bSpawnedCanNotSFX = false;

	ActiveTrailMesh = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::Move(float Value)
{
	USoActivity::Move(bFreezeInAir ? 0.0f : Value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::Tick(float DeltaSeconds)
{
	USoActivity::Tick(DeltaSeconds);

	const float CorrectedDeltaSeconds = DeltaSeconds * AttackSpeedMultiplier;

	// update strike
	const float OldTime = StrikeCounter;
	StrikeCounter -= CorrectedDeltaSeconds;

	if (bWaitBeforeStrike)
	{
		// if (Orb->bRollPressed)
		// 	RollPressed();
		// else
		if (StrikeCounter < 0.0f)
		{
			bWaitBeforeStrike = false;
			StrikeComboChainIndex -= 1;
			Strike();
		}

		return;
	}

	const float PassedTime = StrikeCounterStart - StrikeCounter;
	const float OldPassedTime = StrikeCounterStart - OldTime;
	if (ActiveStrikeData != nullptr)
	{
		for (int32 i = 0; i < ActiveStrikeData->StrikeList.Num(); ++i)
		{
			const FSoStrikeEntry& StrikeData = ActiveStrikeData->StrikeList[i];

			if (OldPassedTime <= StrikeData.TrailDelay && PassedTime > StrikeData.TrailDelay)
				ApplyTrail(StrikeData, i == 0);

			// if (OldPassedTime <= StrikeData.StrikeDelay && PassedTime > StrikeData.StrikeDelay)
			if ((OldPassedTime >= StrikeData.StrikeDelay && OldPassedTime <= StrikeData.StrikeDelay + StrikeData.StrikeDmgDuration) ||
				(PassedTime >= StrikeData.StrikeDelay && PassedTime <= StrikeData.StrikeDelay + StrikeData.StrikeDmgDuration))
			{
				ensure(TrailMeshes.IsValidIndex(i));
				ApplyDamage(StrikeData, TrailMeshes[i], ActiveStrikeData->IgnoreWallHitBellowZThreshold, i);

				if (ActiveStrikeData->bForceGroundAnimsOnBlendOut)
					Orb->bForceGroundAnims = true;
			}
		}

		for (const FSoJump& Jump : ActiveStrikeData->Jumps)
			if (OldPassedTime <= Jump.Time && PassedTime > Jump.Time)
			{
				Orb->SoMovement->Velocity.Z = Jump.Velocity;
				Orb->SoMovement->SetMovementMode(MOVE_Falling);
			}

		// NOT USED right now
		// for (const FSoHorizontalMovement& Movement : ActiveStrikeData->HorizontalMovements)
		// {
		// 	if (OldPassedTime <= Movement.StartTime && PassedTime > Movement.StartTime)
		// 	{
		// 		Orb->SoMovement->MaxWalkSpeed = Movement.Velocity;
		// 		const float OldVelocityZ = Orb->SoMovement->Velocity.Z;
		// 		Orb->SoMovement->Velocity = Orb->GetActorForwardVector() * Movement.Velocity;
		// 		Orb->SoMovement->Velocity.Z = OldVelocityZ;
		//
		// 		Orb->ForcedMovementCounter = Movement.EndTime - Movement.StartTime;
		// 		Orb->ForcedMovementValue = Orb->SoMovement->GetSplineLocation().GetDirectionModifierFromVector(Orb->SoMovement->Velocity);
		// 	}
		//
		// 	if (OldPassedTime <= Movement.EndTime && PassedTime > Movement.EndTime)
		// 	{
		// 		Orb->SoMovement->MaxWalkSpeed = MovementSpeed;
		// 		Orb->ForcedMovementCounter = -1.0f; // just in case
		// 		Orb->LastInputDirection = Orb->GetActorForwardVector();
		// 	}
		// }

		if (ActiveStrikeData->ProjectileClass != nullptr && OldPassedTime <= ActiveStrikeData->RangeAttackDelay && PassedTime > ActiveStrikeData->RangeAttackDelay)
		{
			FSoSplinePoint SplinePoint = Orb->SoMovement->GetSplineLocation();
			SplinePoint += SplinePoint.GetDirectionModifierFromVector(Orb->GetActorForwardVector()) * 100;
			Orb->WeaponProjectileSpawner->SpawnFromProjectileOnSpline(ActiveStrikeData->ProjectileClass,
																	  SplinePoint.ToVector(Orb->GetActorLocation().Z),
																	  Orb->GetActorRotation(),
																	  SplinePoint);
			if (ActiveStrikeData->WeaponToHideIndex == 0)
				Orb->ForceHideWeapon(ActiveStrikeData->WeaponHideDuration, true);
			else if (ActiveStrikeData->WeaponToHideIndex == 1)
				Orb->ForceHideWeapon(ActiveStrikeData->WeaponHideDuration, false);
		}
	}

	if (StrikeCounter < ActiveBlendOutTime)
	{
		Orb->SoMovement->GravityScale = 2.0f;
		bIsInStrike = false;
		if (Orb->bRollPressed)
		{
			OnStrikeChainFinished(ActiveBlendOutTime - FMath::Max(StrikeCounter, 0.0f));
			RollPressed();
			return;
		}
		if (bStrikePressedRecently)
		{
			const FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
			USoWeaponTemplate* WeaponTemplate = Cast<USoWeaponTemplate>(Item.Template);
			if (WeaponTemplate != nullptr)
			{
				const bool bStrikeSwitch = Orb->bSpecialStrikeWasRequestedLast != bSpecialStrikeWasExecutedLast;
				const bool bActiveStrikeHasMore = WeaponTemplate->IsValidStrikeIndex(StrikeComboChainIndex + 1, bSpecialStrikeWasExecutedLast);

				if (!bStrikeSwitch &&
					(bActiveStrikeHasMore || !HasStrikeCooldown(bSpecialStrikeWasExecutedLast)))
				{
					if (!bActiveStrikeHasMore)
						StrikeComboChainIndex = -1;

					Strike();
					return;
				}

				if (bStrikeSwitch)
				{
					if (CanStrike(Orb->bSpecialStrikeWasRequestedLast, !bSpawnedCanNotSFX))
					{
						OnStrikeChainFinished(ActiveBlendOutTime - FMath::Max(StrikeCounter, 0.0f));
						StrikeComboChainIndex = -1;
						Strike();
						return;
					}
					bSpawnedCanNotSFX = true;
				}
			}
		}

		if (StrikeCounter < 0.0f)
		{
			OnStrikeChainFinished(ActiveBlendOutTime);
			if (LastActivity == Orb->SoASlide && Orb->SoMovement->IsMovingOnGround())
				SwitchActivity(Orb->SoASlide);
			else
				SwitchActivity(Orb->SoAWeaponInArm);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::Strike()
{
	ActiveTrailMesh = nullptr;

	if (Orb->bFreezeOrientationUntilLand)
		Orb->bFreezeOrientationUntilLand = false;

	bStrikePressedRecently = false;
	StrikeComboChainIndex += 1;
	bStrikeA = !bStrikeA;
	bWaitBeforeStrike = false;

	ComponentsAlreadyHitA.Empty();
	ComponentsAlreadyHitB.Empty();

	const FSoItem& Item = Orb->GetPlayerCharacterSheet()->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	USoWeaponTemplate* WeaponTemplate = Cast<USoWeaponTemplate>(Item.Template);
	if (WeaponTemplate == nullptr || !WeaponTemplate->IsValidStrikeIndex(StrikeComboChainIndex, Orb->bSpecialStrikeWasRequestedLast))
	{
		SwitchActivity(Orb->SoAWeaponInArm);
		return;
	}

	ActiveAutoAimDistance = WeaponTemplate->GetAutoAimDistance();
	USoCharacterStrike* Strike = WeaponTemplate->GetStrike(Orb->bSpecialStrikeWasRequestedLast);

	const FSoCharacterStrikeData& StrikeData = Strike->GetStrikeData(StrikeComboChainIndex);
	bStrikeBlendIn = StrikeData.bBlendIn;

	// if weapon is force hidden and this strike does not throw weapon or it wants to throw the weapon which is hidden then display that weapon
	if (Orb->bForceHideLeftWeapon || Orb->bForceHideRightWeapon)
	{
		if (StrikeData.ProjectileClass == nullptr ||
			((Orb->bForceHideLeftWeapon && StrikeData.WeaponToHideIndex == 1) ||
			 (Orb->bForceHideRightWeapon && StrikeData.WeaponToHideIndex == 0)))
			Orb->ClearForceHideWeapon();
	}


	if (StrikeData.bRequiresJump)
	{
		const bool bMovingOnGround = Orb->GetCharacterMovement()->IsMovingOnGround();
		const float TimeSinceLastJump = Orb->GetWorld()->GetTimeSeconds() - Orb->LastJumpTime;
		const float Delay = StrikeData.bStartWithLand ? 0.5f : 0.2f;
		if (bMovingOnGround || TimeSinceLastJump < Delay)
		{
			JumpPressed();
			bWaitBeforeStrike = true;
			StrikeCounter = bMovingOnGround ? Delay : Delay - TimeSinceLastJump;
			bIsInStrike = false;
			return;
		}
	}

	Orb->SelectWeapon(Item);

	// UE_LOG(LogTemp, Warning, TEXT("Strike %d"), StrikeIndex);
	AttackSpeedMultiplier = Orb->SoPlayerCharacterSheet->GetAttackSpeedMultiplier();

	// fill data for animation blueprint
	FSoStrikeAnimData& AnimData = bStrikeA ? AnimDataA : AnimDataB;
	AnimData.Anim = StrikeData.AnimSequenceNew;
	AnimData.AnimEndBlend = StrikeData.AnimEndingSequenceRight;
	AnimData.AnimEndBlendLeft = StrikeData.AnimEndingSequenceLeft;
	if (AnimData.Anim != nullptr)
	{
		AnimData.AnimPlayRate = (AnimData.Anim->GetPlayLength() / StrikeData.AnimDuration) * AttackSpeedMultiplier;
		StrikeCounter = StrikeData.AnimDuration + StrikeData.AnimEndDuration;
		StrikeCounterStart = StrikeCounter;
		ActiveBlendOutTime = StrikeData.AnimEndDuration;
	}
	else
	{
		SwitchActivity(Orb->SoAWeaponInArm);
		return;
	}

	if (AnimData.AnimEndBlend != nullptr)
		AnimData.AnimEndBlendPlayRate = (AnimData.AnimEndBlend->GetPlayLength() / StrikeData.AnimEndDuration) * AttackSpeedMultiplier;

	bIsInStrike = true;

	// update orientation
	if (StrikeData.ProjectileClass != nullptr || !AutoAim())
	{
		const float MovementAxisValue = Orb->GetMovementXAxisValue();
		const FVector MovDir = Orb->SoMovement->GetDirection() * MovementAxisValue * ((Orb->SavedCamMovModifier != 0) ? Orb->SavedCamMovModifier : 1);
		if (fabs(MovementAxisValue) > KINDA_SMALL_NUMBER)
		{
			if ((MovDir.GetSafeNormal() | Orb->GetActorForwardVector()) < 0.0f)
			{
				Orb->SetActorRotation(MovDir.Rotation());
				UpdateCamera(0.01f);
				Orb->StoredForwardVector = Orb->GetActorForwardVector();
			}
		}
	}


	FSoRootMotionDesc RootMotionDesc = StrikeData.RootMotionDesc;
	if (RootMotionDesc.bVerticalRootMotionEnabled && StrikeData.bStopVerticalRootMotionNearApex && !Orb->SoMovement->IsMovingOnGround())
		RootMotionDesc.bVerticalRootMotionEnabled = false;

	Orb->SoMovement->SetRootMotionDesc(RootMotionDesc);

	RangeAttackCounter = 0.0f;
	ActiveStrikeData = &StrikeData;
	ActiveWeaponTemplate = WeaponTemplate;

	bSpecialStrikeWasExecutedLast = Orb->bSpecialStrikeWasRequestedLast;

	TrailMeshes.Empty(StrikeData.StrikeList.Num());

	bFreezeInAir = StrikeData.bFreezeInAir && !Orb->SoMovement->IsMovingOnGround();
	Orb->SoMovement->GravityScale = bFreezeInAir ? 0.0f : 2.0f;
	if (bFreezeInAir)
		Orb->SoMovement->Velocity = FVector(0.0f, 0.0f, 0.0f);

	const ESoStatusEffect NewAppliedEffect = Strike->GetStatusEffect();

	if (NewAppliedEffect != AppliedEffect && NewAppliedEffect != ESoStatusEffect::ESE_NumOf)
	{
		ISoMortal::Execute_OnStatusEffectChanged(Orb, AppliedEffect, false);
		AppliedEffect = NewAppliedEffect;
		if (AppliedEffect != ESoStatusEffect::ESE_NumOf)
			ISoMortal::Execute_OnStatusEffectChanged(Orb, AppliedEffect, true);
	}

	bSpawnedCanNotSFX = false;

	if (StrikeData.bStartWithLand && !Orb->SoMovement->IsMovingOnGround())
		ForceLand();
	else
		LandingAmount = -1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::ForceLand()
{
	const FVector Start = Orb->GetActorLocation();
	const FVector ToEnd = -FVector(0.f, 0.f, 1000);

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(Orb);
	static const FName PawnName = FName("Pawn");

	FHitResult HitData(ForceInit);
	if (GetWorld()->SweepSingleByProfile(HitData,
									 Start,
									 Start + ToEnd,
									 Orb->GetCapsuleComponent()->GetComponentQuat(),
									 PawnName,
									 Orb->GetCapsuleComponent()->GetCollisionShape(),
									 TraceParams))
	{
		LandingAmount = -ToEnd.Z * HitData.Time;
		Orb->SetActorLocation(Start + HitData.Time * ToEnd, true);
		Orb->SoMovement->Velocity = FVector(0.0f, 0.0f, 0.0f);
		Orb->JumpPressedRecentlyTimeOver();

		if (!AutoAim() && Orb->LastInputDirection.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			Orb->SetActorRotation(Orb->LastInputDirection.Rotation());
			UpdateCamera(0.01f);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::OnStrikeChainFinished(float BlendTimeToSubtractFromCounter)
{
	USoCharacterStrike* CharacterStrike = nullptr;
	if (ActiveWeaponTemplate != nullptr)
		CharacterStrike = ActiveWeaponTemplate->GetStrike(bSpecialStrikeWasExecutedLast);

	if (CharacterStrike != nullptr)
	{
		const float Cooldown = CharacterStrike->GetCooldownTime();
		if (Cooldown > 0.0f)
			Orb->AddCooldown(CharacterStrike, Cooldown - BlendTimeToSubtractFromCounter, CharacterStrike->CanCountDownInAir_Implementation());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::ApplyTrail(const struct FSoStrikeEntry& StrikeData, bool bAllowTurn)
{
	AutoAim();

	USkeletalMeshComponent* CharMesh = Orb->GetMesh();
	AStaticMeshActor* StaticMeshActor = nullptr;
	if (Orb->WeaponTrailMeshes.Num() == 0)
	{
		StaticMeshActor = Orb->GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), CharMesh->GetComponentTransform());
		StaticMeshActor->SetMobility(EComponentMobility::Movable);
	}
	else
	{
		StaticMeshActor = Orb->WeaponTrailMeshes.Pop();
		StaticMeshActor->GetStaticMeshComponent()->SetVisibility(true);
		StaticMeshActor->SetActorTransform(CharMesh->GetComponentTransform());
		// DrawDebugSphere(Orb->GetWorld(), CharMesh->GetComponentTransform().GetLocation(), 16.0f, 32, FColor::Red, true, 100.0f);
	}
	UStaticMeshComponent* TrailComponent = StaticMeshActor->GetStaticMeshComponent();
	TrailComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrailComponent->SetStaticMesh(StrikeData.TrailMesh);
	TrailComponent->SetTranslucentSortPriority(StrikeData.TranslucencySortPriority);
	if (StrikeData.bAttachToCharacter)
	{
		StaticMeshActor->AttachToComponent(Orb->GetMesh(), FAttachmentTransformRules::KeepWorldTransform);
		StaticMeshActor->GetRootComponent()->bAbsoluteRotation = true;
	}
	else
		StaticMeshActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	StaticMeshActor->SetActorRotation(CharMesh->GetComponentRotation()); // no idea why but it has to be called :/

	Orb->WeaponTrailMeshFadeList.Add({StaticMeshActor, StrikeData.TrailDisplayTime, StrikeData.TrailDisplayTime });
	ActiveTrailMesh = StaticMeshActor;

	static const FName SlideValueName = FName("Slide");
	TrailComponent->SetScalarParameterValueOnMaterials(SlideValueName, 0.0f);
	static const FName ColorValueName = FName("Color");
	static const FName AnimValueName = FName("AnimLight");
	if (ActiveWeaponTemplate != nullptr)
	{
		FVector TrailColor = FVector(ActiveWeaponTemplate->GetTrailColor());
		FVector TrailAnimLight = FVector(ActiveWeaponTemplate->GetTrailAnimLight());

		for (const FSoWeaponBoost& WeaponBoost : Orb->WeaponBoosts)
			if (WeaponBoost.WeaponTemplate == ActiveWeaponTemplate)
			{
				TrailColor = FVector(WeaponBoost.TrailColor);
				TrailAnimLight = FVector(WeaponBoost.TrailAnimLight);
			}
		TrailComponent->SetVectorParameterValueOnMaterials(ColorValueName, TrailColor);
		TrailComponent->SetVectorParameterValueOnMaterials(AnimValueName, TrailAnimLight);
	}
	TrailMeshes.Push(TrailComponent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::ApplyDamage(const struct FSoStrikeEntry& StrikeData, UStaticMeshComponent* TrailComponent, float WallHitThreshold, int32 StrikeIndex)
{
	if (TrailComponent == nullptr)
		return;

	auto& ComponentsAlreadyHit = StrikeIndex == 0 ? ComponentsAlreadyHitA : ComponentsAlreadyHitB;

	TrailComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	FComponentQueryParams Param = FComponentQueryParams::DefaultComponentQueryParams;
	Param.AddIgnoredActor(Orb);
	Param.bIgnoreTouches = true;
	TArray<struct FOverlapResult> OutOverlaps;
	TrailComponent->ComponentOverlapMulti(	OutOverlaps,
											Orb->GetWorld(),
											TrailComponent->GetComponentLocation(),
											TrailComponent->GetComponentTransform().GetRotation(),
											ECollisionChannel::ECC_PhysicsBody,
											Param,
											FCollisionObjectQueryParams::DefaultObjectQueryParam);
	TrailComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const FVector AttackSourceLocation = Orb->GetMesh()->GetSocketLocation(FName("ItemAttachPoint"));

	TMap<AActor*, FSoMeleeHitParam> HitList;
	TSet<UPrimitiveComponent*> EnvironmentHits;

	for (const FOverlapResult& Overlap : OutOverlaps)
	{
		if (UPrimitiveComponent* Component = Overlap.Component.Get())
		{
			if (ComponentsAlreadyHit.Contains(Component))
				continue;

			ComponentsAlreadyHit.Add(Component);
			auto SpawnEnvironmentHitVFX = [this, &EnvironmentHits, Component, &AttackSourceLocation, &Overlap, &WallHitThreshold](bool bIgnoreWallHitThreshold)
			{
				if (!EnvironmentHits.Contains(Component) &&
					(Component->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic ||
						Component->GetCollisionObjectType() == ECollisionChannel::ECC_WorldDynamic))
				{
					FVector HitLocation;
					const FName BoneName = USoStaticHelper::GetBoneNameFromBodyIndex(Cast<USkeletalMeshComponent>(Component), Overlap.ItemIndex);
					const float Distance = Component->GetClosestPointOnCollision(AttackSourceLocation, HitLocation, BoneName);
					if (Distance > KINDA_SMALL_NUMBER)
					{
						EnvironmentHits.Add(Component);
						if (bIgnoreWallHitThreshold || (AttackSourceLocation.Z + WallHitThreshold < HitLocation.Z))
						{
							FHitResult Hit;
							Component->LineTraceComponent(Hit,
								AttackSourceLocation,
								AttackSourceLocation + (HitLocation - AttackSourceLocation) * 2,
								FCollisionQueryParams::DefaultQueryParam);

							UPhysicalMaterial* PhyMat = Hit.PhysMaterial.Get();

							if (PhyMat == nullptr)
								PhyMat = Component->BodyInstance.GetSimplePhysicalMaterial();

							//DrawDebugLine(Orb->GetWorld(), AttackSourceLocation,
							//	AttackSourceLocation + (HitLocation - AttackSourceLocation) * 2,
							//	FColor::Red, false, 1.0f, 0, 12.333);

							UParticleSystem* WallHitVFX = USoGameSingleton::GetWallHitVFXFromPhysicalMaterial(PhyMat);
							UGameplayStatics::SpawnEmitterAtLocation(Orb->GetWorld(), WallHitVFX, HitLocation, FRotator::ZeroRotator, true);

							UFMODEvent* WallHitSFX = USoGameSingleton::GetWallHitSFXFromPhysicalMaterial(PhyMat);
							USoAudioManager::PlaySoundAtLocation(Orb, WallHitSFX, FTransform(HitLocation));
						}
					}
				}
			};

			AActor* Actor = Overlap.Actor.Get();
			if (Actor->GetClass()->ImplementsInterface(USoMortal::StaticClass()))
			{
				// only hit mortals if the component's collision is ECC_PhysicsBody
				if ((Component->GetCollisionObjectType() == ECollisionChannel::ECC_PhysicsBody || Actor->ActorHasTag("PlayerCanHitAnyComponent")) &&
					ISoMortal::Execute_IsAlive(Actor))
				{
					FSoMeleeHitParam* HitParamPtr = HitList.Find(Actor);
					if (HitParamPtr == nullptr)
					{

						const bool bCritical = FMath::RandRange(0.0f, 1.0f) < Orb->SoPlayerCharacterSheet->GetCriticalHitChance();
						const float CriticalMultiplier = bCritical ? ActiveWeaponTemplate->GetCriticalMultiplier() : 1.0f;

						HitParamPtr = &HitList.Add(Actor);
						HitParamPtr->Dmg = ActiveWeaponTemplate->GetDmg(bSpecialStrikeWasExecutedLast) *
											(FMath::Clamp(4.0f * LandingAmount / 1000.0f, 1.0f, 10.0f) * CriticalMultiplier) *
											Orb->SoPlayerCharacterSheet->GetDamageModifiers();

						for (const FSoWeaponBoost& WeaponBoost : Orb->WeaponBoosts)
							if (WeaponBoost.WeaponTemplate == ActiveWeaponTemplate)
							{
								HitParamPtr->Dmg += WeaponBoost.BonusDamage;
								HitParamPtr->ImpactFX = WeaponBoost.ImpactFX;
							}

						if (bDmgBoostCheat)
							HitParamPtr->Dmg *= 1000;

						HitParamPtr->bBlockable = false;
						HitParamPtr->bCritical = bCritical;
						HitParamPtr->HitReactDesc.AssociatedActor = Orb;
						HitParamPtr->HitReactDesc.Irresistibility = ActiveWeaponTemplate->GetInterruptModifier() + 1;
						HitParamPtr->Effects = StrikeData.HitEffects;
					}
					HitParamPtr->HitReactDesc.AssociatedBones.Add(USoStaticHelper::GetBoneNameFromBodyIndex(Cast<USkeletalMeshComponent>(Overlap.Component.Get()),
																	Overlap.ItemIndex));
				}
				else if (Component->ComponentHasTag(FName("SpawnVFXOnHit")))
					SpawnEnvironmentHitVFX(true);
			}
			else
				SpawnEnvironmentHitVFX(false);
		}
	}
	for (const auto& Pair : HitList)
	{
		Orb->OnMeleeHit.Broadcast(Pair.Key, Pair.Value);
		ISoMortal::Execute_MeleeHit(Pair.Key, Pair.Value);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::StrikePressed()
{
	//if (bStrikePressedRecently == true)
	//	bBanStrikeCauseSpamming = true;
	bStrikePressedRecently = true;
	Orb->gTimerManager->SetTimer(
		StrikePressedRecentlyTimer,
		this, &ThisClass::StrikePressedRecentlyTimeOver,
		GetStrikePressedRecentlyTimeOffset(),
		false
	);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void USoAStrike::StrikePressedRecentlyTimeOver()
{
	bStrikePressedRecently = false;
	bBanStrikeCauseSpamming = false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::OnBlocked()
{
	// SwitchActivity(Orb->SoAWeaponInArm);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAStrike::CanBeInterrupted(EActivity DesiredState) const
{
	return (!bIsInStrike) && (!bWaitBeforeStrike);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::OnLanded()
{
	USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXArmedLand, FTransform(Orb->GetActorTransform()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAStrike::CanStrike(bool bSpecial, bool bSpawnSFXIfNot)
{
	const FSoItem& Item = Orb->SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	if (USoWeaponTemplate* WeaponTemplate = Cast<USoWeaponTemplate>(Item.Template))
	{
		USoCharacterStrike* Strike = WeaponTemplate->GetStrike(bSpecial);
		if (Strike != nullptr)
		{
			for (int32 i = 0; i < Orb->Cooldowns.Num(); ++i)
				if (Orb->Cooldowns[i].Object == Strike)
				{
					Orb->CooldownBlocksEvent.Broadcast(i, Orb->Cooldowns[i].Counter, Orb->Cooldowns[i].Object);
					if (bSpawnSFXIfNot)
						USoAudioManager::PlaySoundAtLocation(Orb, Orb->SFXFCanNot, {});

					return false;
				}

			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAStrike::HasStrikeCooldown(bool bSpecial) const
{
	const FSoItem& Item = Orb->SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	if (USoWeaponTemplate* WeaponTemplate = Cast<USoWeaponTemplate>(Item.Template))
	{
		USoCharacterStrike* Strike = WeaponTemplate->GetStrike(bSpecial);
		return (Strike == nullptr || Strike->GetCooldownTime() > 0.0f);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAStrike::UnhideWeapon()
{
	Orb->SoSword->SetVisibility(true);
	const FSoItem& Item = Orb->SoPlayerCharacterSheet->GetEquippedItem(ESoItemSlot::EIS_Weapon0);
	if (USoWeaponTemplate* Template = Cast<USoWeaponTemplate>(Item.Template))
		if (Template->GetWeaponType() == ESoWeaponType::EWT_DualWield)
			Orb->SoOffHandWeapon->SetVisibility(true, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAStrike::AutoAim()
{
	const FVector CharLocation = Orb->GetActorLocation();
	TArray<ASoEnemy*> EnemiesClose;

	for (ASoEnemy* Enemy : ASoGameMode::Get(Orb).GetEnemies())
		if (Enemy->IsAlive() &&
			Enemy->IsAutoAimSupported() &&
			(CharLocation - Enemy->GetActorLocation()).SizeSquared() < ActiveAutoAimDistance * ActiveAutoAimDistance)
			EnemiesClose.Add(Enemy);

	if (EnemiesClose.Num() == 0)
	{
		if (WouldHitMortals())
			return true;

		Orb->SetActorRotation((-Orb->GetActorForwardVector()).Rotation());
		if (ActiveTrailMesh)
			ActiveTrailMesh->SetActorRotation(Orb->GetMesh()->GetComponentRotation());
		if (WouldHitMortals())
		{
			UpdateCamera(0.01f);
			Orb->StoredForwardVector = Orb->GetActorForwardVector();
			Orb->LastInputDirection = Orb->GetActorForwardVector();

			// UE_LOG(LogTemp, Warning, TEXT("Rotated cause would hit mortal!"));
			return true;
		}

		Orb->SetActorRotation((-Orb->GetActorForwardVector()).Rotation());
		if (ActiveTrailMesh)
			ActiveTrailMesh->SetActorRotation(Orb->GetMesh()->GetComponentRotation());

		return false;
	}

	const bool bWouldHitMortals = WouldHitMortals();
	bool bAnyEnemyBehind = false;
	for (ASoEnemy* Enemy : EnemiesClose)
	{
		const FVector FaceDir = (Enemy->GetActorLocation() - CharLocation).GetSafeNormal();
		if ((FaceDir | Orb->GetActorForwardVector()) > 0.0f)
		{
			if (bWouldHitMortals)
				return true;
		}
		else
			bAnyEnemyBehind = true;
	}

	if (bAnyEnemyBehind)
	{
		const FVector FaceDir = Orb->SoMovement->GetSplineLocation().GetDirectionFromVector(EnemiesClose[0]->GetActorLocation() - CharLocation);
		if ((FaceDir | Orb->GetActorForwardVector()) < 0.0f)
		{
			Orb->SetActorRotation(FaceDir.Rotation());
			UpdateCamera(0.01f);
			Orb->StoredForwardVector = Orb->GetActorForwardVector();
			Orb->LastInputDirection = Orb->GetActorForwardVector();
		}

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAStrike::WouldHitMortals()
{
	if (ActiveTrailMesh == nullptr)
	{
		USkeletalMeshComponent* CharMesh = Orb->GetMesh();
		if (Orb->WeaponTrailMeshes.Num() == 0)
		{
			ActiveTrailMesh = Orb->GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), CharMesh->GetComponentTransform());
			ActiveTrailMesh->SetMobility(EComponentMobility::Movable);
			ActiveTrailMesh->GetStaticMeshComponent()->SetVisibility(false);
			Orb->WeaponTrailMeshes.Push(ActiveTrailMesh);
		}
		else
		{
			ActiveTrailMesh = Orb->WeaponTrailMeshes.Last();
			ActiveTrailMesh->SetActorTransform(CharMesh->GetComponentTransform());
		}
		UStaticMeshComponent* TrailComponent = ActiveTrailMesh->GetStaticMeshComponent();
		TrailComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (ActiveStrikeData != nullptr && ActiveStrikeData->StrikeList.Num() > 0)
		{
			TrailComponent->SetStaticMesh(ActiveStrikeData->StrikeList[0].TrailMesh);
			if (ActiveStrikeData->StrikeList[0].bAttachToCharacter)
			{
				ActiveTrailMesh->AttachToComponent(Orb->GetMesh(), FAttachmentTransformRules::KeepWorldTransform);
				ActiveTrailMesh->GetRootComponent()->bAbsoluteRotation = true;
			}
		}
		ActiveTrailMesh->SetActorRotation(CharMesh->GetComponentRotation());
	}

	UStaticMeshComponent* ActiveTrailComponent = ActiveTrailMesh->GetStaticMeshComponent();

	ActiveTrailComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	FComponentQueryParams Param = FComponentQueryParams::DefaultComponentQueryParams;
	Param.AddIgnoredActor(Orb);
	Param.bIgnoreTouches = true;
	TArray<struct FOverlapResult> OutOverlaps;
	ActiveTrailComponent->ComponentOverlapMulti(OutOverlaps,
										  Orb->GetWorld(),
										  ActiveTrailComponent->GetComponentLocation(),
										  ActiveTrailComponent->GetComponentTransform().GetRotation(),
										  ECollisionChannel::ECC_PhysicsBody,
										  Param,
										  FCollisionObjectQueryParams::DefaultObjectQueryParam);
	ActiveTrailComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (const FOverlapResult& Overlap : OutOverlaps)
	{
		if (UPrimitiveComponent* Component = Overlap.Component.Get())
		{
			AActor* Actor = Overlap.Actor.Get();
			if ((Component->GetCollisionObjectType() == ECollisionChannel::ECC_PhysicsBody || Cast<ASoProjectile>(Actor) != nullptr) &&
				Actor->GetClass()->ImplementsInterface(USoMortal::StaticClass()) &&
				ISoMortal::Execute_IsAlive(Actor))
			{
				return true;
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoAStrike::GetStrikePressedRecentlyTimeOffset() const
{
	return USoDateTimeHelper::NormalizeTime(StrikePressedRecentlyTimeOffset);
}
