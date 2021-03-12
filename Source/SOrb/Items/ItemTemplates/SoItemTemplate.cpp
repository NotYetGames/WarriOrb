// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoItemTemplate.h"

#include "Basic/SoGameSingleton.h"
#include "Basic/Helpers/SoStringHelper.h"

FString USoItemTemplate::TextNamespace(TEXT("ItemData"));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoItemTemplate::USoItemTemplate()
{
}

#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemTemplate::PreSave(const ITargetPlatform* TargetPlatform)
{
	UpdateLocalizedFields();
	Super::PreSave(TargetPlatform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemTemplate::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// being extra lazy here and updating text fields even if it is not needed
	UpdateLocalizedFields();
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoItemTemplate::GetObjectPath() const
{
	const auto ThePath = FSoftObjectPath(this);
	if (!ThePath.IsValid())
		return "";

	return GetClass()->GetDescription() + "'" + ThePath.ToString() + "'";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoItemTemplate* USoItemTemplate::GetTemplateFromPath(const FString& Path)
{
	if (Path.Len() == 0)
		return nullptr;

	return Cast<USoItemTemplate>(StaticLoadObject(USoItemTemplate::StaticClass(), NULL, *Path));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoItemTemplate::GetSubTypeText() const
{
	return USoGameSingleton::GetTextForItemType(ItemType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FString USoItemTemplate::GetItemTypeAsFriendlyString() const
{
	return USoStringHelper::ItemTypeToFriendlyString(ItemType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemTemplate::UpdateLocalizedFields()
{
	const FString ObjectName = USoStringHelper::GetObjectBaseName(this);
	Name = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
		*Name.ToString(),
		*TextNamespace,
		*(ObjectName + "_name")
	);
	Description = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
		*Description.ToString(),
		*TextNamespace,
		*(ObjectName + "_desc")
	);
}
