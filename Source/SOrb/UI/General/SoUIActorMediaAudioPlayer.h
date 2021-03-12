// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "GameFramework/Actor.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "Engine/World.h"

#include "SoUIActorMediaAudioPlayer.generated.h"


// Helper Actor to use for MediaSoundComponent...
UCLASS()
class SORB_API ASoUIActorMediaAudioPlayer : public AActor
{
	GENERATED_BODY()

public:
	ASoUIActorMediaAudioPlayer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
	{
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bTickEvenWhenPaused = true;
		MediaSoundComponent = CreateDefaultSubobject<UMediaSoundComponent>(TEXT("MediaSoundComponent"));
		MediaSoundComponent->bAllowSpatialization = false;
		MediaSoundComponent->bIsUISound = true;
	}

	void RightAfterConstructorButNotInIt()
	{
		// Demo
		MediaSoundComponent->SetActive(true);
		MediaSoundComponent->SetTickableWhenPaused(true);
		// Role = ENetRole::ROLE_None;
	}

	void SetMediaPlayer(UMediaPlayer* MediaPlayer)
	{
		MediaSoundComponent->SetMediaPlayer(MediaPlayer);
		MediaSoundComponent->UpdatePlayer();
	}

	static ASoUIActorMediaAudioPlayer* SpawnInWorld(UWorld* World)
	{
		if (!World)
			return nullptr;

		static FTransform Transform{};
		static FActorSpawnParameters SpawnParams;
		ASoUIActorMediaAudioPlayer* This = World->SpawnActor<ThisClass>(ThisClass::StaticClass(), Transform, SpawnParams);
		This->RightAfterConstructorButNotInIt();
		return This;
	}

public:
	UPROPERTY(BlueprintReadOnly, Category = ">Video")
	UMediaSoundComponent* MediaSoundComponent = nullptr;
};
