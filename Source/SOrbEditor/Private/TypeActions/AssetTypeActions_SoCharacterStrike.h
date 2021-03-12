// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "AssetTypeActions_Base.h"
#include "Character/SoCharacterStrike.h"

class SWidget;

/**
 * Defines the look and actions the editor takes when clicking/viewing a char strike template asset
 * See FSOrbEditorModule::StartupModule for usage.
 */
class FAssetTypeActions_SoCharacterStrike : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_SoCharacterStrike(EAssetTypeCategories::Type InAssetCategory) : AssetCategory(InAssetCategory) {}

	// IAssetTypeActions interface
	/** Returns the name of this type */
	FText GetName() const override { return FText::FromString(TEXT("Character Strike")); }

	/** Returns the color associated with this type */
	FColor GetTypeColor() const override { return FColor::White; }

	/** Checks to see if the specified object is handled by this type. */
	UClass* GetSupportedClass() const override { return USoCharacterStrike::StaticClass(); }

	/** Returns the categories that this asset type. The return value is one or more flags from EAssetTypeCategories.  */
	uint32 GetCategories() override { return AssetCategory; }

	/** Optionally returns a custom widget to overlay on top of this assets' thumbnail */
	TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& AssetData) const override;
	// End of IAssetTypeActions interface

protected:
	// FAssetTypeActions_Base interface
	/** Returns additional tooltip information for the specified asset, if it has any (otherwise return the null widget) */
	FText GetAssetDescription(const FAssetData& AssetData) const override;

private:
	/** Indicates the category used for this class */
	EAssetTypeCategories::Type AssetCategory;
};
