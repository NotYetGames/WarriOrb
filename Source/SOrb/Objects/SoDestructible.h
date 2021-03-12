// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "SoDestructible.generated.h"

class UDestructibleComponent;

/*
 *  UDestructibleComponents created in BP are buggy so Destructibles need a c++ base
 */
UCLASS()
class SORB_API ASoDestructible : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASoDestructible();

protected:
	UDestructibleComponent* Destructible = nullptr;
};
