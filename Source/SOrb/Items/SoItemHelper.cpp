// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoItemHelper.h"

#include "Engine/ObjectLibrary.h"
#include "ItemTemplates/SoItemTemplate.h"
#include "SoItem.h"
#include "ItemTemplates/SoItemTemplateRuneStone.h"
#include "ItemTemplates/SoWeaponTemplate.h"
#include "Basic/Helpers/SoStaticHelper.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoItemTemplateRuneStone* FSoItem::GetTemplateAsRuneStone()
{
	USoItemTemplateRuneStone* RuneStone = Cast<USoItemTemplateRuneStone>(Template);
	ensure(RuneStone != nullptr);
	return RuneStone;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const USoItemTemplateRuneStone* FSoItem::GetTemplateAsRuneStone() const
{
	USoItemTemplateRuneStone* RuneStone = Cast<USoItemTemplateRuneStone>(Template);
	ensure(RuneStone != nullptr);
	return RuneStone;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemHelper::LoadAllItemsIntoMemory()
{
	// NOTE: All paths must NOT have the forward slash "/" at the end.
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(USoItemTemplate::StaticClass(), true, GIsEditor);
	ObjectLibrary->AddToRoot();
	ObjectLibrary->LoadAssetDataFromPaths({ TEXT("/Game") });
	ObjectLibrary->LoadAssetsFromAssetData();
	ObjectLibrary->RemoveFromRoot();
}
