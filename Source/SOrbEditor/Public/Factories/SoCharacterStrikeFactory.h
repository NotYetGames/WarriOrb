// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once
#include "Factories/Factory.h"

#include "SoCharacterStrikeFactory.generated.h"

/**
 * Factory for character strikes - this way they can be created in content browser
 */
UCLASS()
class SORBEDITOR_API USoCharacterStrikeFactory : public UFactory
{
	GENERATED_BODY()

public:
	USoCharacterStrikeFactory(const FObjectInitializer& ObjectInitializer);

	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
