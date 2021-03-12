// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEnemyVoiceManager.h"

#include "Components/SkeletalMeshComponent.h"

#include "Basic/SoGameInstance.h"
#include "Basic/SoGameSingleton.h"
#include "CharacterBase/SoVoiceUser.h"
#include "Settings/SoGameSettings.h"
#include "Basic/SoAudioManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoEnemyVoiceManager, All, All)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEnemyVoiceManager::USoEnemyVoiceManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEnemyVoiceManager::~USoEnemyVoiceManager()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEnemyVoiceManager* USoEnemyVoiceManager::GetInstance(const UObject* WorldContextObject)
{
	if (USoGameInstance* GameInstance = USoGameInstance::GetInstance(WorldContextObject))
		return GameInstance->GetEnemyVoiceManager();

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEnemyVoiceManager::PlayEnemyVoice(AActor* Enemy, UFMODEvent* EventToPlay, ESoVoiceType Type)
{
	if (Enemy == nullptr || EventToPlay == nullptr)
	{
		return false;
	}

	// Voice gibberish disabled by settings
	if (USoGameSettings::Get().IsVoiceGibberishMuted())
		return false;

	if (!Enemy->GetClass()->ImplementsInterface(USoVoiceUser::StaticClass()))
	{
		UE_LOG(LogSoEnemyVoiceManager, Warning, TEXT("Failed to use Voice: class %s does not implement the VoiceUser interface"), *Enemy->GetClass()->GetName());
		return false;
	}

	if (!ISoVoiceUser::Execute_ShouldSpawnVO(Enemy, Type))
		return false;

	if (Type == ESoVoiceType::SoVoiceTypeNoise)
	{
		USceneComponent* SceneComponent = ISoVoiceUser::Execute_GetComponent(Enemy);
		const FName SocketName = ISoVoiceUser::Execute_GetSocketName(Enemy);
		USoAudioManager::PlaySoundAttached(
			EventToPlay,
			SceneComponent,
			SocketName,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget
		);
		return true;
	}

	// do a cleanup to avoid possible nullptr dereferencing
	CleanupActiveVoiceList();

	const bool bForceOverride = Type == ESoVoiceType::SoVoiceTypeDeath;

	if (FSoEnemyVoice* VoicePtr = GetActiveVoiceFromEnemy(Enemy))
	{
		if (!bForceOverride || VoicePtr->bForced)
			return false;

		VoicePtr->Voice->Stop();
		if (!PlayEnemyVoice_Internal(*VoicePtr, Enemy, EventToPlay, bForceOverride))
		{
			RemoveVoice(VoicePtr);
			return false;
		}

		return true;
	}

	if (!bForceOverride && GetActiveVoiceFromClass(Enemy) != nullptr)
		return false;

	// if there is a free voice slot
	if (FSoEnemyVoice* VoicePtr = GetFreeVoiceSlot(Enemy->GetClass(), bForceOverride))
	{
		if (!PlayEnemyVoice_Internal(*VoicePtr, Enemy, EventToPlay, bForceOverride))
		{
			RemoveVoice(VoicePtr);
			return false;
		}
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEnemyVoiceManager::StopEnemyVoice(AActor* Enemy)
{
	for (int32 i = 0; i < ActiveVoices.Num(); ++i)
		if (ActiveVoices[i].Enemy == Enemy)
		{
			if (ActiveVoices[i].Voice != nullptr)
				ActiveVoices[i].Voice->Stop();

			ActiveVoices.RemoveAtSwap(i);
			return;
		}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEnemyVoiceManager::CleanupActiveVoiceList()
{
	// DestroyComponent() calls the function again cause of FMOD :rolling_eyes:
	if (bCleanupWIP)
		return;

	bCleanupWIP = true;

	for (int32 i = ActiveVoices.Num() - 1; ActiveVoices.IsValidIndex(i); --i)
		if (ActiveVoices[i].Enemy == nullptr || ActiveVoices[i].Voice == nullptr || !ActiveVoices[i].Voice->IsActive())
		{
			if (ActiveVoices[i].Voice != nullptr)
				ActiveVoices[i].Voice->DestroyComponent();

			ActiveVoices.RemoveAtSwap(i);
		}

	bCleanupWIP = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoEnemyVoice* USoEnemyVoiceManager::GetActiveVoiceFromEnemy(AActor* Enemy)
{
	for (FSoEnemyVoice& EnemyVoice : ActiveVoices)
		if (EnemyVoice.Enemy == Enemy)
			return &EnemyVoice;

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoEnemyVoice* USoEnemyVoiceManager::GetActiveVoiceFromClass(AActor* Enemy)
{
	for (FSoEnemyVoice& EnemyVoice : ActiveVoices)
		if (EnemyVoice.Enemy->GetClass() == Enemy->GetClass())
			return &EnemyVoice;

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSoEnemyVoice* USoEnemyVoiceManager::GetFreeVoiceSlot(UClass* RequesterClass, bool bIgnoreCheck)
{
	// only 2 different enemy can talk in the same time

	if (!bIgnoreCheck)
	{
		TSet<UClass*> Classes;
		Classes.Add(RequesterClass);

		for (FSoEnemyVoice& EnemyVoice : ActiveVoices)
			if (EnemyVoice.Enemy != nullptr)
				Classes.Add(EnemyVoice.Enemy->GetClass());

		if (Classes.Num() > 2)
			return nullptr;
	}

	ActiveVoices.Add({});
	return &ActiveVoices.Last(0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoEnemyVoiceManager::PlayEnemyVoice_Internal(FSoEnemyVoice& Voice, AActor* Enemy, UFMODEvent* EventToPlay, bool bForceOverride)
{
	USceneComponent* SceneComponent = ISoVoiceUser::Execute_GetComponent(Enemy);
	const FName SocketName = ISoVoiceUser::Execute_GetSocketName(Enemy);

	Voice.Enemy = Enemy;
	Voice.bForced = bForceOverride;

	const bool bStopWhenAttachedToDestroyed = true;
	const bool bAutoPlay = true;
	const bool bAutoDestroy = false;
	Voice.Voice = USoAudioManager::PlaySoundAttached(
		EventToPlay,
		SceneComponent,
		SocketName,
		FVector::ZeroVector,
		EAttachLocation::SnapToTarget,
		bStopWhenAttachedToDestroyed,
		bAutoPlay,
		bAutoDestroy
	);

	if (Voice.Voice != nullptr && Voice.Voice->IsPlaying())
	{
		Voice.Voice->OnEventStopped.AddDynamic(this, &USoEnemyVoiceManager::CleanupActiveVoiceList);
		OnDisplayVoice.Broadcast(SceneComponent->GetSocketLocation(SocketName), USoGameSingleton::GetEnemyVoiceText(EventToPlay));
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEnemyVoiceManager::RemoveVoice(FSoEnemyVoice* VoicePtr)
{
	for (int32 i = 0; i < ActiveVoices.Num(); ++i)
		if (&ActiveVoices[i] == VoicePtr)
		{
			ActiveVoices.RemoveAtSwap(i);
			return;
		}
}
