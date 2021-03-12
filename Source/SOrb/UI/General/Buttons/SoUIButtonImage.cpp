// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIButtonImage.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UI/General/Commands/SoUICommandImage.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (Image)
	{
		if (USoUICommandImage::IsValidOverrideWidthAndHeight(ImageOverrideWidthAndHeight))
			Image->SetOverrideWidthAndHeight(ImageOverrideWidthAndHeight);

		if (PreviewImage)
			Image->SetPreviewImage(PreviewImage);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImage::SetOverrideBackgroundImage(UTexture2D* NewImage, bool bInMatchSize)
{
	Super::SetOverrideBackgroundImage(NewImage, bInMatchSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImage::SetUICommand(ESoUICommand InCommand, bool bUpdateDeviceType)
{
	if (Image)
		Image->SetUICommand(InCommand, bUpdateDeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImage::SetUICommandPriorities(const FSoInputUICommandPriorities& Priorities, bool bUpdateImage)
{
	if (Image)
		Image->SetUICommandPriorities(Priorities, bUpdateImage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImage::SetActionNameType(ESoInputActionNameType InType, bool bUpdateDeviceType)
{
	if (Image)
		Image->SetActionNameType(InType, bUpdateDeviceType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImage::SetImageFromTexture(UTexture2D* NewTexture, bool bInMatchSize)
{
	if (Image)
		Image->EnableStaticTextureMode(NewTexture, bInMatchSize);

	EnableImageMode();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIButtonImage::UpdateImageMode()
{
	if (ElementText)
		ElementText->SetVisibility(bImageMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (BackgroundImage)
		BackgroundImage->SetVisibility(bImageMode ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (Image)
		Image->SetVisibility(bImageMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
