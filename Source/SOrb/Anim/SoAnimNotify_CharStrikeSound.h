// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "SoAnimNotify_CharStrikeSound.generated.h"

class UFMODEvent;

UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Play So Strike Sound"))
class SORB_API USoAnimNotify_CharStrikeSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	USoAnimNotify_CharStrikeSound();

	// Being UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *AnimSeq) override;
	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotify interface

	// If this sound should follow its owner
	UPROPERTY(EditAnywhere, Category = "So Anim Notify")
	uint32 bFollow : 1;

	// Socket or bone name to attach sound to
	UPROPERTY(EditAnywhere, Category = "So Anim Notify", meta = (EditCondition = "bFollow"))
	FString AttachName;

	// Sound to Play
	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	TAssetPtr<UFMODEvent> EventNormal;

	// Sound to Play if the character has a speed boost
	UPROPERTY(EditAnywhere, Category = "So Anim Notify", BlueprintReadWrite)
	TAssetPtr<UFMODEvent> EventQuick;

#if WITH_EDITORONLY_DATA
	/** Color of Notify in editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AnimNotify)
	bool bTestQuickSound = false;
#endif // WITH_EDITORONLY_DATA
};
