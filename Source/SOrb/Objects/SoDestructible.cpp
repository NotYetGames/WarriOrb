// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoDestructible.h"
#include "DestructibleComponent.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ASoDestructible::ASoDestructible()
{
	PrimaryActorTick.bCanEverTick = false;

	Destructible = CreateDefaultSubobject<UDestructibleComponent>("DestructibleMesh");
	RootComponent = Destructible;
}
