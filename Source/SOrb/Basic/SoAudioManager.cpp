// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoAudioManager.h"

#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

#include "FMODEvent.h"
#include "FMODBus.h"
#include "FMODBlueprintStatics.h"
#include "FMODVCA.h"
#include "fmod_errors.h"

#include "SoGameInstance.h"
#include "SoGameSingleton.h"
#include "Basic/Helpers/SoMathHelper.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SplineLogic/SoPlayerSpline.h"
#include "Character/SoCharacter.h"
#include "Enemy/SoEnemy.h"
#include "Helpers/SoDateTimeHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoAudioManager, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAudioManager::USoAudioManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAudioManager::~USoAudioManager()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::Initialize()
{
	// Should be set in Blueprint
	ensure(USoGameSingleton::Get().MenuMusicPtr.Get() != nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::Tick(float DeltaSeconds)
{
	// Set volumes for the blend
	for (FSoVCABlend* Blend : USoGameSingleton::Get().VCA_BlendArray)
	{
		if (!FMath::IsNearlyEqual(Blend->Target, Blend->Counter, KINDA_SMALL_NUMBER))
		{
			const float Speed = (1.0f - Blend->MinValue) * DeltaSeconds / Blend->BlendDuration * (Blend->Target > Blend->Counter ? 1 : -1);
			Blend->Counter += Speed;
			Blend->Counter = FMath::Clamp(Blend->Counter, Blend->MinValue, 1.0f);

			SetVolume(Blend->VCAPtr.Get(), Blend->Counter);
		}
	}

	if (MusicEventInstance != nullptr)
	{
		FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
		MusicEventInstance->getPlaybackState(&state);
		if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
		{
			MusicEventInstance->release();
			MusicEventInstance = nullptr;

			MusicRestartCounter = FMath::RandRange(MinWaitBeforeMusicRestart, MaxWaitBeforeMusicRestart);
			// UE_LOG(LogTemp, Warning, TEXT("Music stopped, wait time is %f"), MusicRestartCounter);
		}
	}
	else if (PlayedMusic != nullptr)
	{
		MusicRestartCounter -= DeltaSeconds;
		if (MusicRestartCounter < 0.0f)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Music wait time is out, it is started again!"));
			MusicEventInstance = StartEventInstance2D(PlayedMusic);
		}
	}

	// Change playback speed
	if (!FMath::IsNearlyEqual(TargetPlaybackSpeed, SourcePlaybackSpeed))
	{
		PlaybackSpeedCounter = FMath::Min(PlaybackSpeedCounter + DeltaSeconds, 1.0f);
		float PlaybackSpeed = USoMathHelper::InterpolateSmoothStep(
			SourcePlaybackSpeed,
			TargetPlaybackSpeed,
			FMath::Clamp(PlaybackSpeedCounter / PlaybackSpeedChangeDuration, 0.0f, 1.0f)
		);

		// We are near our target
		if (FMath::IsNearlyEqual(TargetPlaybackSpeed, PlaybackSpeedCounter, KINDA_SMALL_NUMBER))
		{
			SourcePlaybackSpeed = TargetPlaybackSpeed;
			PlaybackSpeed = TargetPlaybackSpeed;
		}

		const bool bPlaybackSpeedChanged = SetPlaybackSpeedOnAllChannels(PlaybackSpeed);
		if (bPlaybackSpeedChanged)
			OnPlaybackSpeedChanged.Broadcast(PlaybackSpeed);
	}

	// Update times for 2D sounds effects
	for (auto& Elem : Sounds2DTimeSinceLastUsedMap)
		Elem.Value += DeltaSeconds;

	UpdateAmbientParams();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::Shutdown()
{
	if (MusicEventInstance != nullptr)
	{
		MusicEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		MusicEventInstance->release();
		MusicEventInstance = nullptr;
	}
	PlayedMusic = nullptr;
	Sounds2DTimeSinceLastUsedMap.Empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoAudioManager* USoAudioManager::GetInstance(const UObject* WorldContextObject)
{
	return USoGameInstance::Get(WorldContextObject).GetAudioManager();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::RegisterAmbient(UObject* WorldContextObject, const TArray<UFMODEvent*>& Events, bool bAdd)
{
	auto& AudioManager = Get(WorldContextObject);
	for (UFMODEvent* Event : Events)
	{
		int32 Index = 0;
		for (; Index < AudioManager.AmbientRequests.Num(); ++Index)
			if (AudioManager.AmbientRequests[Index].Event == Event)
				break;

		if (!AudioManager.AmbientRequests.IsValidIndex(Index))
		{
			if (bAdd)
			{
				FSoAmbientEntry& Entry = AudioManager.AmbientRequests.Add_GetRef({});
				Entry.Event = Event;
				Entry.RequestCount = 1;
				Entry.Instance = PlaySound2D(WorldContextObject, Event);
			}
			else
			{
				UE_LOG(LogSoAudioManager, Warning, TEXT("USoAudioManager::RegisterAmbient Failed to decrease ambient count: no such entry?!"));
			}
		}
		else
		{
			AudioManager.AmbientRequests[Index].RequestCount += bAdd ? 1 : -1;
			if (!AudioManager.bAmbientFreeze && AudioManager.AmbientRequests[Index].RequestCount == 0)
			{
				UFMODBlueprintStatics::EventInstanceStop(AudioManager.AmbientRequests[Index].Instance);
				AudioManager.AmbientRequests.RemoveAtSwap(Index);
			}
		}
	}

	if (bAdd)
		AudioManager.UpdateAmbientParams();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SetAmbientFreeze(bool bFreeze)
{
	bAmbientFreeze = bFreeze;

	if (!bAmbientFreeze)
	{
		for (int32 i = AmbientRequests.Num() - 1; i >= 0; --i)
		{
			if (AmbientRequests[i].RequestCount <= 0)
			{
				UFMODBlueprintStatics::EventInstanceStop(AmbientRequests[i].Instance);
				AmbientRequests.RemoveAtSwap(i);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::IsComponentPlaying(UFMODAudioComponent* Component)
{
	if (Component != nullptr)
		return Component->IsPlaying();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::PlayComponentIfNotPlaying(UFMODAudioComponent* Component)
{
	if (Component != nullptr && !Component->IsPlaying())
		Component->Play();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FFMODEventInstance USoAudioManager::PlaySound2D(UObject* WorldContextObject, UFMODEvent* Event, bool bAutoPlay)
{
	static FFMODEventInstance EmptyInstance;
	EmptyInstance.Instance = nullptr;
	if (Event == nullptr || WorldContextObject == nullptr)
	{
		if (WorldContextObject)
		{
			UE_LOG(LogSoAudioManager, Warning, TEXT("USoAudioManager::PlaySound2D(Context = %s) Empty Event"), *WorldContextObject->GetPathName());
		}
		else
		{
			UE_LOG(LogSoAudioManager, Warning, TEXT("USoAudioManager::PlaySound2D Empty Event AND Context"));
		}

		return EmptyInstance;
	}

	// Protect against UI spamming the same sound
	// auto& AudioManager = Get(WorldContextObject);
	// static constexpr float SpamSecondsThreshold = 0.1f;
	// const uint32 Key = Event->GetUniqueID();
	// if (AudioManager.Sounds2DTimeSinceLastUsedMap.Contains(Key))
	// {
	// 	// Abort as the spam threshold was not surpassed
	// 	const float LastUsed = AudioManager.Sounds2DTimeSinceLastUsedMap.FindChecked(Key);
	// 	if (LastUsed < SpamSecondsThreshold)
	// 		return EmptyInstance;
	// }
	//
	// // Reset
	// AudioManager.Sounds2DTimeSinceLastUsedMap.Add(Key, 0.f);

	//UE_LOG(LogSoAudioManager, Verbose, TEXT("PlaySound2D: Context = %s, EventName = %s"), *WorldContextObject->GetPathName(), *Event->GetPathName());
	return PlaySoundAtLocation(WorldContextObject, Event, FTransform(), bAutoPlay);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FFMODEventInstance USoAudioManager::PlaySoundAtLocation(UObject* WorldContextObject, UFMODEvent* Event, const FTransform& Location, bool bAutoPlay)
{
	static FFMODEventInstance EmptyInstance;
	EmptyInstance.Instance = nullptr;
	if (Event == nullptr || WorldContextObject == nullptr)
		return EmptyInstance;

	return UFMODBlueprintStatics::PlayEventAtLocation(WorldContextObject, Event, Location, bAutoPlay);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UFMODAudioComponent* USoAudioManager::PlaySoundAttached(
	UFMODEvent* Event,
	USceneComponent* AttachToComponent,
	FName AttachPointName,
	FVector Location,
	EAttachLocation::Type LocationType,
	bool bStopWhenAttachedToDestroyed,
	bool bAutoPlay,
	bool bAutoDestroy
)
{
	return UFMODBlueprintStatics::PlayEventAttached(Event, AttachToComponent, AttachPointName, Location, LocationType, bStopWhenAttachedToDestroyed, bAutoPlay, bAutoDestroy);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::IsSoundInstanceValid(FFMODEventInstance SoundInstance)
{
	if (SoundInstance.Instance)
		return SoundInstance.Instance->isValid();

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SoundInstancePlay(FFMODEventInstance SoundInstance)
{
	if (!SoundInstance.Instance)
		return;

	const FMOD_RESULT Result = SoundInstance.Instance->start();
	LOG_IF_FMOD_ERROR(Result);
	if (Result != FMOD_OK)
	{
		UE_LOG(LogSoAudioManager, Warning, TEXT("USoAudioManager::SoundInstancePlay Failed to play event instance"));
	}

	// Once we start playing, allow instance to be cleaned up when it finishes
	SoundInstance.Instance->release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SoundInstanceStop(FFMODEventInstance SoundInstance, bool Release)
{
	if (!IsSoundInstanceValid(SoundInstance))
		return false;

	const FMOD_RESULT Result = SoundInstance.Instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
	LOG_IF_FMOD_ERROR(Result);
	if (Result != FMOD_OK)
	{
		UE_LOG(LogSoAudioManager, Warning, TEXT("USoAudioManager::SoundInstanceStop Failed to stop event instance"));
		return false;
	}

	if (Release)
		SoundInstanceRelease(SoundInstance);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SoundInstancePause(FFMODEventInstance SoundInstance, bool bPaused)
{
	if (!IsSoundInstanceValid(SoundInstance))
		return false;

	const FMOD_RESULT Result = SoundInstance.Instance->setPaused(bPaused);
	LOG_IF_FMOD_ERROR(Result);
	if (Result != FMOD_OK)
	{
		UE_LOG(LogSoAudioManager, Warning, TEXT("USoAudioManager::SoundInstancePause Failed to pause or resume event instance"));
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SoundInstanceRelease(FFMODEventInstance SoundInstance)
{
	if (!SoundInstance.Instance)
		return;

	const FMOD_RESULT Result = SoundInstance.Instance->release();
	LOG_IF_FMOD_ERROR(Result);
	if (Result != FMOD_OK)
	{
		UE_LOG(LogSoAudioManager, Warning, TEXT("Failed to release event instance"));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoAudioManager::GetSoundLengthSeconds(UFMODEvent* Event)
{
	if (Event != nullptr)
	{
		FMOD::Studio::EventDescription* EventDesc = IFMODStudioModule::Get().GetEventDescription(Event, EFMODSystemContext::Type::Runtime);
		if (EventDesc != nullptr)
		{
			int32 EventLength;
			LOG_IF_FMOD_ERROR(EventDesc->getLength(&EventLength));
			return static_cast<float>(EventLength) / 1000.0f;
		}
	}
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UFMODEvent* USoAudioManager::SelectHitReactSFX(AActor* Mortal, const FSoHitReactDesc& HitReact, UFMODEvent* SFX, UFMODEvent* SFXOnDeath, UFMODEvent* SFXEnemy)
{
	if (ASoEnemy* SoEnemy = Cast<ASoEnemy>(Mortal))
		return SFXEnemy;

	if (SFXOnDeath == nullptr)
		return SFX;

	ASoCharacter* SoCharacter = Cast<ASoCharacter>(Mortal);
	if (HitReact.HitReact == ESoHitReactType::EHR_BreakIntoPieces ||
		HitReact.HitReact == ESoHitReactType::EHR_FallToDeath ||
		SoCharacter == nullptr)
	{
		return SFXOnDeath;
	}

	const bool bAlive = SoCharacter->GetActivity() != EActivity::EA_Dead;
	if (!bAlive)
		return SFXOnDeath;

	static const FName TimeName = FName("TimeName");
	const float LastSFXPlayTime = IDlgDialogueParticipant::Execute_GetFloatValue(SoCharacter, TimeName);
	const float CurrentTime = Mortal->GetWorld()->GetTimeSeconds();
	const float ThresholdSeconds = LastSFXPlayTime + USoDateTimeHelper::NormalizeTime(0.2f);
	if (ThresholdSeconds > CurrentTime)
		return SFXOnDeath;

	IDlgDialogueParticipant::Execute_ModifyFloatValue(SoCharacter, TimeName, false, CurrentTime);
	return SFX;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::PlayHitReactSFX(
	UObject* WorldContextObject,
	AActor* Mortal,
	const FTransform& WorldTransform,
	const FSoHitReactDesc& HitReact,
	UFMODEvent* SFX,
	UFMODEvent* SFX2D,
	UFMODEvent* SFXOnDeath,
	UFMODEvent* SFXEnemy
)
{
	if (ASoEnemy* SoEnemy = Cast<ASoEnemy>(Mortal))
	{
		if (SFXEnemy != nullptr)
			PlaySoundAtLocation(WorldContextObject, SFXEnemy, WorldTransform);

		return;
	}

	ASoCharacter* SoCharacter = Cast<ASoCharacter>(Mortal);
	if (SoCharacter == nullptr)
	{
		return;
	}

	static const FName TimeName = FName("TimeName");
	const float LastSFXPlayTime = IDlgDialogueParticipant::Execute_GetFloatValue(SoCharacter, TimeName);
	const float CurrentTime = Mortal->GetWorld()->GetTimeSeconds();
	const float ThresholdSeconds = LastSFXPlayTime + USoDateTimeHelper::NormalizeTime(0.2f);
	if (SFXOnDeath != nullptr &&
		(HitReact.HitReact == ESoHitReactType::EHR_BreakIntoPieces ||
		HitReact.HitReact == ESoHitReactType::EHR_FallToDeath ||
		SoCharacter == nullptr ||
		SoCharacter->GetActivity() == EActivity::EA_Dead ||
		 ThresholdSeconds > CurrentTime))
	{
		PlaySoundAtLocation(WorldContextObject, SFXOnDeath, WorldTransform);
		return;
	}

	IDlgDialogueParticipant::Execute_ModifyFloatValue(SoCharacter, TimeName, false, CurrentTime);

	if (SFX != nullptr)
		PlaySoundAtLocation(WorldContextObject, SFX, WorldTransform);

	if (SFX2D != nullptr)
		PlaySoundAtLocation(WorldContextObject, SFX2D, WorldTransform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SetVolume(UFMODVCA* AssetVCA, float Volume)
{
	if (!AssetVCA)
		return false;

	FMOD::Studio::System* StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (!StudioSystem)
		return false;

	FMOD::Studio::ID GUID = FMODUtils::ConvertGuid(AssetVCA->AssetGuid);
	FMOD::Studio::VCA* VCA = nullptr;
	const FMOD_RESULT Result = StudioSystem->getVCAByID(&GUID, &VCA);
	LOG_IF_FMOD_ERROR(Result);
	if (Result != FMOD_OK || VCA == nullptr)
		return false;

	return VCA->setVolume(Volume) == FMOD_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::GetVolume(UFMODVCA* AssetVCA, float& OutVolume)
{
	if (!AssetVCA)
		return false;

	FMOD::Studio::System* StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (!StudioSystem)
		return false;

	FMOD::Studio::ID GUID = FMODUtils::ConvertGuid(AssetVCA->AssetGuid);
	FMOD::Studio::VCA* VCA = nullptr;
	const FMOD_RESULT Result = StudioSystem->getVCAByID(&GUID, &VCA);
	LOG_IF_FMOD_ERROR(Result);
	if (Result != FMOD_OK || VCA == nullptr)
		return false;

	float Volume = 0.f;
	float FinalVolume = 0.f;
	if (VCA->getVolume(&Volume, &FinalVolume) == FMOD_OK)
	{
		OutVolume = Volume;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::MuteAudio()
{
	SetVolume(USoGameSingleton::Get().VCA_MutePtr.Get(), 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::UnMuteAudio()
{
	SetVolume(USoGameSingleton::Get().VCA_MutePtr.Get(), 1.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::IsAudioMuted()
{
	float VolumeMute = 1.f;
	GetVolume(USoGameSingleton::Get().VCA_MutePtr.Get(), VolumeMute);
	return FMath::IsNearlyEqual(VolumeMute, 0.f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SetVolumeMaster(float Volume)
{
	SetVolume(USoGameSingleton::Get().VCA_MasterPtr.Get(), Volume);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SetVolumeMusic(float Volume)
{
	SetVolume(USoGameSingleton::Get().VCA_MusicPtr.Get(), Volume);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SetVolumeSFX(float Volume)
{
	SetVolume(USoGameSingleton::Get().VCA_SFXPtr.Get(), Volume);
	SetVolume(USoGameSingleton::Get().VCA_UIPtr.Get(), Volume);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoAudioManager::GetVolumeMaster()
{
	float Volume = 0.f;
	GetVolume(USoGameSingleton::Get().VCA_MasterPtr.Get(), Volume);
	return Volume;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoAudioManager::GetVolumeMusic()
{
	float Volume = 0.f;
	GetVolume(USoGameSingleton::Get().VCA_MusicPtr.Get(), Volume);
	return Volume;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoAudioManager::GetVolumeSFX()
{
	float Volume = 0.f;
	GetVolume(USoGameSingleton::Get().VCA_SFXPtr.Get(), Volume);
	return Volume;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SetPausedMenuBuses(bool bOpened)
{
	USoGameSingleton::Get().VCA_Menu.Target = bOpened ? 0.0f : 1.0f;
	for (const TSoftObjectPtr<UFMODBus> BusPtr : USoGameSingleton::Get().MenuBusesToPauseArrayPtr)
		if (UFMODBus* Bus = BusPtr.Get())
			UFMODBlueprintStatics::BusSetPaused(Bus, bOpened);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::LogFMODError(FMOD_RESULT Result, const char* Function)
{
	const FString ErrorStr(ANSI_TO_TCHAR(FMOD_ErrorString(Result)));
	const FString FunctionStr(ANSI_TO_TCHAR(Function));
	UE_LOG(LogSoAudioManager, Error, TEXT("%s had an FMOD Error Message = `%s`"), *FunctionStr, *ErrorStr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::SetMusic(UFMODEvent* NewMusic, bool bInSkipRequestDelay, float StartDelay)
{
	MusicStartDelay = StartDelay;
	bSkipRequestDelay = bInSkipRequestDelay;

	if (NewMusic == nullptr)
		return;

	// We are loading
	if (bLoadingScreenMode)
	{
		// Cache for when we finish loading screen
		MusicToPlayNext = NewMusic;
		return;
	}

	switch (MusicPlayerState)
	{
		case ESoMusicPlayerState::Default:
			ensure(MusicToPlayNext == nullptr);

			// Music is already playing, ignore
			if (NewMusic == PlayedMusic)
				break;

			if (PlayedMusic == nullptr)
			{
				ensure(MusicEventInstance == nullptr);

				// Nothing is playing, play it now
				PlayedMusic = NewMusic;
				MusicEventInstance = StartEventInstance2D(NewMusic);
			}
			else
			{
				// Something else is playing, wait for request threshold before playing
				MusicToPlayNext = NewMusic;
				MusicPlayerState = ESoMusicPlayerState::WaitForRequestThreshold;
				if (bSkipRequestDelay)
				{
					OnRequestThresholdOver();
				}
				else
				{
					if (UWorld* World = GetWorld())
						World->GetTimerManager().SetTimer(
							MusicTimeHandle,
							this,
							&ThisClass::OnRequestThresholdOver,
							GetRequestAcceptThreshold()
						);
				}
			}
			break;

		case ESoMusicPlayerState::WaitForRequestThreshold:
			if (PlayedMusic == NewMusic)
			{
				// Cancel previous play music request as we are playing the right sound
				MusicToPlayNext = nullptr;
				MusicPlayerState = ESoMusicPlayerState::Default;
				if (UWorld* World = GetWorld())
					World->GetTimerManager().ClearTimer(MusicTimeHandle);
			}
			else
			{
				// Fade this music next instead of what was previously set
				MusicToPlayNext = NewMusic;
				if (bSkipRequestDelay)
				{
					if (UWorld* World = GetWorld())
						World->GetTimerManager().ClearTimer(MusicTimeHandle);

					OnRequestThresholdOver();
				}
			}
			break;

		case ESoMusicPlayerState::WaitForFadeOut:
			// Fade this music next instead of what was previously set
			MusicToPlayNext = NewMusic;
			break;

		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::FadeOutActiveMusic()
{
	if (MusicEventInstance != nullptr)
	{
		MusicEventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		MusicEventInstance->release();
		MusicEventInstance = nullptr;
		PlayedMusic = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::StartLoadingScreenMode(UFMODEvent* LoadingScreenMusic)
{
	if (bLoadingScreenMode == true && LoadingScreenMusic == PlayedMusic)
		return;

	// Instant everything
	SetMusic(LoadingScreenMusic, true, 0.f);
	bLoadingScreenMode = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::StopLoadingScreenMode()
{
	UFMODEvent* MusicToPlay = MusicToPlayNext;
	MusicToPlayNext = nullptr;
	bLoadingScreenMode = false;
	SetMusic(MusicToPlay, bSkipRequestDelay, GetMusicStartDelay());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::OnRequestThresholdOver()
{
	ensure(MusicPlayerState == ESoMusicPlayerState::WaitForRequestThreshold);
	ensure(MusicToPlayNext != nullptr);
	ensure(MusicToPlayNext != PlayedMusic);

	// Fade out music
	MusicPlayerState = ESoMusicPlayerState::WaitForFadeOut;
	const bool bIsInstant = FMath::IsNearlyZero(MusicStartDelay) || MusicStartDelay < 0.f;

	if (MusicEventInstance != nullptr)
	{
		MusicEventInstance->stop(bIsInstant ? FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT);
		MusicEventInstance->release();
		MusicEventInstance = nullptr;
	}

	if (bIsInstant)
	{
		// Instant
		OnFadeOutFinished();
	}
	else
	{
		// Wait
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				MusicTimeHandle,
				this,
				&ThisClass::OnFadeOutFinished,
				GetMusicStartDelay()
			);
			PlayedMusic = nullptr;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::OnFadeOutFinished()
{
	ensure(MusicPlayerState == ESoMusicPlayerState::WaitForFadeOut && MusicToPlayNext != nullptr);
	ensure(MusicEventInstance == nullptr);
	PlayedMusic = MusicToPlayNext;
	MusicToPlayNext = nullptr;

	if (PlayedMusic != nullptr)
	{
		MusicEventInstance = StartEventInstance2D(PlayedMusic);

		if (MusicEventInstance == nullptr)
			PlayedMusic = nullptr;
	}

	MusicPlayerState = ESoMusicPlayerState::Default;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FMOD::Studio::EventInstance* USoAudioManager::StartEventInstance2D(UFMODEvent* EventToPlay)
{
	FMOD::Studio::EventInstance* Instance = nullptr;

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld)
		return nullptr;

	if (FMODUtils::IsWorldAudible(ThisWorld, false))
	{
		FMOD::Studio::EventDescription* EventDesc = IFMODStudioModule::Get().GetEventDescription(EventToPlay);
		if (EventDesc != nullptr)
		{
			EventDesc->createInstance(&Instance);
			if (Instance != nullptr)
			{
				FMOD_3D_ATTRIBUTES EventAttr = { { 0 } };
				FMODUtils::Assign(EventAttr, FTransform());
				Instance->set3DAttributes(&EventAttr);
				Instance->start();
			}
		}
	}

	return Instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::CanChangePlaybackSpeedOnChannel(FMOD::Channel* Channel) const
{
	if (!Channel)
		return false;

	FMOD::Studio::System* StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (!StudioSystem)
		return false;

	// Find the parent channel group
	FMOD::ChannelGroup* ChannelGroup = nullptr;
	if (Channel->getChannelGroup(&ChannelGroup) != FMOD_OK || ChannelGroup == nullptr)
		return false;

	if (MusicEventInstance)
	{
		FMOD::ChannelGroup* MusicChannelGroup = nullptr;
		LOG_IF_FMOD_ERROR(MusicEventInstance->getChannelGroup(&MusicChannelGroup));
		for (FMOD::ChannelGroup* Group = ChannelGroup; Group != nullptr && MusicChannelGroup != nullptr; Group->getParentGroup(&Group))
			if (MusicChannelGroup == Group)
				return false;
	}

	// Is the channel group in our ignored list?
	for (TSoftObjectPtr<UFMODBus> BusPtr : PlaybackSpeedIgnoreList)
	{
		UFMODBus* Bus = BusPtr.Get();
		if (!IsValid(Bus))
			continue;

		FMOD::Studio::ID GUID = FMODUtils::ConvertGuid(Bus->AssetGuid);
		FMOD::Studio::Bus* InternalBus = nullptr;
		const FMOD_RESULT Result = StudioSystem->getBusByID(&GUID, &InternalBus);
		LOG_IF_FMOD_ERROR(Result);
		if (Result != FMOD_OK || InternalBus == nullptr)
			continue;

		FMOD::ChannelGroup* BusChannelGroup;
		if (InternalBus->getChannelGroup(&BusChannelGroup) != FMOD_OK)
			continue;

		// Find the channel group that matches
		TSet<FMOD::ChannelGroup*> AlreadyIn;
		for (FMOD::ChannelGroup* Group = ChannelGroup; Group != nullptr && !AlreadyIn.Contains(Group); Group->getParentGroup(&Group))
		{
			if (Group == BusChannelGroup)
				return false;

			AlreadyIn.Add(Group);
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SetPlaybackSpeedOnChannel(FMOD::Channel* Channel, float PlaybackSpeed)
{
	if (!Channel)
		return false;

	//
	// OLD method
	//

	FMOD::Sound* Sound = nullptr;
	Channel->getCurrentSound(&Sound);
	if (!Sound)
		return false;

	int32 Priority;
	float Frequency;
	const FMOD_RESULT Result = Sound->getDefaults(&Frequency, &Priority);
	LOG_IF_FMOD_ERROR(Result);
	if (Result == FMOD_OK)
	{
		LOG_IF_FMOD_ERROR(Channel->setFrequency(Frequency * PlaybackSpeed));
		return true;
	}

	return false;

	//
	// NEW method
	//

	// // Reference Getting Started with C++ Audio programming for Game Development page 51
	// // https://qa.fmod.com/t/timestretch-in-realtime-with-fmod-dsp-type-pitchshift/13731
	//
	// FMOD::Studio::System* StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	// if (!StudioSystem)
	// 	return false;
	//
	// FMOD::System* System = nullptr;
	// StudioSystem->getLowLevelSystem(&System);
	// if (!System)
	// 	return false;
	//
	// // Modify frequency which changes both speed and pitch
	// float Frequency = 0.f;
	//
	// // NOTE: if we get get frequency from the channel we can't know the defaults
	// // If we modify the Speed to 0.5
	// // The next time we get we get the frequency it will be wrong one
	// // Channel->getFrequency(&Frequency);
	// // Channel->setFrequency(Frequency * PlaybackSpeed);
	//
	// // Get default frequency from the current sound
	// {
	// 	FMOD::Sound* Sound = nullptr;
	// 	Channel->getCurrentSound(&Sound);
	// 	if (!Sound)
	// 		return false;
	//
	// 	int32 Priority = 0;
	// 	const FMOD_RESULT Result = Sound->getDefaults(&Frequency, &Priority);
	// 	LOG_IF_FMOD_ERROR(Result);
	// 	if (Result != FMOD_OK)
	// 		return false;
	// }
	//
	// // Set new Frequency
	// {
	// 	// System->lockDSP();
	// 	const FMOD_RESULT Result = Channel->setFrequency(Frequency * PlaybackSpeed);
	// 	LOG_IF_FMOD_ERROR(Result);
	// 	if (Result != FMOD_OK)
	// 	{
	// 		System->unlockDSP();
	// 		return false;
	// 	}
	// 	// System->unlockDSP();
	// }
	//
	// if (FMath::IsNearlyEqual(PlaybackSpeed, 1.f))
	// {
	// 	// Speed is back to normal, just remove the DSP effect
	// 	FMOD::DSP* dsp = nullptr;
	// 	LOG_IF_FMOD_ERROR(Channel->getDSP(0, &dsp));
	// 	if (!dsp)
	// 		return false;
	//
	// 	LOG_IF_FMOD_ERROR(Channel->removeDSP(dsp));
	// }
	// else
	// {
	// 	// Create a pitch shift DSP to get pitch back to normal by applying the inverse amount
	// 	FMOD::DSP* dsp = nullptr;
	// 	// System->lockDSP();
	//
	// 	// Try to get the currently DSP effect
	// 	if (Channel->getDSP(0, &dsp) != FMOD_OK)
	// 	{
	// 		// Create it
	// 		LOG_IF_FMOD_ERROR(System->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &dsp));
	//
	// 		// Add it
	// 		LOG_IF_FMOD_ERROR(Channel->addDSP(0, dsp));
	// 	}
	// 	if (!dsp)
	// 	{
	// 		//System->unlockDSP();
	// 		return false;
	// 	}
	//
	// 	LOG_IF_FMOD_ERROR(dsp->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, 1.0f / PlaybackSpeed));
	// 	//LOG_IF_FMOD_ERROR(dsp->setParameterFloat(FMOD_DSP_PITCHSHIFT_FFTSIZE, 1024.f));
	// 	//System->unlockDSP();
	// }
	//
	// return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SetPlaybackSpeedOnChannelGroupChannels(FMOD::ChannelGroup* ChannelGroup, float PlaybackSpeed)
{
	if (!ChannelGroup)
		return false;

	// Count number of channels
	FMOD::Channel* Channel = nullptr;
	int32 ChannelCount = 0;
	ChannelGroup->getNumChannels(&ChannelCount);

	// Change speed on each channel
	bool bPlaybackSpeedChanged = false;
	for (int32 i = 0; i < ChannelCount; ++i)
	{
		ChannelGroup->getChannel(i, &Channel);
		if (SetPlaybackSpeedOnChannel(Channel, PlaybackSpeed))
			bPlaybackSpeedChanged = true;
	}

	return bPlaybackSpeedChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SetPlaybackSpeedOnChannelGroup(FMOD::ChannelGroup* ChannelGroup, float PlaybackSpeed)
{
	// Change speed of all all channels of this group
	bool bPlaybackSpeedChanged = false;
	if (SetPlaybackSpeedOnChannelGroupChannels(ChannelGroup, PlaybackSpeed))
		bPlaybackSpeedChanged = true;

	// Count number of groups
	int32 NestedCount = 0;
	ChannelGroup->getNumGroups(&NestedCount);

	// Change speed of all child nested groups
	for (int32 i = 0; i < NestedCount; ++i)
	{
		FMOD::ChannelGroup* Nested;
		ChannelGroup->getGroup(i, &Nested);
		if (!Nested)
			continue;

		if (SetPlaybackSpeedOnChannelGroup(Nested, PlaybackSpeed))
			bPlaybackSpeedChanged = true;
	}

	return bPlaybackSpeedChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SetPlaybackSpeedOnAllChannels(float PlaybackSpeed)
{
	FMOD::Studio::System* StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (!StudioSystem)
		return false;

	FMOD::System* System = nullptr;
	StudioSystem->getLowLevelSystem(&System);
	if (!System)
		return false;

	bool bPlaybackSpeedChanged = false;
	FMOD::Channel* Channel = nullptr;
	for (int32 i = 0; System->getChannel(i, &Channel) == FMOD_OK && Channel != nullptr; ++i)
	{
		if (CanChangePlaybackSpeedOnChannel(Channel) || FMath::IsNearlyEqual(PlaybackSpeed, 1.f))
		{
			if (SetPlaybackSpeedOnChannel(Channel, PlaybackSpeed))
				bPlaybackSpeedChanged = true;
		}
	}

	return bPlaybackSpeedChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoAudioManager::SetPlaybackSpeedOnBus(UFMODBus* Bus, float PlaybackSpeed)
{
	FMOD::Studio::System* StudioSystem = IFMODStudioModule::Get().GetStudioSystem(EFMODSystemContext::Runtime);
	if (StudioSystem == nullptr)
		return false;

	FMOD::Studio::ID Guid = FMODUtils::ConvertGuid(Bus->AssetGuid);
	FMOD::Studio::Bus* InternalBus = nullptr;
	const FMOD_RESULT Result = StudioSystem->getBusByID(&Guid, &InternalBus);
	LOG_IF_FMOD_ERROR(Result);
	if (InternalBus == nullptr || Result != FMOD_OK)
		return false;

	FMOD::ChannelGroup* BusChannelGroup;
	if (InternalBus->getChannelGroup(&BusChannelGroup) == FMOD_OK)
	{
		return SetPlaybackSpeedOnChannelGroup(BusChannelGroup, PlaybackSpeed);
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoAudioManager::UpdateAmbientParams()
{
	AActor* PlayerActor = USoStaticHelper::GetPlayerCharacterAsActor(this);
	if (PlayerActor == nullptr)
		return;

	const FSoSplinePoint PlayerSplinePoint = USoStaticHelper::GetPlayerSplineLocation(this);
	const ASoPlayerSpline* PlayerSpline = Cast<ASoPlayerSpline>(PlayerSplinePoint.GetSpline());

	if (PlayerSpline == nullptr)
		return;

	auto ApplySingle = [this](const TArray<FSoAmbientParameter>& Params)
	{
		for (const FSoAmbientParameter& Param : Params)
		{
			for (FSoAmbientEntry& Entry : AmbientRequests)
			{
				if (Entry.Instance.Instance != nullptr)
				{
					Entry.Instance.Instance->setParameterValue(TCHAR_TO_UTF8(*Param.ParameterName.ToString()), Param.ParameterValue);
					if (bPrintAmbientLog)
						UE_LOG(LogSoAudioManager, Log, TEXT("Single applied: %s (%f)"), *Param.ParameterName.ToString(), Param.ParameterValue);
				}
			}
		}
	};

	const float PlayerSplineDistance = PlayerSplinePoint.GetDistance();

	const TArray<FSoAmbientControlPoint>& ControlPoints = PlayerSpline->GetAmbientControlPoints();

	if (ControlPoints.Num() == 0)
		return;

	// search the index of the first control point with greater spline distance
	int32 Index = 0;
	for (; ControlPoints.IsValidIndex(Index) && ControlPoints[Index].DistanceOnSpline < PlayerSplineDistance; ++Index);

	if (!ControlPoints.IsValidIndex(Index))
	{
		ApplySingle(ControlPoints.Last(0).Parameters);
		return;
	}

	if (Index == 0)
	{
		ApplySingle(ControlPoints[0].Parameters);
		return;
	}

	const FSoAmbientControlPoint& Param0 = ControlPoints[Index - 1];
	const FSoAmbientControlPoint& Param1 = ControlPoints[Index];

	const FVector PlayerLocation = PlayerActor->GetActorLocation();
	const FVector2D BA = FVector2D(Param1.Location - Param0.Location);

	const FVector2D P = FVector2D(PlayerLocation);

	const FVector2D PA = P - FVector2D(Param0.Location);
	const FVector2D BA_Normalized = BA.GetSafeNormal();

	const float PAProjectedSize = (PA | BA_Normalized);
	const FVector2D PAProjected = BA_Normalized * PAProjectedSize;

	const FVector Projected = FVector((FVector2D(Param0.Location) + PAProjected), PlayerLocation.Z);
	// DrawDebugLine(GetWorld(), FVector(FVector2D(Param1.Location), PlayerLocation.Z), Projected, FColor::Blue);
	// DrawDebugLine(GetWorld(), FVector(FVector2D(Param0.Location), PlayerLocation.Z), Projected, FColor::Green);
	// DrawDebugLine(GetWorld(), PlayerLocation, Projected, FColor::Red);

	// same direction, B is closer
	if (PAProjectedSize < 0.0f)
	{
		ApplySingle(Param1.Parameters);
		return;
	}

	const float ABSize = BA.Size();

	// same direction, B is closer
	if (PAProjectedSize > ABSize)
	{
		ApplySingle(Param0.Parameters);
		return;
	}

	const float SecondWeight = PAProjectedSize / ABSize;
	const float FirstWeight = 1.0f - SecondWeight;

	auto GetValueFromParams = [](const TArray<FSoAmbientParameter>& Params, FName ParamName, float& OutValue) -> bool
	{
		for (auto& Param : Params)
			if (Param.ParameterName == ParamName)
			{
				OutValue = Param.ParameterValue;
				return true;
			}

		return false;
	};

	// add all from first
	TArray<FSoAmbientParameter> MergedParams;
	for (const FSoAmbientParameter& Param : Param0.Parameters)
	{
		FSoAmbientParameter& NewParam = MergedParams.Add_GetRef(Param);

		float OtherParam = 0.0f;
		if (GetValueFromParams(Param1.Parameters, Param.ParameterName, OtherParam))
			NewParam.ParameterValue = Param.ParameterValue * FirstWeight + OtherParam * SecondWeight;
	}

	// add from second which is only in second
	for (const FSoAmbientParameter& Param : Param1.Parameters)
	{
		float OtherParam = 0.0f;
		if (!GetValueFromParams(Param1.Parameters, Param.ParameterName, OtherParam))
			MergedParams.Add(Param);
	}

	for (FSoAmbientEntry& Entry : AmbientRequests)
	{
		if (Entry.Instance.Instance != nullptr)
		{
			for (const FSoAmbientParameter& Param : MergedParams)
			{
				Entry.Instance.Instance->setParameterValue(TCHAR_TO_UTF8(*(Param.ParameterName.ToString())), Param.ParameterValue);
				if (bPrintAmbientLog)
					UE_LOG(LogSoAudioManager, Log, TEXT("Mixed applied: %s (%f)"), *Param.ParameterName.ToString(), Param.ParameterValue);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoAudioManager::GetRequestAcceptThreshold() const
{
	return USoDateTimeHelper::NormalizeTime(RequestAcceptThreshold);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float USoAudioManager::GetMusicStartDelay() const
{
	return USoDateTimeHelper::NormalizeTime(MusicStartDelay);
}
