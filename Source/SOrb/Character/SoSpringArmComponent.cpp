// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoSpringArmComponent.h"




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoSpringArmComponent::ForceUpdate()
{
	UpdateDesiredArmLocation(false, false, false, 1.0f);
}
