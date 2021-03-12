// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoAnimNotify_CharStrikeSound.h"

#include "FMODEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Character/SoCharacter.h"
#include "Character/SoPlayerCharacterSheet.h"
#include "Basic/SoAudioManager.h"

USoAnimNotify_CharStrikeSound::USoAnimNotify_CharStrikeSound()
	: Super()
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(127, 255, 127, 255);
#endif // WITH_EDITORONLY_DATA
}

void USoAnimNotify_CharStrikeSound::Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *AnimSeq)
{
	TAssetPtr<UFMODEvent> Event = EventNormal;

	if (EventQuick.IsValid())
	{
		if (ASoCharacter* SoCharacter = Cast<ASoCharacter>(MeshComp->GetOwner()))
			if (USoPlayerCharacterSheet* PCS = SoCharacter->GetPlayerCharacterSheet())
				if (PCS->GetAttackSpeedMultiplier() > 1.0f)
					Event = EventQuick;

#if WITH_EDITORONLY_DATA
		if (bTestQuickSound)
			Event = EventQuick;
#endif // WITH_EDITORONLY_DATA
	}

	if (Event.IsValid())
	{
		if (bFollow)
		{
			// Play event attached
			USoAudioManager::PlaySoundAttached(
				Event.Get(),
				MeshComp,
				*AttachName,
				FVector(0, 0, 0),
				EAttachLocation::KeepRelativeOffset
			);
		}
		else
		{
			// Play event at location
			USoAudioManager::PlaySoundAtLocation(
				MeshComp,
				Event.Get(),
				MeshComp->GetComponentTransform()
			);
		}
	}
}

FString USoAnimNotify_CharStrikeSound::GetNotifyName_Implementation() const
{
	if (EventNormal.IsValid())
	{
		return (EventNormal.Get())->GetName();
	}

	return Super::GetNotifyName_Implementation();
}
