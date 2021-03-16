// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.

#pragma once

#include "Containers/Map.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Sound/SoundAttenuation.h"
#include "AudioDevice.h"
#include "FMODStudioModule.h"
#include "FMODUtils.h"
#include "FMODAudioComponent.generated.h"

// Event property
UENUM()
namespace EFMODEventProperty
{
enum Type
{
    /* Priority to set on low-level channels created by this event instance (-1 to 256). */
    ChannelPriority,
    /** Schedule delay to synchronized playback for multiple tracks in DSP clocks, or -1 for default. */
    ScheduleDelay,
    /** Schedule look-ahead on the timeline in DSP clocks, or -1 for default. */
    ScheduleLookahead,
    /** Override the event's 3D minimum distance, or -1 for default. */
    MinimumDistance,
    /** Override the event's 3D maximum distance, or -1 for default. */
    MaximumDistance,
    /** Number of options */
    Count
};
}

class FFMODDynamicParameter : public FDynamicParameter
{
public:
    FFMODDynamicParameter(float Initial = 0.0f)
        : FDynamicParameter(Initial)
    {
    }
};

/** Used to store callback info from FMOD thread to our event */
struct FTimelineMarkerProperties
{
    FString Name;
    int32 Position;
};

/** Used to store callback info from FMOD thread to our event */
struct FTimelineBeatProperties
{
    int32 Bar;
    int32 Beat;
    int32 Position;
    float Tempo;
    int32 TimeSignatureUpper;
    int32 TimeSignatureLower;
};

USTRUCT(BlueprintType)
struct FFMODAttenuationDetails
{
    GENERATED_USTRUCT_BODY()

    /** Should we use Attenuation set in Studio or be able to modify in Editor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Attenuation")
    uint32 bOverrideAttenuation : 1;

    /** Override the event's 3D minimum distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Attenuation",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bOverrideAttenuation"))
    float MinimumDistance;

    /** Override the event's 3D maximum distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Attenuation",
        meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bOverrideAttenuation"))
    float MaximumDistance;

    FFMODAttenuationDetails()
        : bOverrideAttenuation(false)
        , MinimumDistance(1.0f)
        , MaximumDistance(10.0f)
    {
    }
};

USTRUCT(BlueprintType)
struct FFMODOcclusionDetails
{
    GENERATED_USTRUCT_BODY()

    /** Enable Occlusion Settings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion")
    bool bEnableOcclusion;

    /* Which trace channel to use for audio occlusion checks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion", meta = (EditCondition = "bEnableOcclusion"))
    TEnumAsByte<enum ECollisionChannel> OcclusionTraceChannel;

    /** The low pass filter frequency (in hertz) to apply if the sound playing in this audio component is occluded. This will override the frequency set in LowPassFilterFrequency. A frequency of 0.0 is the device sample rate and will bypass the filter. */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
    float OcclusionLowPassFilterFrequency;

    /** The amount of volume attenuation to apply to sounds which are occluded. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion",
        meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0", EditCondition = "bEnableOcclusion"))
    float OcclusionVolumeAttenuation;

    /** The amount of time in seconds to interpolate to the target OcclusionLowPassFilterFrequency when a sound is occluded. */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
    float OcclusionInterpolationTime;

    /** Whether or not to enable complex geometry occlusion checks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD|Occlusion", meta = (EditCondition = "bEnableOcclusion"))
    bool bUseComplexCollisionForOcclusion;

    FFMODOcclusionDetails()
        : bEnableOcclusion(false)
        , OcclusionTraceChannel(ECC_Visibility)
        , OcclusionLowPassFilterFrequency(20000.0f)
        , OcclusionVolumeAttenuation(1.0f)
        , OcclusionInterpolationTime(0.1f)
        , bUseComplexCollisionForOcclusion(false)
    {
    }
};

/** called when an event stops, either because it played to completion or because a Stop() call turned it off early */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEventStopped);
/** called when we reach a named marker on the timeline */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimelineMarker, FString, Name, int32, Position);
/** called when we reach a beat on the timeline */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(
    FOnTimelineBeat, int32, Bar, int32, Beat, int32, Position, float, Tempo, int32, TimeSignatureUpper, int32, TimeSignatureLower);

namespace FMOD
{
class DSP;
class Sound;

namespace Studio
{
class EventDescription;
class EventInstance;
}
}

struct FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES;
struct FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES;

/**
 * Plays FMOD Studio events.
 */
UCLASS(Blueprintable, ClassGroup = (Audio, Common), hidecategories = (Object, ActorComponent, Physics, Rendering, Mobility, LOD),
    ShowCategories = Trigger, meta = (BlueprintSpawnableComponent))
class FMODSTUDIO_API UFMODAudioComponent : public USceneComponent
{
    GENERATED_UCLASS_BODY()
public:

    /** The event asset to use for this sound */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    TAssetPtr<class UFMODEvent> Event;

    /** Event parameter cache */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SimpleDisplay, Category = FMODAudio)
    TMap<FName, float> ParameterCache;
    bool bDefaultParameterValuesCached;

    /** Sound name used for programmer sound.  Will look up the name in any loaded audio table. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    FString ProgrammerSoundName;

    /** Enable timeline callbacks for this sound, so that OnTimelineMarker and OnTimelineBeat can be used */
    UPROPERTY(EditAnywhere, Category = FMODAudio)
    uint32 bEnableTimelineCallbacks : 1;

    /** Stored properties to apply next time we create an instance */
    float StoredProperties[EFMODEventProperty::Count];

    /** Auto destroy this component on completion */
    UPROPERTY()
    uint32 bAutoDestroy : 1;

    /** Stop sound when owner is destroyed */
    UPROPERTY()
    uint32 bStopWhenOwnerDestroyed : 1;

    /** Whether we apply gain and low-pass based on audio zones. */
    uint32 bApplyAmbientVolumes : 1;

    /** Whether we apply gain and low-pass based on occlusion directly into the sound. */
    uint32 bApplyOcclusionDirect : 1;

    /** Whether we apply gain and low-pass based on occlusion onto a parameter. */
    uint32 bApplyOcclusionParameter : 1;

    /** Whether we have applied the occlusion at least once. */
    uint32 bHasCheckedOcclusion : 1;

    /** called when an event stops, either because it played to completion or because a Stop() call turned it off early */
    UPROPERTY(BlueprintAssignable)
    FOnEventStopped OnEventStopped;

    /** called when we reach a named marker (if bEnableTimelineCallbacks is true) */
    UPROPERTY(BlueprintAssignable)
    FOnTimelineMarker OnTimelineMarker;

    /** called when we reach a beat of a tempo (if bEnableTimelineCallbacks is true) */
    UPROPERTY(BlueprintAssignable)
    FOnTimelineBeat OnTimelineBeat;

    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetEvent(UFMODEvent *NewEvent);

    /** Start a sound playing on an audio component */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void Play();

    /** Stop an audio component playing its sound cue, issue any delegates if needed */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void Stop();

    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void Release();

    /** Trigger a cue in an event */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void TriggerCue();

    /** @return true if this component is currently playing an event */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    bool IsPlaying();

    /** Set volume on an audio component */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetVolume(float volume);

    /** Set pitch on an audio component */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetPitch(float pitch);

    /** Pause/Unpause an audio component */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetPaused(bool paused);

    /** Set a parameter into the event */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetParameter(FName Name, float Value);

    /** Get parameter value from the event */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    float GetParameter(FName Name);

    /** Set a parameter into the event */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetProperty(EFMODEventProperty::Type Property, float Value);

    /** Get a property from the event */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    float GetProperty(EFMODEventProperty::Type Property);

    /** Get the event length in milliseconds */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    int32 GetLength() const;

    /** Set the timeline position in milliseconds  */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetTimelinePosition(int32 Time);

    /** Get the timeline position in milliseconds */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    int32 GetTimelinePosition();

    /** Called when the event has finished stopping */
    void OnPlaybackCompleted();

    /** Update gain and low-pass based on interior volumes */
    void UpdateInteriorVolumes();

    /** Set the sound name to use for programmer sound.  Will look up the name in any loaded audio table. */
    UFUNCTION(BlueprintCallable, Category = "Audio|FMOD|Components")
    void SetProgrammerSoundName(FString Value);

    /** Set a programmer sound to use for this audio component.  Lifetime of sound must exceed that of the audio component. */
    void SetProgrammerSound(FMOD::Sound *Sound);

    /** FMOD Custom Attenuation Details */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    struct FFMODAttenuationDetails AttenuationDetails;

    /** FMOD Custom Attenuation Details */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FMODAudio)
    struct FFMODOcclusionDetails OcclusionDetails;

    /** Update attenuation if we have it set */
    void UpdateAttenuation();

    /** Apply Volume and LPF into event */
    void ApplyVolumeLPF();

    /** Cache default event parameter values */
    void CacheDefaultParameterValues();

public:
    /** Internal play function which can play events in the editor */
    void PlayInternal(EFMODSystemContext::Type Context);

    /** Actual Studio instance handle */
    FMOD::Studio::EventInstance *StudioInstance;

    void EventCallbackAddMarker(struct FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES *props);
    void EventCallbackAddBeat(struct FMOD_STUDIO_TIMELINE_BEAT_PROPERTIES *props);
    void EventCallbackCreateProgrammerSound(struct FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props);
    void EventCallbackDestroyProgrammerSound(struct FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES *props);

// Begin UObject interface.
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent &e) override;
#endif // WITH_EDITOR
    virtual void PostLoad() override;
    virtual FString GetDetailedInfoInternal() const override;
    // End UObject interface.
    // Begin USceneComponent Interface
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;
    virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;
    // End USceneComponent Interface

private:
// Begin ActorComponent interface.
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
// End ActorComponent interface.

#if WITH_EDITORONLY_DATA
    void UpdateSpriteTexture();
#endif

    void ReleaseEventCache();
    void ReleaseEventInstance();

    IFMODStudioModule& GetStudioModule()
    {
        if (Module == nullptr)
        {
            Module = &IFMODStudioModule::Get();
        }
        return *Module;
    }
    IFMODStudioModule* Module;

    // Settings for ambient volume effects
    double InteriorLastUpdateTime;
    float SourceInteriorVolume;
    float SourceInteriorLPF;
    float CurrentInteriorVolume;
    float CurrentInteriorLPF;
    float AmbientVolumeMultiplier;
    float AmbientLPF;
    float LastLPF;
    float LastVolume;
    FFMODDynamicParameter CurrentOcclusionFilterFrequency;
    FFMODDynamicParameter CurrentOcclusionVolumeAttenuation;

    // Tempo and marker callbacks
    FCriticalSection CallbackLock;
    TArray<FTimelineMarkerProperties> CallbackMarkerQueue;
    TArray<FTimelineBeatProperties> CallbackBeatQueue;

    // Direct assignment of programmer sound from other C++ code
    FMOD::Sound *ProgrammerSound;
    FMOD::DSP *LowPass;
    int LowPassParam;

    int32 EventLength;
};
