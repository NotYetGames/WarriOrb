// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoFadingPlatformTimeBased.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Basic/SoAudioManager.h"
#include "Engine/World.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/SoGameMode.h"
#include "CharacterBase/SoMortal.h"
#include "Character/SoCharacter.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoFadingPlatformTimeBased::ASoFadingPlatformTimeBased()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	RootComponent = Mesh;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::BeginPlay()
{
	Super::BeginPlay();

	ASoGameMode::Get(this).OnPostLoad.AddDynamic(this, &ASoFadingPlatformTimeBased::Reset);

	if (bHideOnTrigger)
		USoEventHandlerHelper::SubscribeToSoPostLoad(this);
	else
		Reset();

	if (ActionOnDisabled == ESoFadingPlatformDisabledBehavior::EFPT_Kill)
		Mesh->OnComponentHit.AddDynamic(this, &ASoFadingPlatformTimeBased::OnHitCallback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(TimeHandle);

	if (ActionOnDisabled == ESoFadingPlatformDisabledBehavior::EFPT_Kill)
		Mesh->OnComponentHit.RemoveDynamic(this, &ASoFadingPlatformTimeBased::OnHitCallback);

	ASoGameMode::Get(this).OnPostLoad.RemoveDynamic(this, &ASoFadingPlatformTimeBased::Reset);
	if (bHideOnTrigger)
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);

	Super::EndPlay(EndPlayReason);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Counter -= DeltaTime;
	float FadePercent = FMath::Clamp(Counter / (bFadeIn ? FadeInTime : FadeOutTime), 0.0f, 1.0f);
	if (bFadeIn)
		FadePercent = 1.0f - FadePercent;

	ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (ActionOnDisabled == ESoFadingPlatformDisabledBehavior::EFPT_Kill &&
		FadePercent < ActAsDisabledPercent &&
		Character &&
		Character->GetMovementBase() &&
		Character->GetMovementBase()->GetOwner() == this)
		ISoMortal::Execute_CauseDmg(Character, Dmg, HitReactDesc);

	if (ActionOnDisabled == ESoFadingPlatformDisabledBehavior::EFPT_Hide)
	{
		Mesh->SetCollisionEnabled(FadePercent > ActAsDisabledPercent ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Mesh->SetVisibility(FadePercent > KINDA_SMALL_NUMBER);
	}
	OnUpdate(FadePercent);
	Mesh->SetScalarParameterValueOnMaterials(MaterialParamName, FadeInValue * FadePercent + FadeOutValue * (1.0f - FadePercent));

	if (Counter < 0.0f)
	{
		SetActorTickEnabled(false);

		if (bFadeIn)
			GetWorld()->GetTimerManager().SetTimer(TimeHandle, this, &ASoFadingPlatformTimeBased::OnEnabledTimeOver, EnabledTime, false);
		else
			GetWorld()->GetTimerManager().SetTimer(TimeHandle, this, &ASoFadingPlatformTimeBased::OnDisabledTimeOver, DisabledTime, false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::Trigger_Implementation(const FSoTriggerData& TriggerData)
{
	if (!bHideOnTrigger)
		return;

	Mesh->SetVisibility(TriggerData.SourceIdentifier > 0, true);
	Mesh->SetCollisionEnabled(TriggerData.SourceIdentifier > 0 ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::HandleSoPostLoad_Implementation()
{
	Mesh->SetVisibility(true, true);
	Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::Reset()
{
	SetActorTickEnabled(false);

	if (ActionOnDisabled == ESoFadingPlatformDisabledBehavior::EFPT_Hide)
	{
		Mesh->SetCollisionEnabled(bFadedInByDefault ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Mesh->SetVisibility(bFadedInByDefault);
	}

	Mesh->SetScalarParameterValueOnMaterials(MaterialParamName, bFadedInByDefault ? FadeInValue : FadeOutValue);

	if (bFadedInByDefault)
		GetWorld()->GetTimerManager().SetTimer(TimeHandle, this, &ASoFadingPlatformTimeBased::OnEnabledTimeOver, EnabledTime + FirstTimeOffset, false);
	else
		GetWorld()->GetTimerManager().SetTimer(TimeHandle, this, &ASoFadingPlatformTimeBased::OnDisabledTimeOver, DisabledTime + FirstTimeOffset, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::OnEnabledTimeOver()
{
	SetActorTickEnabled(true);
	Counter = FadeInTime;
	bFadeIn = false;
	if (SFXOnHide != nullptr)
		USoAudioManager::PlaySoundAtLocation(this, SFXOnHide, GetActorTransform(), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::OnDisabledTimeOver()
{
	SetActorTickEnabled(true);
	Counter = FadeOutTime;
	bFadeIn = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatformTimeBased::OnHitCallback(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	float FadePercent = FMath::Clamp(Counter / (bFadeIn ? FadeInTime : FadeOutTime), 0.0f, 1.0f);
	if (bFadeIn)
		FadePercent = 1.0f - FadePercent;

	if (ActionOnDisabled == ESoFadingPlatformDisabledBehavior::EFPT_Kill &&
		FadePercent < ActAsDisabledPercent &&
		OtherActor &&
		OtherActor->GetClass()->ImplementsInterface(USoMortal::StaticClass()))
	{
		ISoMortal::Execute_CauseDmg(OtherActor, Dmg, HitReactDesc);
		USoAudioManager::PlayHitReactSFX(
			this,
			OtherActor,
			GetActorTransform(),
			HitReactDesc,
			SFXOnHit,
			SFXOnHit2D,
			SFXOnHitDeath,
			SFXOnEnemyHit
		);
	}
}
