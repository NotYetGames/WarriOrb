// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "Templates/SubclassOf.h"
#include "CharacterBase/SoVoiceUser.h"

#include "SoAnimNotify_EnemyVoiceRandomClassBased.generated.h"

class ASoEnemy;
class UFMODEvent;

USTRUCT(BlueprintType, Blueprintable)
struct FSoEventsForClass
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ASoEnemy> EnemyClass;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TAssetPtr<UFMODEvent>> Events;
};



UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Play Random Enemy Voice (Class)"))
class SORB_API USoAnimNotify_EnemyVoiceRandomClassBased : public UAnimNotify
{
	GENERATED_BODY()

public:
	USoAnimNotify_EnemyVoiceRandomClassBased();

	// Being UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *AnimSeq) override;
	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotify interface

	/** The event to play */
	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	TArray<FSoEventsForClass> Events;

	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	ESoVoiceType VoiceType = ESoVoiceType::SoVoiceTypeAttack;
};
