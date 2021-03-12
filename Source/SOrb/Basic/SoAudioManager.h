// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "UObject/Object.h"
#include "FMODBlueprintStatics.h"

#include "CharacterBase/SoIMortalTypes.h"

#include "SoAudioManager.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSoPlaybackSpeedChanged, float, NewPlaybackSpeed);

class UFMODEvent;
class UFMODVCA;
class UFMODBus;

namespace FMOD
{
	class Channel;
}

namespace FMOD
{
	namespace Studio
	{
		class EventInstance;
	}
}

UENUM()
enum class ESoMusicPlayerState : uint8
{
	// Single track is playing or none
	Default = 0,

	// A new track is requested to be played, but we wait a few sec before actually starting to play it to avoid starting/stopping music all the time
	WaitForRequestThreshold,

	// Fade out is requested on the old track, once it's done we start a new one with fade
	WaitForFadeOut
};

USTRUCT()
struct FSoAmbientEntry
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY()
	UFMODEvent* Event = nullptr;

	UPROPERTY()
	FFMODEventInstance Instance;

	int32 RequestCount = 0;
};

#define LOG_IF_FMOD_ERROR(_ResultCall)                               \
	{                                                                \
		const FMOD_RESULT _Result = _ResultCall;                     \
		if (_Result != FMOD_OK)                                      \
		{                                                            \
			USoAudioManager::LogFMODError(_Result, __FUNCTION__);    \
		}                                                            \
	}


UCLASS(Blueprintable, BlueprintType)
class SORB_API USoAudioManager : public UObject
{
	GENERATED_BODY()
public:
	USoAudioManager(const FObjectInitializer& ObjectInitializer);
	~USoAudioManager();

	void Initialize();

	// Simulate tick
	void Tick(float DeltaSeconds);
	void Shutdown();

	UFUNCTION(BlueprintPure, DisplayName = "Get So Audio Manager", Category = ">Audio", meta = (WorldContext = "WorldContextObject"))
	static USoAudioManager* GetInstance(const UObject* WorldContextObject);
	static USoAudioManager& Get(const UObject* WorldContextObject)
	{
		check(IsValid(WorldContextObject));
		auto* Instance = GetInstance(WorldContextObject);
		check(IsValid(Instance));
		return *Instance;
	}

	UFUNCTION(BlueprintPure, DisplayName = "Get So Audio Manager From Object", Category = ">Audio")
	static USoAudioManager* GetInstanceFromObject(const UObject* Object) { return GetInstance(Object); }

	// Adds/removes ambient request
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static void RegisterAmbient(UObject* WorldContextObject, const TArray<UFMODEvent*>& Events, bool bAdd);

	// Enables/disables stopping/starting sfx based on requests
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD")
	void SetAmbientFreeze(bool bFreeze);

	UFUNCTION(BlueprintPure, Category = ">Audio|FMOD")
	static bool IsComponentPlaying(UFMODAudioComponent* Component);

	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD")
	static void PlayComponentIfNotPlaying(UFMODAudioComponent* Component);

	/**
	 * Plays an event.  This returns an FMOD Event Instance. The sound does not travel with any actor.
	 * NOTE: Proxy method for UFMODBlueprintStatics::PlayEvent2D
	 *
	 * @param Event - event to play
	 * @param bAutoPlay - Start the event automatically.
	 */
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD",
		meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true"))
	static FFMODEventInstance PlaySound2D(UObject* WorldContextObject, UFMODEvent* Event, bool bAutoPlay = true);

	/**
	 * Plays an event at the given location. This returns an FMOD Event Instance.  The sound does not travel with any actor.
	 * NOTE: Proxy method for UFMODBlueprintStatics::PlayEventAtLocation
	 *
	 * @param Event - event to play
	 * @param Location - World position to play event at
	 * @param bAutoPlay - Start the event automatically.
	 */
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD",
		meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", AdvancedDisplay = "2",  UnsafeDuringActorConstruction = "true"))
	static FFMODEventInstance PlaySoundAtLocation(UObject* WorldContextObject, UFMODEvent* Event, const FTransform& Location, bool bAutoPlay = true);

	/**
	 * Plays an event attached to and following the specified component.
	 * NOTE: Proxy method for UFMODBlueprintStatics::PlayEventAttached
	 *
	 * @param Event - event to play
	 * @param AttachComponent - Component to attach to.
	 * @param AttachPointName - Optional named point within the AttachComponent to play the sound at
	 * @param Location - Depending on the value of Location Type this is either a relative offset from the attach component/point or an absolute world position that will be translated to a relative offset
	 * @param LocationType - Specifies whether Location is a relative offset or an absolute world position
	 * @param bStopWhenAttachedToDestroyed - Specifies whether the sound should stop playing when the owner of the attach to component is destroyed.
	 * @param bAutoPlay - Start the event automatically.
	 * @param bAutoDestroy - Automatically destroy the audio component when the sound is stopped.
	 */
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD", meta = (AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true"))
	static UFMODAudioComponent* PlaySoundAttached(
		UFMODEvent* Event,
		USceneComponent* AttachToComponent,
		FName AttachPointName,
		FVector Location,
		EAttachLocation::Type LocationType,
		bool bStopWhenAttachedToDestroyed = false,
		bool bAutoPlay = true,
		bool bAutoDestroy = true
	);


	/**
	 * Returns whether this FMOD Event Instance is valid.  The instance will be invalidated when the sound stops.
	 * NOTE: Reimplementation UFMODBlueprintStatics::EventInstanceIsValid
	 *
	 * @param EventInstance - Event instance
	 */
	UFUNCTION(BlueprintPure, Category = ">Audio|FMOD|SoundInstance", meta = (UnsafeDuringActorConstruction = "true"))
	static bool IsSoundInstanceValid(FFMODEventInstance SoundInstance);

	/**
	 * Plays an FMOD Event Instance.
	 * NOTE: Reimplementation UFMODBlueprintStatics::EventInstancePlay
	 *
	 * @param EventInstance - Event instance
	 */
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD|SoundInstance", meta = (UnsafeDuringActorConstruction = "true"))
	static void SoundInstancePlay(FFMODEventInstance SoundInstance);

	/**
	 * Stop an FMOD Event Instance.
	 * NOTE: Reimplementation UFMODBlueprintStatics::EventInstanceStop
	 *
	 * @param EventInstance - Event instance
	 * @param Release - Whether to release the Event Instance
	 */
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD|SoundInstance", meta = (UnsafeDuringActorConstruction = "true"))
	static bool SoundInstanceStop(FFMODEventInstance SoundInstance, bool Release = false);

	/**
	 * Pause/resume an FMOD Event Instance.
	 *
	 * @param EventInstance - Event instance
	 * @param bPaused - Whether to pause or resume the event
	 */
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD|SoundInstance", meta = (UnsafeDuringActorConstruction = "true"))
	static bool SoundInstancePause(FFMODEventInstance SoundInstance, bool bPaused);

	/**
	 * Release an FMOD Event Instance.
	 * NOTE: Reimplementation UFMODBlueprintStatics::EventInstanceRelease
	 *
	 * @param EventInstance - Event instance
	 */
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD|SoundInstance", meta = (UnsafeDuringActorConstruction = "true"))
	static void SoundInstanceRelease(FFMODEventInstance SoundInstance);

	// returns with the length of the sound event in seconds
	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD")
	static float GetSoundLengthSeconds(UFMODEvent* Sound);

	UFUNCTION(BlueprintPure, Category = ">Audio|FMOD|HitReact")
	static UFMODEvent* SelectHitReactSFX(
		AActor* Mortal,
		const FSoHitReactDesc& HitReact,
		UFMODEvent* SFX,
		UFMODEvent* SFXOnDeath,
		UFMODEvent* SFXEnemy
	);

	UFUNCTION(BlueprintCallable, Category = ">Audio|FMOD|HitReact", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static void PlayHitReactSFX(
		UObject* WorldContextObject,
		AActor* Mortal,
		const FTransform& WorldTransform,
		const FSoHitReactDesc& HitReact,
		UFMODEvent* SFX,
		UFMODEvent* SFX2D,
		UFMODEvent* SFXOnDeath,
		UFMODEvent* SFXEnemy
	);

	// VCA: player volume controller
	static bool SetVolume(UFMODVCA* VCA, float Volume);
	static bool GetVolume(UFMODVCA* VCA, float& OutVolume);

	// Mute state
	static void MuteAudio();
	static void UnMuteAudio();
	static bool IsAudioMuted();

	// Sets the volume for the different sound classes, note the volume is usually normalized in the value [0.f, 1.f] but it can go higher  or lower
	// Buy why do you need to do that?
	static void SetVolumeMaster(float Volume);
	static void SetVolumeMusic(float Volume);
	static void SetVolumeSFX(float Volume);

	// Gets the volume of the different sound classes from their volume sound mixer override
	static float GetVolumeMaster();
	static float GetVolumeMusic();
	static float GetVolumeSFX();

	// Pause/Unpause all menu buses
	static void SetPausedMenuBuses(bool bOpened);

	static void LogFMODError(FMOD_RESULT Result, const char* Function);

	/**
	 * Sets the new played music
	 * if bSkipRequestDelay is true, old music fade out starts and the new music is started after StartDelay time
	 * if bSkipRequestDelay is false, RequestAcceptThreshold time is waited before the process starts, meanwhile the music can change again
	 */
	void SetMusic(UFMODEvent* NewMusic, bool bSkipRequestDelay, float StartDelay = 0.5f);

	// fade out time is hardcoded to events right now (0.5s)
	void FadeOutActiveMusic();

	void StartLoadingScreenMode(UFMODEvent* LoadingScreenMusic);
	void StopLoadingScreenMode();
	const UFMODEvent* GetCurrentlyPlayingMusic() const { return PlayedMusic;  }

	void SetMusicRestartParams(float MinWaitTime, float MaxWaitTime)
	{
		MinWaitBeforeMusicRestart = MinWaitTime;
		MaxWaitBeforeMusicRestart = MaxWaitTime;
	}

	void SetPlaybackSpeedIgnoreList(const TArray<TSoftObjectPtr<UFMODBus>>& BusList)
	{
		PlaybackSpeedIgnoreList = BusList;
	}

	UFUNCTION(BlueprintCallable)
	bool SetPlaybackSpeedOnBus(UFMODBus* Bus, float PlaybackSpeed);

	UFUNCTION(BlueprintCallable)
	void SetPlaybackSpeed(float Speed, float ChangeTime)
	{
		SourcePlaybackSpeed = TargetPlaybackSpeed;
		TargetPlaybackSpeed = Speed;
		PlaybackSpeedCounter = 0.0f;
		PlaybackSpeedChangeDuration = ChangeTime;
	}

	void TogglePrintAmbientLog()
	{
		bPrintAmbientLog = !bPrintAmbientLog;
	}

	float GetRequestAcceptThreshold() const;
	float GetMusicStartDelay() const;

protected:

	UFUNCTION()
	void OnRequestThresholdOver();

	UFUNCTION()
	void OnFadeOutFinished();

	// Starts the event instance without releasing the pointer it returns with
	FMOD::Studio::EventInstance* StartEventInstance2D(UFMODEvent* EventToPlay);

	static bool SetPlaybackSpeedOnChannel(FMOD::Channel* Channel, float PlaybackSpeed);
	static bool SetPlaybackSpeedOnChannelGroupChannels(FMOD::ChannelGroup* ChannelGroup, float PlaybackSpeed);
	static bool SetPlaybackSpeedOnChannelGroup(FMOD::ChannelGroup* ChannelGroup, float PlaybackSpeed);
	bool SetPlaybackSpeedOnAllChannels(float PlaybackSpeed);
	bool CanChangePlaybackSpeedOnChannel(FMOD::Channel* Channel) const;

	void UpdateAmbientParams();

protected:

	//
	// Audio
	//

	FMOD::Studio::EventInstance* MusicEventInstance = nullptr;

	// Currently played music
	UPROPERTY()
	UFMODEvent* PlayedMusic = nullptr;

	// Music to play next
	UPROPERTY()
	UFMODEvent* MusicToPlayNext = nullptr;

	UPROPERTY(EditAnywhere, Category = Music)
	float MusicStartDelay = 0.5f;

	// New music is only started after x time on request, change can be interrupted if it is resetted to default
	UPROPERTY(EditAnywhere, Category = Music)
	float RequestAcceptThreshold = 3.0f;

	// Map from sound unique identifier => seconds time since last used
	UPROPERTY()
	TMap<uint32, float> Sounds2DTimeSinceLastUsedMap;

	// Current state of the play music transition
	UPROPERTY()
	ESoMusicPlayerState MusicPlayerState = ESoMusicPlayerState::Default;

	// Time handle to trigger fade out
	UPROPERTY()
	FTimerHandle MusicTimeHandle;

	// Last set by  SetMusic
	UPROPERTY()
	bool bSkipRequestDelay = false;

	// Special mode
	UPROPERTY()
	bool bLoadingScreenMode = false;

	UPROPERTY()
	TArray<FSoAmbientEntry> AmbientRequests;

	bool bAmbientFreeze = false;

	float MusicRestartCounter = -1.0f;
	float MaxWaitBeforeMusicRestart = 0.f;
	float MinWaitBeforeMusicRestart = 0.f;

	bool bPrintAmbientLog = false;

	// For changing playback speed
	float SourcePlaybackSpeed = 1.0f;
	float TargetPlaybackSpeed = 1.0f;

	float PlaybackSpeedCounter = 0.f;
	float PlaybackSpeedChangeDuration = 0.f;

	UPROPERTY()
	TArray<TSoftObjectPtr<UFMODBus>> PlaybackSpeedIgnoreList;

	UPROPERTY(BlueprintAssignable)
	FSoPlaybackSpeedChanged OnPlaybackSpeedChanged;
};
