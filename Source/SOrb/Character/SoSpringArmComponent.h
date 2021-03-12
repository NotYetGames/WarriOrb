// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "GameFramework/SpringArmComponent.h"
#include "SoSpringArmComponent.generated.h"

/**
 * 
 */
UCLASS()
class SORB_API USoSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()
	
public:

	void ForceUpdate();
	
};
