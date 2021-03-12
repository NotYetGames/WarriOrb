// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.


#include "SoBounceSFXOverrideBox.h"

ASoBounceSFXOverrideBox::ASoBounceSFXOverrideBox(const FObjectInitializer& ObjectInitializer)
{
	SFXBounceVariants.SetNum(5);
}

UFMODEvent* ASoBounceSFXOverrideBox::GetBounceSFX(int32 Index)
{
	return SFXBounceVariants.IsValidIndex(Index) ? SFXBounceVariants[Index] : nullptr;
}
