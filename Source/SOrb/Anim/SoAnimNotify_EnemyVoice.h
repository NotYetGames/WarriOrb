// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CharacterBase/SoVoiceUser.h"

#include "SoAnimNotify_EnemyVoice.generated.h"

class UFMODEvent;

UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Play Enemy Voice"))
class SORB_API USoAnimNotify_EnemyVoice : public UAnimNotify
{
	GENERATED_BODY()

public:
	USoAnimNotify_EnemyVoice();

	// Being UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *AnimSeq) override;
	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotify interface

	/** The event to play */
	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	TAssetPtr<UFMODEvent> Event;

	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	ESoVoiceType VoiceType = ESoVoiceType::SoVoiceTypeAttack;
};
