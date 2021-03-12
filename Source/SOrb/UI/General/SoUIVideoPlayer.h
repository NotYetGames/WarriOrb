// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Blueprint/UserWidget.h"

#include "FMODBlueprintStatics.h"

#include "Engine/StreamableManager.h"
#include "MediaTexture.h"

#include "SoUIVideoPlayer.generated.h"

class ASoUIActorMediaAudioPlayer;
class UFileMediaSource;
class UMediaSource;
class UMediaPlayer;
class UImage;
enum class EMediaEvent;
class USoUIPressAndHoldConfirmation;
class UFMODEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSoVideoFinished);


UCLASS(HideCategories = ("Navigation"))
class SORB_API USoUIVideoPlayer : public UUserWidget
{
	GENERATED_BODY()
	typedef USoUIVideoPlayer Self;

public:
	//
	// UUserWidget Interface
	//
	void SynchronizeProperties() override;
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
	void NativeConstruct() override;
	void NativeDestruct() override;

	//
	// Own methods
	//

	void StartVideo(TSoftObjectPtr<UMediaSource> VideoMediaSourcePtr,
					TSoftObjectPtr<UMediaPlayer> VideoMediaPlayerPtr,
					TSoftObjectPtr<UMediaTexture> VideoMediaTexturePtr,
					UFMODEvent* InVideoSound = nullptr);


	UFUNCTION(BlueprintCallable)
	void SetVideoTextureSource(UObject* InVideoTextureSource);

	UFUNCTION(BlueprintCallable)
	void SetVideoMediaSource(UMediaSource* InVideoMediaSource);

	UFUNCTION(BlueprintCallable)
	void SetVideoMediaPlayer(UMediaPlayer* InVideoMediaPlayer);

	UFUNCTION(BlueprintCallable)
	void StartVideo();

	UFUNCTION(BlueprintCallable)
	void StopVideo();

	UFUNCTION(BlueprintCallable)
	void SkipVideo();

	void SetSubtitles(TArray<struct FSoVideoSubtitle>* Sub) { CurrentSubtitles = Sub; }

	void PauseVideo(bool bPause);
	bool IsVideoPaused() const;

	UFUNCTION(BlueprintCallable)
	void SetIsVideoLooping(bool bValue) { bIsLooping = bValue; }


	UFUNCTION()
	void OnSkipOrEnd();

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void OnWaitForStreamingStart();

	UFUNCTION(BlueprintImplementableEvent)
	void OnWaitForStreamingEnd();


	void HandleMediaPlayerMediaEvent(EMediaEvent Event);

	/** Return value true if all of them is valid and loaded
	  * if they are not the function starts the asynch load process and  they eventually will be loaded
	  * Checking the pointers is sufficient in this case
	  */
	bool UpdateResourcesFromPtrs();

public:
	// The source of the video player
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Video", meta = (AllowPrivateAccess = "true", DisplayThumbnail = "true", AllowedClasses = "Texture,MaterialInterface,SlateTextureAtlasInterface"))
	UObject* VideoTextureSource = nullptr;

	// The video file we are playing
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Video")
	UMediaSource* VideoMediaSource = nullptr;

	// The media player we are using
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Video")
	UMediaPlayer* VideoMediaPlayer = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = ">Video")
	ASoUIActorMediaAudioPlayer* VideoAudioPlayer = nullptr;

	// Is Video looping
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ">Video")
	bool bIsLooping = true;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = ">Video")
	UImage* VideoPlayer = nullptr;


	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = ">Video")
	USoUIPressAndHoldConfirmation* SkipButton;


	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = ">Video")
	class UTextBlock* SubtitleTextBlock = nullptr;


	FSoVideoFinished OnVideoFinishedOrSkipped;

	bool bWaitingForLoad = false;

	bool bReleaseResourcesAtEnd = false;

protected:

	// A native class for managing streaming assets in and keeping them in memory.
	FStreamableManager StreamableManager;

	TSoftObjectPtr<UMediaSource> CurrentVideoMediaSourcePtr;
	TSoftObjectPtr<UMediaPlayer> CurrentVideoMediaPlayerPtr;
	TSoftObjectPtr<UMediaTexture> CurrentVideoMediaTexturePtr;

	UPROPERTY()
	class UFMODEvent* VideoSound;


	FFMODEventInstance SoundEventInstance;

	// subtitles
	TArray<struct FSoVideoSubtitle>* CurrentSubtitles = nullptr;
	float Counter = -0.0f;
};
