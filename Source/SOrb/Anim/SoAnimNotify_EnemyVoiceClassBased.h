// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "Templates/SubclassOf.h"
#include "CharacterBase/SoVoiceUser.h"

#include "SoAnimNotify_EnemyVoiceClassBased.generated.h"

class ASoEnemy;
class UFMODEvent;

USTRUCT(BlueprintType, Blueprintable)
struct FSoEventForClass
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ASoEnemy> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TAssetPtr<UFMODEvent> Event;
};



UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Play Enemy Voice (Class)"))
class SORB_API USoAnimNotify_EnemyVoiceClassBased : public UAnimNotify
{
	GENERATED_BODY()

public:
	USoAnimNotify_EnemyVoiceClassBased();

	// Being UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *AnimSeq) override;
	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotify interface

	/** The event to play */
	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	TArray<FSoEventForClass> Events;

	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	ESoVoiceType VoiceType = ESoVoiceType::SoVoiceTypeAttack;
};
