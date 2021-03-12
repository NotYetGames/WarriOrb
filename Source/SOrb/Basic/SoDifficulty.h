// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "SoDifficulty.generated.h"


////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoDifficulty : uint8
{
	/** Soulkeeper can be resummoned without being picked up first */
	Sane			UMETA(DisplayName = "Sane"),

	/** Soulkeeper can not be summoned if it is already placed */
	Intended		UMETA(DisplayName = "Intended"),

	/** Soulkeeper is for the mediocre... I mean normal people */
	Insane			UMETA(DisplayName = "Insane"),

	NumOf			UMETA(Hidden)
};

