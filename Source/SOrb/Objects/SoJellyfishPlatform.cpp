// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoJellyfishPlatform.h"

#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "FMODAudioComponent.h"


const FName ASoJellyfishPlatform::BaseOPName = "BaseOP";
const FName ASoJellyfishPlatform::OPAddName = "OP_Add";
const float ASoJellyfishPlatform::BaseOpMin = -1.2f;
const float ASoJellyfishPlatform::BaseOpMax = 0.5f;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoJellyfishPlatform::ASoJellyfishPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	RootComponent = Collision;
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);

	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Particles"));
	ParticleComponent->SetupAttachment(RootComponent);

	HitSFX = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("HitSFX"));
	HitSFX->bAutoActivate = false;
	HitSFX->SetupAttachment(RootComponent);

	Tags.Add(FName("Trampoline"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::BeginPlay()
{
	Super::BeginPlay();
	Collision->OnComponentHit.AddDynamic(this, &ASoJellyfishPlatform::OnHitCallback);

	if (!bActiveByDefault)
	{
		ActiveState = ESoJellyfishState::EJS_Inactive;
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMesh->SetVisibility(false);
		SkeletalMesh->SetScalarParameterValueOnMaterials(OPAddName, 0.0f);
		SkeletalMesh->SetScalarParameterValueOnMaterials(BaseOPName, BaseOpMin);
	}
	else
		ActiveState = ESoJellyfishState::EJS_Active;

	if (bResetOnPlayerRematerialize)
		USoEventHandlerHelper::SubscribeToPlayerRematerialize(this);
	if (bResetOnReload)
		USoEventHandlerHelper::SubscribeToSoPostLoad(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bResetOnPlayerRematerialize)
		USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(this);
	if (bResetOnReload)
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);

	Super::EndPlay(EndPlayReason);

	Collision->OnComponentHit.RemoveDynamic(this, &ASoJellyfishPlatform::OnHitCallback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (ActiveState)
	{
		case ESoJellyfishState::EJS_FadeIn:
		{
			FadeCounter += DeltaTime;
			const float FadePercent = FadeCounter / FadeDuration;
			SkeletalMesh->SetScalarParameterValueOnMaterials(OPAddName, FadePercent);
			SkeletalMesh->SetScalarParameterValueOnMaterials(BaseOPName, FMath::Lerp(BaseOpMin, BaseOpMax, FadePercent));

			if (FadePercent < 1.0f)
				return;

			Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ActiveState = ESoJellyfishState::EJS_Active;
		}
		break;

		case ESoJellyfishState::EJS_FadeOut:
		{
			FadeCounter -= DeltaTime;
			const float FadePercent = FadeCounter / FadeDuration;
			SkeletalMesh->SetScalarParameterValueOnMaterials(OPAddName, FadePercent);
			SkeletalMesh->SetScalarParameterValueOnMaterials(BaseOPName, FMath::Lerp(-1.2, 0.5, FadePercent));

			if (FadePercent > 0.0f)
				return;

			SkeletalMesh->SetVisibility(false);
			ActiveState = ESoJellyfishState::EJS_Inactive;
			if (FadeBackTime > 0.0f)
			{
				ActiveState = ESoJellyfishState::EJS_FadeIn;
				GetWorld()->GetTimerManager().SetTimer(Timer, this, &ASoJellyfishPlatform::StartFade, FadeBackTime, false);
			}
		}
		break;

		default:
			break;
	}

	SetActorTickEnabled(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::Trigger_Implementation(const FSoTriggerData& TriggerData)
{
	const bool bResetToDefaultState = TriggerData.SourceIdentifier == 0;
	ResetToDefaultState(bResetToDefaultState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::HandlePlayerRematerialize_Implementation()
{
	if (bResetOnPlayerRematerialize)
	{
		GetWorld()->GetTimerManager().ClearTimer(Timer);
		ResetToDefaultState();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::HandleSoPostLoad_Implementation()
{
	if (bResetOnReload)
	{
		GetWorld()->GetTimerManager().ClearTimer(Timer);
		ResetToDefaultState();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::OnHitCallback(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// play hit react anim
	SkeletalMesh->SetPosition(0.0f);
	SkeletalMesh->Play(false);
	ParticleComponent->Activate(true);
	HitSFX->Activate(true);

	if (bFadeOutOnHit && ActiveState != ESoJellyfishState::EJS_FadeOut)
	{
		ActiveState = ESoJellyfishState::EJS_FadeOut;
		GetWorld()->GetTimerManager().SetTimer(Timer, this, &ASoJellyfishPlatform::StartFade, DeactivationDelay, false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::ASoJellyfishPlatform::StartFade()
{
	SetActorTickEnabled(true);
	if (ActiveState == ESoJellyfishState::EJS_FadeOut)
	{
		FadeCounter = FadeDuration;
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		FadeCounter = 0.0f;
		SkeletalMesh->SetVisibility(true);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoJellyfishPlatform::ResetToDefaultState(bool bResetToDefaultState)
{
	const bool bInactivate = (bResetToDefaultState != bActiveByDefault);
	if (bInactivate)
	{
		switch (ActiveState)
		{
			case ESoJellyfishState::EJS_Active:
				ActiveState = ESoJellyfishState::EJS_FadeOut;
				StartFade();
				break;

			case ESoJellyfishState::EJS_FadeIn:
				ActiveState = ESoJellyfishState::EJS_FadeOut;
				break;

			default:
				break;
		}
	}
	else
	{
		
		switch (ActiveState)
		{
			case ESoJellyfishState::EJS_Inactive:
				ActiveState = ESoJellyfishState::EJS_FadeIn;
			case ESoJellyfishState::EJS_FadeIn:
				StartFade();
				break;

			case ESoJellyfishState::EJS_FadeOut:
				ActiveState = ESoJellyfishState::EJS_FadeIn;
				break;

			default:
				break;
		}
	}
	SetActorTickEnabled(true);
}
