// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "SoLevelConfigSaver.generated.h"


UCLASS()
class SORB_API ASoLevelConfigSaver : public AActor
{
	GENERATED_BODY()

public:

	virtual void PreSave(const ITargetPlatform* TargetPlatform) override;

};
