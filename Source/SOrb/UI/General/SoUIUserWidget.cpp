// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIUserWidget.h"
#include "Basic/SoAudioManager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidget::NativeConstruct()
{
	// Call our CPP
	ActiveChangedEvent.AddDynamic(this, &ThisClass::OnProxyActiveChangedEvent);

	Super::NativeConstruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidget::NativeDestruct()
{
	ActiveChangedEvent.RemoveDynamic(this, &ThisClass::OnProxyActiveChangedEvent);
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIUserWidget::NavigateOnPressed(bool bPlaySound)
{
	if (bPlaySound)
		USoAudioManager::PlaySound2D(this, SFXOnPressed);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIUserWidget::SetIsActive(bool bActive, bool bPlaySound)
{
	if (bIsActive != bActive)
	{
		if (bPlaySound)
			USoAudioManager::PlaySound2D(this, SFXActiveChanged);

		bIsActive = bActive;
		OnActiveChanged(bIsActive);
	}
}
