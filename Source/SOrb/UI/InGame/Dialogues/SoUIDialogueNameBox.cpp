// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIDialogueNameBox.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueNameBox::SetBackgroundImage(UTexture2D* Image)
{
	Background->SetBrushFromTexture(Image);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIDialogueNameBox::SetDisplayText(const FText& TextToDisplay)
{
	NameText->SetText(TextToDisplay);
}