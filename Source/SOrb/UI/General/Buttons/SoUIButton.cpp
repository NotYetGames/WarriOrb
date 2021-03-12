// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIButton.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

USoUIButton::USoUIButton(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsActive = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateAll();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButton::SetButtonText(FText InText)
{
	if (ElementText)
		ElementText->SetText(InText);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoUIButton::GetButtonText() const
{
	return ElementText->Text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButton::SetTextAlignment(TEnumAsByte<ETextJustify::Type> TextAlignment)
{
	ElementText->SetJustification(TextAlignment);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButton::UpdateBackgroundImage()
{
	if (!BackgroundImage)
		return;

	if (OverrideBackgroundImage)
		BackgroundImage->SetBrushFromTexture(OverrideBackgroundImage, bOverrideBackgroundImageMatchSize);

	BackgroundImage->SetVisibility(bShowBackgroundImage ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButton::UpdateText()
{
	if (!ElementText)
		return;

	if (!OverrideText.IsEmpty())
		ElementText->SetText(OverrideText);

	ElementText->SetVisibility(bShowText ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
