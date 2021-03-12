// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "AssetTypeActions_SoItem.h"
#include "AssetData.h"

FText FAssetTypeActions_SoItemTemplate::GetAssetDescription(const FAssetData& AssetData) const
{
	UObject* Object = AssetData.GetAsset();
	if (Object == nullptr)
		return FText::GetEmpty();

	return FText::FromString(Object->GetDesc());
}

TSharedPtr<SWidget> FAssetTypeActions_SoItemTemplate::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	// Trigger load asset, the actual thumbnail is show in USoItemThumbnailRenderer
	if (!AssetData.IsAssetLoaded())
		AssetData.GetAsset();

	return nullptr;
}
