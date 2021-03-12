// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAnimNotify_EnemyVoice.h"

#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "FMODEvent.h"

#include "Enemy/SoEnemyVoiceManager.h"
#include "Settings/SoGameSettings.h"
#include "Basic/SoAudioManager.h"


USoAnimNotify_EnemyVoice::USoAnimNotify_EnemyVoice()
	: Super()
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(127, 255, 127, 255);
#endif // WITH_EDITORONLY_DATA
}

void USoAnimNotify_EnemyVoice::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSeq)
{
	if (Event.IsValid())
	{
		if (USoEnemyVoiceManager* VoiceManager = USoEnemyVoiceManager::GetInstance(MeshComp))
		{
			if (AActor* Owner = Cast<AActor>(MeshComp->GetOwner()))
			{
				VoiceManager->PlayEnemyVoice(Owner, Event.Get(), VoiceType);
				return;
			}
		}

		// Voice gibberish disabled by settings
		if (USoGameSettings::Get().IsVoiceGibberishMuted())
			return;

		USoAudioManager::PlaySoundAttached(
			Event.Get(),
			MeshComp,
			NAME_None,
			FVector(0, 0, 0),
			EAttachLocation::KeepRelativeOffset
		);
	}
}

FString USoAnimNotify_EnemyVoice::GetNotifyName_Implementation() const
{
	if (Event.IsValid())
	{
		return (Event.Get())->GetName();
	}

	return Super::GetNotifyName_Implementation();
}
