// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "Factories/SoItemFactory.h"
#include "Items/ItemTemplates/SoItemTemplate.h"
#include "SoEditorUtilities.h"

/////////////////////////////////////////////////////
// USoItemFactory
USoItemFactory::USoItemFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USoItemTemplate::StaticClass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoItemFactory::ConfigureProperties()
{
	static const FText TitleText = FText::FromString(TEXT("Pick Item Template Class"));

	// nullptr the ItemTemplateClass so we can check for selection
	ItemTemplateClass = nullptr;

	UClass* ChosenClass = nullptr;
	const bool bPressedOk = FSoEditorUtilities::PickChildrenOfClass(TitleText, ChosenClass, SupportedClass);
	if (bPressedOk)
		ItemTemplateClass = ChosenClass;

	return bPressedOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UObject* USoItemFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
										  UObject* Context, FFeedbackContext* Warn)
{
	if (ItemTemplateClass != nullptr)
		return NewObject<USoItemTemplate>(InParent, ItemTemplateClass, Name, Flags | RF_Transactional);

	// This will fail as the default USoItemTemplate (Class) is abstract
	checkNoEntry();
	return nullptr;
}
