// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAnimNotify_EnemyVoiceRandomClassBased.h"

#include "FMODEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Enemy/SoEnemy.h"
#include "Enemy/SoEnemyVoiceManager.h"
#include "Settings/SoGameSettings.h"
#include "Basic/SoAudioManager.h"


USoAnimNotify_EnemyVoiceRandomClassBased::USoAnimNotify_EnemyVoiceRandomClassBased()
	: Super()
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(127, 255, 127, 255);
#endif // WITH_EDITORONLY_DATA
}

void USoAnimNotify_EnemyVoiceRandomClassBased::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSeq)
{
	UObject* Owner = MeshComp->GetOwner();
	const UClass* OwnerClass = Owner != nullptr ? Owner->GetClass() : nullptr;

	int32 SelectedIndex = 0;
	if (Owner != nullptr && OwnerClass != nullptr)
	{
		SelectedIndex = -1;
		for (int32 i = 0; i < Events.Num(); ++i)
		{
			if (Events[i].EnemyClass == Owner->GetClass() && Events[i].Events.Num() > 0)
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

	if (Events.IsValidIndex(SelectedIndex) && Events[SelectedIndex].Events.Num() > 0)
	{
		TAssetPtr<UFMODEvent> Event = Events[SelectedIndex].Events[FMath::RandRange(0, Events[SelectedIndex].Events.Num() - 1)];
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

FString USoAnimNotify_EnemyVoiceRandomClassBased::GetNotifyName_Implementation() const
{
	return FString("[EVRC]");
}
