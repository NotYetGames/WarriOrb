// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoEventHelperComponent.h"
#include "Basic/Helpers/SoStaticHelper.h"
#include "SplineLogic/SoSpline.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEventHelperComponent::USoEventHelperComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHelperComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bHandleSoPostLoad)
		USoEventHandlerHelper::SubscribeToSoPostLoad(this);
	if (bHandlePlayerRematerialize)
		USoEventHandlerHelper::SubscribeToPlayerRematerialize(this);
	if (bHandlePlayerRespawn)
		USoEventHandlerHelper::SubscribeToPlayerRespawn(this);
	if (bHandleWhitelistedSplines && WhitelistedSplines.Num() > 0)
	{
		bool bInWhiteListed = false;
		USoEventHandlerHelper::SubscribeWhitelistedSplines(this, WhitelistedSplines);
		if (ASoSpline* PlayerSpline = USoStaticHelper::GetPlayerSplineLocation(this).GetSpline())
			for (TAssetPtr<ASoSpline>& AssetPtr : WhitelistedSplines)
				if (AssetPtr.Get() == PlayerSpline)
				{
					bInWhiteListed = true;
					break;
				}

		if (bInWhiteListed)
			OnWhitelistedSplinesEntered.Broadcast();
		else
			OnWhitelistedSplinesLeft.Broadcast();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHelperComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bHandleSoPostLoad)
		USoEventHandlerHelper::UnsubscribeFromSoPostLoad(this);
	if (bHandlePlayerRematerialize)
		USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(this);
	if (bHandlePlayerRespawn)
		USoEventHandlerHelper::UnsubscribeFromPlayerRespawn(this);
	if (bHandleWhitelistedSplines)
		USoEventHandlerHelper::UnsubscribeWhitelistedSplines(this);

	Super::EndPlay(EndPlayReason);
}
