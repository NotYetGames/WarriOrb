// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "SoBounceSFXOverrideBox.generated.h"

class UFMODEvent;

/**
 *
 */
UCLASS()
class SORB_API ASoBounceSFXOverrideBox : public ATriggerBox
{
	GENERATED_BODY()

public:

	ASoBounceSFXOverrideBox(const FObjectInitializer& ObjectInitializer);
	UFMODEvent* GetBounceSFX(int32 Index);

protected:

	UPROPERTY(EditFixedSize, EditAnywhere, Category = SFX)
	TArray<UFMODEvent*> SFXBounceVariants;
};
