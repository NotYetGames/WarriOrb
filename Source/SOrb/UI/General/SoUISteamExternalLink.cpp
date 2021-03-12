// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUISteamExternalLink.h"
#include "Basic/Helpers/SoPlatformHelper.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISteamExternalLink::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISteamExternalLink::NativeConstruct()
{
	Super::NativeConstruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISteamExternalLink::NativeDestruct()
{
	Super::NativeDestruct();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUISteamExternalLink::OpenLink()
{
#if WARRIORB_WITH_STEAM
	if (USoPlatformHelper::IsSteamInitialized() && !USoPlatformHelper::IsSteamBuildPirated())
	{
		if (USoPlatformHelper::OpenSteamOverlayToStore(ExternalAppId, bAddToCart, bShowCart))
		{
			return;
		}
	}
#endif

	// Fallback to open link
	Super::OpenLink();
}
