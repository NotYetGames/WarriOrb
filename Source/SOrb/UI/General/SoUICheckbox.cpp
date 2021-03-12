// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUICheckbox.h"
#include "Components/Image.h"
#include "Basic/SoGameSingleton.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICheckbox::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateFromCurrentState();

	// Set sane default
	if (SFXActiveChanged == nullptr)
		SFXActiveChanged = USoGameSingleton::GetSFX(ESoSFX::SettingsValueSwitch);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICheckbox::NativeConstruct()
{
	Super::NativeConstruct();

	// Reset
	SetIsHighlighted(false, false);
	SetIsActive(false, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICheckbox::UpdateFromCurrentState()
{
	if (XImage)
		XImage->SetVisibility(IsChecked() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (SelectionImage)
		SelectionImage->SetVisibility(IsHighlighted() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
