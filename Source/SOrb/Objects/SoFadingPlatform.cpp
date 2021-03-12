// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoFadingPlatform.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Character/SoCharacter.h"
#include "Basic/SoAudioManager.h"

const FName ASoFadingPlatform::OPAddName = "OP_Add";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoFadingPlatform::ASoFadingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	FadeInArea = CreateDefaultSubobject<UBoxComponent>(TEXT("FadeInArea"));
	FadeInArea->SetupAttachment(RootComponent);

	FadeOutArea = CreateDefaultSubobject<UBoxComponent>(TEXT("FadeOutArea"));
	FadeOutArea->SetupAttachment(RootComponent);

	FadeInArea->OnComponentBeginOverlap.AddDynamic(this, &ASoFadingPlatform::OnFadeInSignal);
	FadeOutArea->OnComponentBeginOverlap.AddDynamic(this, &ASoFadingPlatform::OnFadeOutSignal);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatform::BeginPlay()
{
	Super::BeginPlay();
	Reset();

	ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (SoCharacter != nullptr)
		SoCharacter->OnPlayerRematerialized.AddDynamic(this, &ASoFadingPlatform::Reset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatform::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ASoCharacter* SoCharacter = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this);
	if (SoCharacter != nullptr)
		SoCharacter->OnPlayerRematerialized.RemoveDynamic(this, &ASoFadingPlatform::Reset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Update(DeltaTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatform::Update(float DeltaSeconds)
{
	bool bFadeIn = false;

	switch (State)
	{
		case EFPS_FadedOut:
		case EFPS_FadedIn:
			SetActorTickEnabled(false);
			return;

		case EFPS_WaitBeforeFadeIn:
			bFadeIn = true;
		case EFPS_WaitBeforeFadeOut:
			Counter -= DeltaSeconds;
			if (Counter < 0.0f)
			{
				const float RestTime = -Counter;
				Counter = bFadeIn ? FadeInTime : FadeOutTime;
				State = bFadeIn ? EFPS_FadeIn : EFPS_FadeOut;
				UFMODEvent* EventToPlay = bFadeIn ? SFXFadeIn : SFXFadeOut;
				if (EventToPlay != nullptr)
					USoAudioManager::PlaySoundAtLocation(this, EventToPlay, GetActorTransform());
				Update(RestTime);
			}
			return;

		case EFPS_FadeIn:
			bFadeIn = true;
		case EFPS_FadeOut:
		{
			Counter -= DeltaSeconds;
			float FadePercent = FMath::Clamp(Counter / (bFadeIn ? FadeInTime : FadeOutTime), 0.0f, 1.0f);
			if (bFadeIn)
				FadePercent = 1.0f - FadePercent;
			Mesh->SetCollisionEnabled(FadePercent > CollisionOffPercent ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
			Mesh->SetVisibility(FadePercent > KINDA_SMALL_NUMBER);
			Mesh->SetScalarParameterValueOnMaterials(OPAddName, FadePercent);

			if (Counter < 0.0f)
			{
				State = bFadeIn ? EFPS_FadedIn : EFPS_FadedOut;

				if (bFadeIn != bFadeInRequestWasLast)
				{
					State = bFadeInRequestWasLast ? EFPS_WaitBeforeFadeIn : EFPS_WaitBeforeFadeOut;
					Counter = bFadeInRequestWasLast ? FadeInTime : FadeOutDelay;
				}
				else
					SetActorTickEnabled(false);
			}
		}
		return;

		default:
			return;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatform::Reset()
{
	SetActorTickEnabled(false);
	State = bFadedInByDefault ? EFPS_FadedIn : EFPS_FadedOut;
	Mesh->SetCollisionEnabled(bFadedInByDefault ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(bFadedInByDefault);

	bFadeInRequestWasLast = bFadedInByDefault;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatform::OnFadeInSignal(UPrimitiveComponent* OverlappedComponent,
									   AActor* OtherActor,
									   UPrimitiveComponent* OtherComp,
									   int32 OtherBodyIndex,
									   bool bFromSweep,
									   const FHitResult& SweepResult)
{
	bFadeInRequestWasLast = true;

	if (State == EFPS_FadedOut)
	{
		SetActorTickEnabled(true);
		Counter = FadeInDelay;
		State = EFPS_WaitBeforeFadeIn;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ASoFadingPlatform::OnFadeOutSignal(UPrimitiveComponent* OverlappedComponent,
										AActor* OtherActor,
										UPrimitiveComponent* OtherComp,
										int32 OtherBodyIndex,
										bool bFromSweep,
										const FHitResult& SweepResult)
{
	bFadeInRequestWasLast = false;

	if (State == EFPS_FadedIn)
	{
		SetActorTickEnabled(true);
		Counter = FadeOutDelay;
		State = EFPS_WaitBeforeFadeOut;
	}
}
