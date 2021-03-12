// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoCarryable.h"

#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
// #include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Particles/ParticleSystemComponent.h"

#include "Online/Achievements/SoAchievementManager.h"
#include "SplineLogic/SoSplinePoint.h"
#include "Character/SoCharacter.h"
#include "Enemy/SoEnemy.h"
#include "SaveFiles/SoWorldState.h"
#include "Kinematic/Tasks/SoKTask.h"
#include "Basic/SoGameMode.h"
#include "Basic/SoAudioManager.h"
#include "Character/SoCharStates/SoActivity.h"
#include "Basic/Helpers/SoStaticHelper.h"

#include "SoLocalization.h"
#include "Basic/Helpers/SoDateTimeHelper.h"

const FName ASoCarryable::StaticMeshSpeedName = FName("Speed");
const float ASoCarryable::StaticMeshAnimSpeedValue = 2.0f;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoCarryable::ASoCarryable()
{
	PrimaryActorTick.bCanEverTick = true;

	SetActorTickEnabled(false);
	SoBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SoBox"));
	SoBox->OnComponentBeginOverlap.AddDynamic(this, &ASoCarryable::OnBoxOverlapBegin);
	RootComponent = SoBox;

	// SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	// SkeletalMesh->SetupAttachment(RootComponent);

	Collision->SetupAttachment(RootComponent);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);

	MagicBoxAnimationSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("MagicBoxAnimationSFX"));
	MagicBoxAnimationSFX->SetupAttachment(RootComponent);
	MagicBoxAnimationSFX->SetupAttachment(RootComponent);


	OnHitParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("OnHitParticle"));
	OnHitParticle->SetupAttachment(RootComponent);

	OnHitParticleSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("OnHitParticleSFX"));
	OnHitParticleSFX->SetupAttachment(RootComponent);
	OnHitParticleSFX->bAutoActivate = false;

	SoPlayerBlockArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SoPlayerBlockBox"));
	SoPlayerBlockArea->SetupAttachment(RootComponent);

	Damage.Physical = 20.0f;

	HitReactDesc.HitReact = ESoHitReactType::EHR_BreakIntoPieces;

	SoTask = CreateDefaultSubobject<USoKTTranslateOnSpline>(TEXT("SoSplineTask"));
	InitTask();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	HitReactDesc.AssociatedActor = SoRematerializePoint;

	if (!bTransformInitialized)
	{
		bTransformInitialized = true;
		SavedActorTransform = GetActorTransform();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::BeginPlay()
{
	Super::BeginPlay();

	if (SoTask == nullptr)
	{
		SoTask = NewObject<USoKTTranslateOnSpline>(this);
		InitTask();
	}

	SoTask->Initialize(SoBox, HitReactDesc.AssociatedActor, 2);
	USoEventHandlerHelper::SubscribeToSoPostLoad(this);

	SoBox->OnComponentHit.AddDynamic(this, &ASoCarryable::OnHitCallback);
	SoPlayerBlockArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::InitTask()
{
	SoTask->bVelocityBased = true;
	SoTask->Value = FVector2D(0.0f, 0.0f);
	SoTask->CollisionType = ESoTaskCollisionResponse::ETCR_PushOrWait;
	SoTask->TimeInSec = 3600 * 24 * 365;
	SoTask->CurrentTime = 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SoBox->OnComponentHit.RemoveDynamic(this, &ASoCarryable::OnHitCallback);

	Super::EndPlay(EndPlayReason);

	USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);

	GetWorld()->GetTimerManager().ClearTimer(PlayerBlockTimer);

	if (PlacedTime > KINDA_SMALL_NUMBER)
		HandleSoPostLoad_Implementation();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::Tick(float DeltaSeconds)
{
	const FVector OldLocation = GetActorLocation();
	SoTask->Value = SoTask->Value + ForceFieldVelocity.GetSafeNormal() * Acceleration * DeltaSeconds;

	if (SoTask->Value.Size() > ForceFieldVelocity.Size())
		SoTask->Value = ForceFieldVelocity;

	SoTask->Execute(DeltaSeconds, true);

	// can leave forcefield in execute
	if (bInForceField)
		Velocity = (GetActorLocation() - OldLocation) / DeltaSeconds;

	SetActorRotation(SoTask->SplineLocation.GetDirection().Rotation());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::Interact_Implementation(ASoCharacter* Character)
{
	StopMovement();
	if (ActiveState == ESoCarryableState::ECS_Idle || ActiveState == ESoCarryableState::ECS_WaitForSignal)
		CarryStarted();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::Trigger_Implementation(const FSoTriggerData& TriggerData)
{
	if (ActiveState == ESoCarryableState::ECS_WaitForSignal)
	{
		ActiveState = ESoCarryableState::ECS_Idle;
		OnGoHomeStart();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::HandleSoPostLoad_Implementation()
{
	SoBox->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	ActiveState = ESoCarryableState::ECS_Idle;
	SwitchToSolid();
	SetActorTransform(SavedActorTransform);

	GetWorld()->GetTimerManager().ClearTimer(GoHomeStartTimer);
	GetWorld()->GetTimerManager().ClearTimer(GoHomeEndTimer);
	GetWorld()->GetTimerManager().ClearTimer(DroppedAndHitProtectedTimer);
	GetWorld()->GetTimerManager().ClearTimer(PlayerBlockTimer);
	SoPlayerBlockArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OnReturned.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::OnBoxOverlapBegin(UPrimitiveComponent* OverlappedComponent,
									AActor* OtherActor,
									UPrimitiveComponent* OtherComp,
									int32 OtherBodyIndex,
									bool bFromSweep,
									const FHitResult& SweepResult)
{
	if (OverlappedComponent == Collision)
	{
		ASoInteractableActor::OnOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
		return;
	}

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (OtherActor != Character && ActiveState == ESoCarryableState::ECS_DropInProgress && OtherComp->GetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn) == ECollisionResponse::ECR_Block)
	{
		Character->SoActivity->OnAnimEvent(EAnimEvents::EAE_OnReleaseCarriedStuffInterrupted);
		SoBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (Cast<ASoEnemy>(OtherActor) != nullptr && (ActiveState == ESoCarryableState::ECS_Idle || ActiveState == ESoCarryableState::ECS_WaitForSignal))
		OnGoHomeStart();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::OnHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ActiveState == ESoCarryableState::ECS_WaitForSignal &&
		((Behavior == ESoCarryableBehavior::ECB_ReturnAfterCharacterHit && Cast<ASoCharacter>(OtherActor) != nullptr)
		|| Cast<ASoEnemy>(OtherActor) != nullptr) &&
		// Threshold
		GetWorld()->GetTimeSeconds() > DroppedTime + GetReturnProtectionTime())
	{
		ActiveState = ESoCarryableState::ECS_Idle;
		OnHitParticle->Activate(true);
		OnHitParticleSFX->Activate(true);
		OnGoHomeStart();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::SwitchToSolid()
{
	SoBox->SetCollisionProfileName(FName("BlockAllDynamic"));
	SoBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// SkeletalMesh->SetPlayRate(1.0f);
	StaticMesh->SetScalarParameterValueOnMaterials(StaticMeshSpeedName, StaticMeshAnimSpeedValue);
	MagicBoxAnimationSFX->Play();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::SwitchToOverlap()
{
	SoBox->SetCollisionProfileName(FName("OverlapAllDynamic"));
	SoBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::StopMovement()
{
	SetActorTickEnabled(false);
	Velocity = FVector(0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::OnGoHomeStart()
{
	StopMovement();

	if (ActiveState != ESoCarryableState::ECS_PickedUp)
	{
		ASoInteractableActor::Deactivate();
		SoBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (SoPlayerBlockArea != nullptr)
			SoPlayerBlockArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MagicBoxAnimationSFX->Stop();

		// SkeletalMesh->SetVisibility(false);
		// SkeletalMesh->SetPlayRate(1.0f);

		StaticMesh->SetVisibility(false);
		StaticMesh->SetScalarParameterValueOnMaterials(StaticMeshSpeedName, StaticMeshAnimSpeedValue);


		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (Character != nullptr)
			Character->SetBase(nullptr);

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ReturnVFX, GetActorLocation(), GetActorRotation(), true, EPSCPoolMethod::AutoRelease);

		GetWorld()->GetTimerManager().SetTimer(
			GoHomeEndTimer,
			this,
			&ThisClass::OnGoHomeEnd,
			GetReturnTime()
		);
		USoAudioManager::PlaySoundAtLocation(this, SFXOnGoHomeStart, GetActorTransform());

		// move home here to avoid weird collision issue?!
		if (Behavior != ESoCarryableBehavior::ECB_ReturnAfterCharacterHit)
			SetActorTransform(SavedActorTransform);

		OnReturnStart.Broadcast();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::OnGoHomeEnd()
{
	StopMovement();

	SetActorTransform(SavedActorTransform);

	ASoInteractableActor::Activate();
	SwitchToOverlap();
	// SkeletalMesh->SetVisibility(true);
	StaticMesh->SetVisibility(true);
	MagicBoxAnimationSFX->Play();

	SoBox->UpdateOverlaps();
	TArray<AActor*> OverlappingActors;
	SoBox->GetOverlappingActors(OverlappingActors, ASoCharacterBase::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		ISoMortal::Execute_CauseDmg(Actor, Damage, HitReactDesc);
		if (Actor == USoStaticHelper::GetPlayerCharacterAsActor(this))
		{
			static const FName SuicideACName = FName("A_SuicideSQuad");
			USoAchievementManager::Get(this).UnlockAchievement(this, SuicideACName);
		}
	}

	SwitchToSolid();
	USoAudioManager::PlaySoundAtLocation(this, SFXOnGoHomeEnd, GetActorTransform());

	OnReturned.Broadcast();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::CarryStarted()
{
	DisplayText = FROM_STRING_TABLE_INTERACTION("drop");
	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);

	SoBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SoBox->SetWorldRotation(ISoSplineWalker::Execute_GetSplineLocationI(Character).GetDirection().Rotation());
	ActiveState = ESoCarryableState::ECS_PickedUp;

	Character->StartCarryStuff(this);
	// SkeletalMesh->SetPlayRate(0.0f);
	StaticMesh->SetScalarParameterValueOnMaterials(StaticMeshSpeedName, 0.0f);
	MagicBoxAnimationSFX->Stop();

	OnTaken.Broadcast();

	USoEventHandlerHelper::SubscribeToPlayerRematerialize(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::Dropped(bool bActivatePlayerBlock)
{
	USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(this);

	DisplayText = FROM_STRING_TABLE_INTERACTION("pick_up");

	SoBox->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	if (bActivatePlayerBlock)
	{
		SoPlayerBlockArea->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetWorld()->GetTimerManager().SetTimer(PlayerBlockTimer, this, &ASoCarryable::OnCharBlockTimeOver, 0.4);
	}


	//TArray<AActor*> OverlappedActors;
	//SoBox->GetOverlappingActors(OverlappedActors);
	//for (AActor* Actor : OverlappedActors)
	//	if (Cast<ASoEnemy>(Actor))
	//	{
	//		OnGoHomeStart();
	//		return;
	//	}

	SwitchToSolid();
	if (Behavior == ESoCarryableBehavior::ECB_ReturnAfterTime)
	{
		if (PlacedTime > KINDA_SMALL_NUMBER)
		{
			GetWorld()->GetTimerManager().SetTimer(GoHomeStartTimer, this, &ASoCarryable::OnGoHomeStart, PlacedTime);
			ActiveState = ESoCarryableState::ECS_Idle;
		}
		else
		{
			OnGoHomeStart();
			return;
		}
	}
	else
		ActiveState = ESoCarryableState::ECS_WaitForSignal;

	OnPlaced.Broadcast();

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	FSoSplinePoint SplineLocation = ISoSplineWalker::Execute_GetSplineLocationI(Character);
	SetActorRotation(SplineLocation.GetDirection().Rotation());

	if (bInForceField && Behavior != ESoCarryableBehavior::ECB_IgnoreForceField)
	{
		SoTask->Value = ForceFieldVelocity.GetSafeNormal() * 100.0f;
		SetActorTickEnabled(true);

		const FVector Delta = GetActorLocation() - Character->GetActorLocation();
		SplineLocation += Delta.Size2D() * SplineLocation.GetDirectionModifierFromVector(Delta);
		SplineLocation.SetReferenceActor(this);
		SoTask->Start(SoBox, true, SplineLocation);
	}

	DroppedTime = GetWorld()->GetTimeSeconds();

	if (Behavior == ESoCarryableBehavior::ECB_ReturnAfterCharacterHit)
		GetWorld()->GetTimerManager().SetTimer(
			DroppedAndHitProtectedTimer,
			this,
			&ThisClass::OnCharHitProtectionTimeOver,
			GetReturnProtectionTime()
		);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::OnCharHitProtectionTimeOver()
{
	if (ActiveState == ESoCarryableState::ECS_WaitForSignal)
	{
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		if (Character->GetMovementBaseActor(Character) == this)
		{
			Character->SetBase(nullptr);
			ActiveState = ESoCarryableState::ECS_Idle;
			OnGoHomeStart();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::OnCharBlockTimeOver()
{
	if (SoPlayerBlockArea != nullptr)
		SoPlayerBlockArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCarryable::GetReturnProtectionTime() const
{
	return ReturnProtectionTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoCarryable::CancelDrop()
{
	if (ActiveState == ESoCarryableState::ECS_DropInProgress)
	{
		ActiveState = ESoCarryableState::ECS_PickedUp;
		SoBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
		SoBox->SetWorldRotation(ISoSplineWalker::Execute_GetSplineLocationI(Character).GetDirection().Rotation());
		ActiveState = ESoCarryableState::ECS_PickedUp;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ASoCarryable::GetReturnTime() const
{
	return USoDateTimeHelper::NormalizeTime(ReturnTime);
}
