// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "AssetTypeActions_Base.h"
#include "Items/ItemTemplates/SoItemTemplate.h"
#include "Items/ItemTemplates/SoQuestItemTemplate.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"
#include "Items/ItemTemplates/SoItemTemplateShard.h"
#include "Items/ItemTemplates/SoUsableItemTemplate.h"
#include "Items/ItemTemplates/SoItemTemplateJewelry.h"
#include "Items/ItemTemplates/SoItemTemplateQuestBook.h"
#include "Items/ItemTemplates/SoItemTemplateRuneStone.h"

class SWidget;

/**
 * Defines the look and actions the editor takes when clicking/viewing a item template asset
 * See FSOrbEditorModule::StartupModule for usage.
 */
class FAssetTypeActions_SoItemTemplate : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_SoItemTemplate(EAssetTypeCategories::Type InAssetCategory) : AssetCategory(InAssetCategory) {}

	// IAssetTypeActions interface
	/** Returns the name of this type */
	FText GetName() const override { return FText::FromString(TEXT("Item Template")); }

	/** Returns the color associated with this type */
	FColor GetTypeColor() const override { return FColor::Blue; }

	/** Checks to see if the specified object is handled by this type. */
	UClass* GetSupportedClass() const override { return USoItemTemplate::StaticClass(); }

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

// For USoItemTemplateShard
class FAssetTypeActions_SoShardItem : public FAssetTypeActions_SoItemTemplate
{
public:
	FAssetTypeActions_SoShardItem(EAssetTypeCategories::Type InAssetCategory) : FAssetTypeActions_SoItemTemplate(InAssetCategory) {}
	FText GetName() const override { return FText::FromString(TEXT("Shard Item")); }
	UClass* GetSupportedClass() const override { return USoItemTemplateShard::StaticClass(); }
};

// For USoQuestItemTemplate
class FAssetTypeActions_SoQuestItem : public FAssetTypeActions_SoItemTemplate
{
public:
	FAssetTypeActions_SoQuestItem(EAssetTypeCategories::Type InAssetCategory) : FAssetTypeActions_SoItemTemplate(InAssetCategory) {}
	FText GetName() const override { return FText::FromString(TEXT("Quest Item")); }
	UClass* GetSupportedClass() const override { return USoQuestItemTemplate::StaticClass(); }
};

// For USoItemTemplateQuestBook
class FAssetTypeActions_SoItemTemplateQuestBook : public FAssetTypeActions_SoItemTemplate
{
public:
	FAssetTypeActions_SoItemTemplateQuestBook(EAssetTypeCategories::Type InAssetCategory) : FAssetTypeActions_SoItemTemplate(InAssetCategory) {}
	FText GetName() const override { return FText::FromString(TEXT("Quest Book")); }
	UClass* GetSupportedClass() const override { return USoItemTemplateQuestBook::StaticClass(); }
};

// For USoItemTemplateJewelry
class FAssetTypeActions_SoItemTemplateJewelry : public FAssetTypeActions_SoItemTemplate
{
public:
	FAssetTypeActions_SoItemTemplateJewelry(EAssetTypeCategories::Type InAssetCategory) : FAssetTypeActions_SoItemTemplate(InAssetCategory) {}
	FText GetName() const override { return FText::FromString(TEXT("Jewelry")); }
	UClass* GetSupportedClass() const override { return USoItemTemplateJewelry::StaticClass(); }
};


// For USoUsableItemTemplate
class FAssetTypeActions_SoUsableItem : public FAssetTypeActions_SoItemTemplate
{
public:
	FAssetTypeActions_SoUsableItem(EAssetTypeCategories::Type InAssetCategory) : FAssetTypeActions_SoItemTemplate(InAssetCategory) {}
	FText GetName() const override { return FText::FromString(TEXT("Usable Item")); }
	UClass* GetSupportedClass() const override { return USoUsableItemTemplate::StaticClass(); }
};

// For USoWeaponTemplate
class FAssetTypeActions_SoWeaponItem : public FAssetTypeActions_SoItemTemplate
{
public:
	FAssetTypeActions_SoWeaponItem(EAssetTypeCategories::Type InAssetCategory) : FAssetTypeActions_SoItemTemplate(InAssetCategory) {}
	FText GetName() const override { return FText::FromString(TEXT("Weapon Item")); }
	UClass* GetSupportedClass() const override { return USoWeaponTemplate::StaticClass(); }
};


// For USoItemTemplateRuneStone
class FAssetTypeActions_SoRuneStone : public FAssetTypeActions_SoItemTemplate
{
public:
	FAssetTypeActions_SoRuneStone(EAssetTypeCategories::Type InAssetCategory) : FAssetTypeActions_SoItemTemplate(InAssetCategory) {}
	FText GetName() const override { return FText::FromString(TEXT("Rune Stone")); }
	UClass* GetSupportedClass() const override { return USoItemTemplateRuneStone::StaticClass(); }
};
