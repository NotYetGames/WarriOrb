// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIVideoPlayer.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "IMediaEventSink.h"
#include "TimerManager.h"

#include "SoUIActorMediaAudioPlayer.h"
#include "UI/General/SoUIPressAndHoldConfirmation.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoAudioManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoUIVideoPlayer, All, All);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (VideoTextureSource != nullptr)
		SetVideoTextureSource(VideoTextureSource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bWaitingForLoad)
	{
		if (VideoTextureSource != nullptr && VideoMediaSource != nullptr && VideoMediaPlayer != nullptr)
		{
			bWaitingForLoad = false;
			OnWaitForStreamingEnd();
			StartVideo();
		}
	}
	else if (VideoMediaPlayer->IsPlaying() && SubtitleTextBlock != nullptr && CurrentSubtitles != nullptr)
	{
		const float OldTime = Counter;
		Counter = Counter + InDeltaTime;

		// hide if needed first
		for (int32 i = 0; i < CurrentSubtitles->Num(); ++i)
		{
			if (OldTime < (*CurrentSubtitles)[i].EndTime && Counter >= (*CurrentSubtitles)[i].EndTime)
			{
				// do not collapse to avoid ui glitch when it becomes visible again
				SubtitleTextBlock->SetVisibility(ESlateVisibility::Hidden);

				if (CurrentSubtitles->IsValidIndex(i + 1))
					SubtitleTextBlock->SetText((*CurrentSubtitles)[i + 1].Text);

				break;
			}
		}

		// set text and show if needed
		for (FSoVideoSubtitle& Sub : *CurrentSubtitles)
		{
			if (OldTime < Sub.StartTime && Counter >= Sub.StartTime)
			{
				SubtitleTextBlock->SetVisibility(ESlateVisibility::Visible);
				SubtitleTextBlock->SetText(Sub.Text);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::NativeConstruct()
{
	Super::NativeConstruct();

	// Hide by default
	SetVisibility(ESlateVisibility::Collapsed);

	SkipButton->OnConfirmed().AddDynamic(this, &USoUIVideoPlayer::SkipVideo);
	SubtitleTextBlock->SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::NativeDestruct()
{
	Super::NativeDestruct();

	SkipButton->OnConfirmed().RemoveDynamic(this, &USoUIVideoPlayer::OnSkipOrEnd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::StartVideo(TSoftObjectPtr<UMediaSource> VideoMediaSourcePtr,
								  TSoftObjectPtr<UMediaPlayer> VideoMediaPlayerPtr,
								  TSoftObjectPtr<UMediaTexture> VideoMediaTexturePtr,
								  UFMODEvent* InVideoSound)
{
	VideoTextureSource = nullptr;
	VideoMediaSource = nullptr;
	VideoMediaPlayer = nullptr;
	SubtitleTextBlock->SetVisibility(ESlateVisibility::Collapsed);

	bReleaseResourcesAtEnd = true;

	CurrentVideoMediaSourcePtr = VideoMediaSourcePtr;
	CurrentVideoMediaPlayerPtr = VideoMediaPlayerPtr;
	CurrentVideoMediaTexturePtr = VideoMediaTexturePtr;

	VideoSound = InVideoSound;

	if (UpdateResourcesFromPtrs())
	{
		StartVideo();
	}
	else
	{
		SetVisibility(ESlateVisibility::Visible);
		bWaitingForLoad = true;
		OnWaitForStreamingStart();
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::SetVideoTextureSource(UObject* InVideoTextureSource)
{
	checkf(InVideoTextureSource != nullptr, TEXT("Did you package the movies?"));
	VideoTextureSource = InVideoTextureSource;
	if (VideoPlayer)
	{
		VideoPlayer->Brush.SetResourceObject(VideoTextureSource);
		VideoPlayer->SynchronizeProperties();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::SetVideoMediaSource(UMediaSource* InVideoMediaSource)
{
	checkf(InVideoMediaSource != nullptr, TEXT("Did you package the movies?"));
	VideoMediaSource = InVideoMediaSource;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::SetVideoMediaPlayer(UMediaPlayer* InVideoMediaPlayer)
{
	checkf(InVideoMediaPlayer != nullptr, TEXT("Did you package the movies?"));
	if (!InVideoMediaPlayer)
	{
		return;
	}

	VideoMediaPlayer = InVideoMediaPlayer;
	VideoMediaPlayer->OnMediaEvent().AddUObject(this, &Self::HandleMediaPlayerMediaEvent);

	// This must be true because Play() manually does not work for some reason
	VideoMediaPlayer->PlayOnOpen = true;
	VideoMediaPlayer->Shuffle = false;

	if (!VideoAudioPlayer)
	{
		VideoAudioPlayer = ASoUIActorMediaAudioPlayer::SpawnInWorld(GetWorld());
	}

	if (VideoAudioPlayer)
	{
		VideoAudioPlayer->SetMediaPlayer(VideoMediaPlayer);
	}
	else
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("SetVideoMediaPlayer: No AudioPlayer set. No sounds provided RIP"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::StartVideo()
{
	SkipButton->Reinitialize();

	if (!VideoTextureSource)
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("No VideoTextureSource set. Aborting StartVideo"));
		return;
	}
	if (!VideoMediaSource)
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("No VideoMediaSource set. Aborting StartVideo"));
		return;
	}
	if (!VideoMediaPlayer)
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("No VideoMediaPlayer set. Aborting StartVideo"));
		return;
	}
	SetVisibility(ESlateVisibility::Visible);

	VideoMediaPlayer->SetLooping(bIsLooping);
	if (VideoMediaPlayer->OpenSource(VideoMediaSource))
	{
		// Play at normal rate
		// Useless call it sems cuz mediaplayer is made by users with IQ of 1000
		VideoMediaPlayer->Play();

		if (VideoSound != nullptr)
		{
			// VideoMediaPlayer->SetNativeVolume(0.0f);
			SoundEventInstance = USoAudioManager::PlaySound2D(this, VideoSound, true);
		}

		//if (VideoAudioPlayer)
		//	VideoAudioPlayer->Start();
	}
	else
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("StartVideo: Can't open media source"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::StopVideo()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (!VideoTextureSource)
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("No VideoTextureSource set. Aborting StartVideo"));
		return;
	}
	if (!VideoMediaPlayer)
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("No VideoMediaPlayer set. Can't StopVideo"));
		return;
	}

	if (VideoMediaPlayer->IsPlaying())
	{
		if (VideoSound != nullptr)
		{
			USoAudioManager::SoundInstanceStop(SoundEventInstance, true);
			SoundEventInstance.Instance = nullptr;
			VideoSound = nullptr;
		}
		
		VideoMediaPlayer->Close();
	}
	else
	{
		UE_LOG(LogSoUIVideoPlayer, Warning, TEXT("VideoMediaPlayer was not playing. Weird."));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::SkipVideo()
{
	PauseVideo(true);
	if (VideoSound != nullptr)
	{
		USoAudioManager::SoundInstanceStop(SoundEventInstance, true);
		SoundEventInstance.Instance = nullptr;
		VideoSound = nullptr;
	}
	OnSkipOrEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::PauseVideo(bool bPause)
{
	if (!VideoMediaPlayer)
	{
		UE_LOG(LogSoUIVideoPlayer, Error, TEXT("No VideoMediaPlayer set. Can't PauseVideo"));
		return;
	}

	if (bPause)
		VideoMediaPlayer->Pause();
	else
		VideoMediaPlayer->Play();

	if (VideoSound != nullptr)
		USoAudioManager::SoundInstancePause(SoundEventInstance, bPause);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIVideoPlayer::IsVideoPaused() const
{
	if (!VideoMediaPlayer)
	{
		return false;
	}

	return VideoMediaPlayer->IsPaused();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::OnSkipOrEnd()
{
	StopVideo();
	OnVideoFinishedOrSkipped.Broadcast();

	if (bReleaseResourcesAtEnd)
	{
		bReleaseResourcesAtEnd = false;
		StreamableManager.Unload(CurrentVideoMediaSourcePtr.ToSoftObjectPath());
		StreamableManager.Unload(CurrentVideoMediaPlayerPtr.ToSoftObjectPath());
		StreamableManager.Unload(CurrentVideoMediaTexturePtr.ToSoftObjectPath());
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIVideoPlayer::HandleMediaPlayerMediaEvent(EMediaEvent Event)
{
	if (VideoAudioPlayer == nullptr)
	{
		return;
	}

	if (Event == EMediaEvent::PlaybackEndReached)
	{
		OnSkipOrEnd();
	}

	//if (Event == EMediaEvent::PlaybackSuspended)
	//{
	//	VideoAudioPlayer->MediaSoundComponent->Stop();
	//}
	//else if (Event == EMediaEvent::PlaybackResumed)
	//{
	//	VideoAudioPlayer->MediaSoundComponent->Start();
	//}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIVideoPlayer::UpdateResourcesFromPtrs()
{
	bool bAllLoaded = true;

	// Texture source
	if (UObject* VTS = CurrentVideoMediaTexturePtr.Get())
	{
		SetVideoTextureSource(VTS);
	}
	else
	{
		bAllLoaded = false;
		const FSoftObjectPath ValuePath = CurrentVideoMediaTexturePtr.ToSoftObjectPath();
		StreamableManager.RequestAsyncLoad(ValuePath, [this, ValuePath]()
		{
			SetVideoTextureSource(ValuePath.ResolveObject());
		});

	}

	//  Media player
	if (UMediaPlayer* VMP = CurrentVideoMediaPlayerPtr.Get())
	{
		SetVideoMediaPlayer(VMP);
	}
	else
	{
		bAllLoaded = false;
		const FSoftObjectPath ValuePath = CurrentVideoMediaPlayerPtr.ToSoftObjectPath();
		StreamableManager.RequestAsyncLoad(ValuePath, [this, ValuePath]()
		{
			SetVideoMediaPlayer(Cast<UMediaPlayer>(ValuePath.ResolveObject()));
		});
	}

	//  Media source
	if (UMediaSource* MSP = CurrentVideoMediaSourcePtr.Get())
	{
		SetVideoMediaSource(MSP);
	}
	else
	{
		bAllLoaded = false;
		const FSoftObjectPath ValuePath = CurrentVideoMediaSourcePtr.ToSoftObjectPath();
		StreamableManager.RequestAsyncLoad(ValuePath, [this, ValuePath]()
		{
			SetVideoMediaSource(Cast<UMediaSource>(ValuePath.ResolveObject()));
		});
	}

	return bAllLoaded;
}
