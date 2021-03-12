// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAnimNotify_EnemyVoiceRandom.h"

#include "FMODEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Enemy/SoEnemyVoiceManager.h"
#include "Settings/SoGameSettings.h"
#include "Basic/SoAudioManager.h"


USoAnimNotify_EnemyVoiceRandom::USoAnimNotify_EnemyVoiceRandom()
	: Super()
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(127, 255, 127, 255);
#endif // WITH_EDITORONLY_DATA
}

void USoAnimNotify_EnemyVoiceRandom::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSeq)
{
	if (Events.Num() == 0)
	{
		return;
	}

	TAssetPtr<UFMODEvent> Event = Events[FMath::RandRange(0, Events.Num() - 1)];
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

FString USoAnimNotify_EnemyVoiceRandom::GetNotifyName_Implementation() const
{
	if (Events.Num() > 0 && Events[0].IsValid())
	{
		return FString("[") + (Events[0].Get())->GetName() + ", ...]";
	}

	return Super::GetNotifyName_Implementation();
}
