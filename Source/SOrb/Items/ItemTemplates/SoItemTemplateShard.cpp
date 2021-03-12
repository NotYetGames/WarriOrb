// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoItemTemplateShard.h"

#include "Items/SoItemTooltipUI.h"

#include "Basic/SoGameSingleton.h"
#include "Effects/SoEffectBase.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoItemTemplateShard::IsSlotCompatible(ESoItemSlot Slot) const
{
	switch (Slot)
	{
		case ESoItemSlot::EIS_ShardDefensive0:
		case ESoItemSlot::EIS_ShardDefensive1:
			return ShardType == ESoShardType::Defensive;

		case ESoItemSlot::EIS_ShardOffensive0:
		case ESoItemSlot::EIS_ShardOffensive1:
			return ShardType == ESoShardType::Offensive;

		case ESoItemSlot::EIS_ShardSpecial:
			return ShardType == ESoShardType::Special;

		default:
			return false;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ESoItemSlot USoItemTemplateShard::GetDefaultSlot() const
{
	switch (ShardType)
	{
		case ESoShardType::Defensive:
			return ESoItemSlot::EIS_ShardDefensive0;

		case ESoShardType::Offensive:
			return ESoItemSlot::EIS_ShardOffensive0;

		case ESoShardType::Special:
			return ESoItemSlot::EIS_ShardSpecial;

		default:
			return ESoItemSlot::EIS_Max;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoItemTemplateShard::GetSubTypeText() const
{
	return USoGameSingleton::GetTextForShardType(ShardType);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemTemplateShard::FillTooltipProperties(UObject* Tooltip) const
{
	for (const TSubclassOf<USoEffectBase>& EffectClass : Effects)
		if (const USoEffectBase* DefaultEffect = Cast<USoEffectBase>(EffectClass->GetDefaultObject()))
			ISoItemTooltipUI::Execute_AddEffectLine(Tooltip, DefaultEffect, true);
}
