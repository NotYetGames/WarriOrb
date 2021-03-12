// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoEventHandler.h"

#include "Basic/SoGameMode.h"
#include "Basic/Helpers/SoStaticHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::SubscribeToSoPostLoad(UObject* WorldContextObject)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->SubscribeToSoPostLoad(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::UnsubscribeFromSoPostLoad(UObject* WorldContextObject)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->UnsubscribeFromSoPostLoad(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::SubscribeToPlayerRematerialize(UObject* WorldContextObject)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->SubscribeToPlayerRematerialize(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::UnsubscribeFromPlayerRematerialize(UObject* WorldContextObject)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->UnsubscribeFromPlayerRematerialize(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::SubscribeToPlayerRespawn(UObject* WorldContextObject)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->SubscribeToPlayerRespawn(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::UnsubscribeFromPlayerRespawn(UObject* WorldContextObject)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->UnsubscribeFromPlayerRespawn(WorldContextObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::SubscribeWhitelistedSplines(UObject* WorldContextObject, const TArray<TAssetPtr<ASoSpline>>& WhitelistedSplines)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->SubscribeWhitelistedSplines(WorldContextObject, WhitelistedSplines);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoEventHandlerHelper::UnsubscribeWhitelistedSplines(UObject* WorldContextObject)
{
	if (auto* GameMode = ASoGameMode::GetInstance(WorldContextObject))
		GameMode->UnsubscribeWhitelistedSplines(WorldContextObject);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoEventHandler::USoEventHandler(const FObjectInitializer& PCIP)
	: Super(PCIP)
{
}
