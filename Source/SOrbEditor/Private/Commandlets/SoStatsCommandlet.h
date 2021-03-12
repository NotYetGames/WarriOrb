// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

// Copyright 2017-2018 Csaba Molnar, Daniel Butum
#pragma once

#include "Commandlets/Commandlet.h"

#include "SoStatsCommandlet.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogSoStatsCommandlet, All, All);


UCLASS()
class USoStatsCommandlet: public UCommandlet
{
	GENERATED_BODY()

public:
	USoStatsCommandlet();

public:

	//~ UCommandlet interface
	int32 Main(const FString& Params) override;
};
