// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "SoItemFactory.generated.h"

class USoItemTemplate;

/**
 * Factory for items - this way they can be created in content browser
 */
UCLASS()
class SORBEDITOR_API USoItemFactory : public UFactory
{
	GENERATED_BODY()

public:
	USoItemFactory(const FObjectInitializer& ObjectInitializer);

	// UFactory interface
	/** Opens a dialog to configure the factory properties. Return false if user opted out of configuring properties */
	bool ConfigureProperties() override;

	UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
							  FFeedbackContext* Warn) override;

private:
	// Holds the template of the item class we are building
	UPROPERTY()
	TSubclassOf<USoItemTemplate> ItemTemplateClass;
};
