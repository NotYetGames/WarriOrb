// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAnimNotify_EnemyVoiceClassBased.h"

#include "FMODEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Enemy/SoEnemy.h"
#include "Enemy/SoEnemyVoiceManager.h"
#include "Settings/SoGameSettings.h"
#include "Basic/SoAudioManager.h"


USoAnimNotify_EnemyVoiceClassBased::USoAnimNotify_EnemyVoiceClassBased()
	: Super()
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(127, 255, 127, 255);
#endif // WITH_EDITORONLY_DATA
}

void USoAnimNotify_EnemyVoiceClassBased::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSeq)
{
	UObject* Owner = MeshComp->GetOwner();
	const UClass* OwnerClass = Owner != nullptr ? Owner->GetClass() : nullptr;

	int32 SelectedIndex = 0;
	if (Owner != nullptr && OwnerClass != nullptr)
	{
		SelectedIndex = -1;
		for (int32 i = 0; i < Events.Num(); ++i)
		{
			if (Events[i].EnemyClass == Owner->GetClass())
			{
				SelectedIndex = i;
				break;
			}
		}

		if (SelectedIndex == -1)
		{
			return;
		}
	}

	if (Events.IsValidIndex(SelectedIndex))
	{
		TAssetPtr<UFMODEvent> Event = Events[SelectedIndex].Event;
		if (Event.IsValid())
		{
			if (USoEnemyVoiceManager* VoiceManager = USoEnemyVoiceManager::GetInstance(MeshComp))
			{
				if (AActor* ActorOwner = Cast<AActor>(Owner))
				{
					VoiceManager->PlayEnemyVoice(ActorOwner, Event.Get(), VoiceType);
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
}

FString USoAnimNotify_EnemyVoiceClassBased::GetNotifyName_Implementation() const
{
	return FString("[EVC]");
}
