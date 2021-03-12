// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUIExternalLink.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoUIExternalLink::USoUIExternalLink(const FObjectInitializer& ObjectInitializer)
{
	bIsActive = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIExternalLink::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	bIsActive = true;
	UpdateFromCurrentState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIExternalLink::NativeConstruct()
{
	Super::NativeConstruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIExternalLink::NativeDestruct()
{
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIExternalLink::UpdateFromCurrentState()
{
	if (!TextElement || !ImageElement)
		return;

	TextElement->SetText(Text);
	ImageElement->SetBrushFromTexture(Image);

	if (IsHighlighted())
	{
		TextElement->SetVisibility(ESlateVisibility::Visible);
		TextElement->SetOpacity(OpacityHighlighted);
		ImageElement->SetOpacity(OpacityHighlighted);
	}
	else
	{
		if (!bAlwaysShowText)
			TextElement->SetVisibility(ESlateVisibility::Collapsed);

		ImageElement->SetOpacity(OpacityUnHighlighted);
		TextElement->SetOpacity(OpacityUnHighlighted);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUIExternalLink::NavigateOnPressed(bool bPlaySound)
{
	Super::NavigateOnPressed(bPlaySound);
	OpenLink();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIExternalLink::OnStateChanged()
{
	Super::OnStateChanged();
	UpdateFromCurrentState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUIExternalLink::OpenLink()
{
	if (OpenLinkEventCPP.IsBound())
	{
		OpenLinkEventCPP.Execute();
	}
	else
	{
		USoPlatformHelper::LaunchURL(ExternalLink, bCopyExternalLinkIntoClipboard);
	}
}
