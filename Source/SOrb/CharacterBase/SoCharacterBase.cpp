// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoCharacterBase.h"

#include "Effects/SoEffectHandlerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Effects/SoEffectBase.h"
#include "SoCharacterMovementComponent.h"
#include "SoCharacterSheet.h"
#include "Items/SoItem.h"
#include "Basic/SoAudioManager.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Projectiles/SoProjectileSpawnerComponent.h"
#include "SplineLogic/SoSplinePoint.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoCharacterBase, All, All)


FName ASoCharacterBase::CharacterSheetName(TEXT("SoCharacterSheet"));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoCharacterBase::ASoCharacterBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<USoCharacterMovementComponent>(CharacterMovementComponentName))
{
	SoMovement = Cast<USoCharacterMovementComponent>(GetCharacterMovement());
	SoMovement->bOrientRotationToMovement = true;

	SoCharacterSheet = CreateDefaultSubobject<USoCharacterSheet>(CharacterSheetName);
	SoEffectHandler = ObjectInitializer.CreateDefaultSubobject<USoEffectHandlerComponent>(this, TEXT("SoEffectHandler"));

	ProjectileSpawner = CreateDefaultSubobject<USoProjectileSpawnerComponent>(TEXT("SoProjectileSpawner"));
	ProjectileSpawner->SetupAttachment(RootComponent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::Tick(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Base - Tick"), STAT_Base_Tick, STATGROUP_SoCharacter);

	Super::Tick(DeltaSeconds);
	UpdateMaterialAnimations(DeltaSeconds);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacterBase::NeedsHeal_Implementation() const
{
	return SoCharacterSheet->GetHealth() < SoCharacterSheet->GetMaxHealth();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::Heal_Implementation(bool bAll, float Delta)
{
	if (bAll)
		SoCharacterSheet->RestoreHealth();
	else
		SoCharacterSheet->IncreaseHealth(Delta);

	USoAudioManager::PlaySoundAtLocation(this, SFXFOnHeal, GetActorTransform());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::ApplyEffect_Implementation(TSubclassOf<USoEffectBase> EffectClass, bool bApply)
{
	if (bApply)
		SoEffectHandler->AddEffect(EffectClass);
	else
		SoEffectHandler->RemoveEffect(EffectClass);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacterBase::SubscribeOnDied_Implementation(const FSoOnMortalNoParam& OnDeath, bool bSubscribe)
{
	if (bSubscribe)
		OnMortalDeath.Add(OnDeath);
	else
		OnMortalDeath.Remove(OnDeath);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacterBase::SubscribeOnSpellsReset_Implementation(const FSoOnMortalNoParam& OnSpellsReset, bool bSubscribe)
{
	if (bSubscribe)
		OnSpellsReseted.Add(OnSpellsReset);
	else
		OnSpellsReseted.Remove(OnSpellsReset);

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacterBase::SubscribeOnMeleeHit_Implementation(const FSoOnMeleeHit& InOnMeleeHit, bool bSubscribe)
{
	if (bSubscribe)
		OnMeleeHit.Add(InOnMeleeHit);
	else
		OnMeleeHit.Remove(InOnMeleeHit);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacterBase::SubscribeOnMeleeHitTaken_Implementation(const FSoOnMeleeHit& InOnMeleeHit, bool bSubscribe)
{
	if (bSubscribe)
		OnMeleeHitSuffered.Add(InOnMeleeHit);
	else
		OnMeleeHitSuffered.Remove(InOnMeleeHit);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ASoCharacterBase::SubscribeOnHitTaken_Implementation(const FSoOnHit& OnHit, bool bSubscribe)
{
	if (bSubscribe)
		OnHitSuffered.Add(OnHit);
	else
		OnHitSuffered.Remove(OnHit);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::SetRootMotionDesc(const FSoRootMotionDesc& Desc)
{
	SoMovement->SetRootMotionDesc(Desc);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::ClearRootMotionDesc()
{
	SoMovement->ClearRootMotionDesc();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::ChangeSpline(ASoPlayerSpline* NewSpline)
{
	FSoSplinePoint SplineLoc;
	SplineLoc.SetSpline(NewSpline);
	SplineLoc.SetDistanceFromWorldLocation(GetActorLocation());
	SoMovement->SetSplineLocation(SplineLoc);

	// move character there, otherwise he would get crazy velocity next frame
	FVector NewPos = SplineLoc;
	NewPos.Z = GetActorLocation().Z;
	SetActorLocation(NewPos, true, nullptr, ETeleportType::TeleportPhysics);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoSplinePoint ASoCharacterBase::GetSplineLocationI_Implementation() const
{
	return SoMovement ? SoMovement->GetSplineLocation() : FSoSplinePoint{};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::SetSplineLocation_Implementation(const FSoSplinePoint& SplinePoint, bool bUpdateOrientation)
{
	SoMovement->SetSplineLocation(SplinePoint);
	if (bUpdateOrientation)
		SetActorRotation(SplinePoint.GetDirectionFromVector(GetActorForwardVector()).Rotation());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::OnStatusEffectChanged_Implementation(ESoStatusEffect Effect, bool bGained)
{
	OnStatusEffect(Effect, bGained);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::DisplayVisualEffect_Implementation(ESoVisualEffect Effect)
{
	OnDisplayVisualEffect(Effect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector ASoCharacterBase::GetEffectAttachLocation_Implementation()
{
	return GetActorLocation();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::ApplyStaticEffects(const FSoItem& Item, bool bApply)
{
	TArray<TSubclassOf<USoEffectBase>> Effects;
	Item.GetStaticEffects(Effects);
	for (TSubclassOf<USoEffectBase>& Effect : Effects)
		ISoMortal::Execute_ApplyEffect(this, Effect, bApply);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::PlayMaterialAnimation(int32 Index, bool bForward, float Speed)
{
	if (MaterialAnimations.IsValidIndex(Index))
	{
		MaterialAnimations[Index].Counter = 0.0f;
		MaterialAnimations[Index].bPlayForward = bForward;
		MaterialAnimations[Index].PlaySpeed = Speed;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::StopMaterialAnimation(int32 Index)
{
	if (MaterialAnimations.IsValidIndex(Index) && MaterialAnimations[Index].Counter < MaterialAnimations[Index].Duration)
	{
		MaterialAnimations[Index].Counter = MaterialAnimations[Index].Duration + 0.1f;
		OnMaterialAnimationInterrupted(Index);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::StopAllMaterialAnimations()
{
	for (int32 i = 0; i < MaterialAnimations.Num(); ++i)
		StopMaterialAnimation(i);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::SetScalarOnParticleMaterials(UParticleSystemComponent* PS, FName ParamName, float Value)
{
	if (PS == nullptr)
		return;

	for (UMaterialInterface* MaterialInterface : PS->EmitterMaterials)
		if (UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface))
			DynamicMaterial->SetScalarParameterValue(ParamName, Value);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCharacterBase::UpdateMaterialAnimations(float DeltaSeconds)
{
	for (int32 MatAnimIndex = 0; MatAnimIndex < MaterialAnimations.Num(); ++MatAnimIndex)
	{
		FSoMaterialAnimationEntry& Entry = MaterialAnimations[MatAnimIndex];
		if (Entry.Counter < Entry.Duration && (Entry.ScalarCurve != nullptr || Entry.ColorCurve))
		{
			Entry.Counter += DeltaSeconds * Entry.PlaySpeed;
			const float Progress = FMath::Clamp(Entry.Counter / Entry.Duration, 0.0f, 1.0f);
			const float Percent = Entry.bPlayForward ? Progress : 1.0f - Progress;
			auto ApplyOnMaterialInstance = [&Entry, &Percent](UMaterialInstanceDynamic* Instance)
			{
				if (Instance != nullptr)
				{
					if (Entry.ScalarCurve != nullptr)
						Instance->SetScalarParameterValue(Entry.MaterialParameterName, Entry.ScalarCurve->GetFloatValue(Percent));
					else
						Instance->SetVectorParameterValue(Entry.MaterialParameterName, Entry.ColorCurve->GetLinearColorValue(Percent));
				}
				else
					UE_LOG(LogSoCharacterBase, Warning, TEXT("[ASoCharacterBase]: failed to set param in material: invalid pointer (maybe not dynamic material?)"));
			};

			if (Entry.TargetOverridePS.Num() > 0)
			{
				auto ApplyOnMaterial = [&ApplyOnMaterialInstance](UParticleSystemComponent* PS, int Index)
				{
					if (PS->EmitterMaterials.IsValidIndex(Index))
						ApplyOnMaterialInstance(Cast<UMaterialInstanceDynamic>(PS->EmitterMaterials[Index]));
				};

				for (UParticleSystemComponent* PS : Entry.TargetOverridePS)
				{
					if (Entry.MaterialIndexList.Num() == 0)
						for (int32 i = 0; i < PS->EmitterMaterials.Num(); ++i)
							ApplyOnMaterial(PS, i);
					else
						for (int32 MaterialIndex : Entry.MaterialIndexList)
							ApplyOnMaterial(PS, MaterialIndex);
				}
			}
			else
			{
				if (Entry.MaterialIndexList.Num() == 0)
				{
					if (Entry.ScalarCurve != nullptr)
						GetMesh()->SetScalarParameterValueOnMaterials(Entry.MaterialParameterName, Entry.ScalarCurve->GetFloatValue(Percent));
					else
					{
						const FLinearColor C = Entry.ColorCurve->GetLinearColorValue(Percent);
						GetMesh()->SetVectorParameterValueOnMaterials(Entry.MaterialParameterName, FVector(C.R, C.G, C.B));
					}
				}
				else
				{
					for (int32 MaterialIndex : Entry.MaterialIndexList)
					{
						UMaterialInterface* MaterialInterface = GetMesh()->GetMaterial(MaterialIndex);
						if (MaterialInterface)
						{
							UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
							if (DynamicMaterial == nullptr)
								DynamicMaterial = GetMesh()->CreateAndSetMaterialInstanceDynamic(MaterialIndex);

							ApplyOnMaterialInstance(DynamicMaterial);
						}
					}
				}
			}

			if (Entry.Counter >= Entry.Duration)
			{
				OnMaterialAnimationFinished(MatAnimIndex);
			}
		}
	}

}
