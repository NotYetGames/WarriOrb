// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoUsableItemTemplate.h"
#include "SoItemTemplate.h"
#include "Basic/SoGameSingleton.h"
#include "Items/SoItemTooltipUI.h"
#include "Effects/SoEffectBase.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUsableItemTemplate::IsSlotCompatible(ESoItemSlot Slot) const
{
	return Slot == ESoItemSlot::EIS_Item0 ||
		   Slot == ESoItemSlot::EIS_Item1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUsableItemTemplate::CanBePutTogether(const USoItemTemplate* TheOther) const
{
	auto* Other = Cast<USoUsableItemTemplate>(TheOther);
	if (Other == nullptr)
		return false;

	return (UsableType == Other->UsableType) && (UsableItemName == Other->UsableItemName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoUsableItemTemplate::GetSubTypeText() const
{
	return USoGameSingleton::GetTextForUsableType(UsableType);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUsableItemTemplate::FillTooltipProperties(UObject* Tooltip) const
{
	if (Effects.Num() == 0)
		return;

	ISoItemTooltipUI::Execute_AddEmptyLine(Tooltip);
	for (auto EffectClass : Effects)
		if (EffectClass != nullptr)
			if (const USoEffectBase* DefaultEffect = Cast<USoEffectBase>(EffectClass->GetDefaultObject()))
				ISoItemTooltipUI::Execute_AddEffectLine(Tooltip, DefaultEffect, true);
}
