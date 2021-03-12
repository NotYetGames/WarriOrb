// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoWeaponTemplate.h"

#include "Items/SoItemTooltipUI.h"
#include "Character/SoCharacterStrike.h"
#include "Basic/SoGameSingleton.h"
#include "Basic/SoGameInstance.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWeaponTemplate::PostLoad()
{
	Super::PostLoad();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoWeaponTemplate::GetSubTypeText() const
{
	return USoGameSingleton::GetTextForWeaponType(WeaponType);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoWeaponTemplate::FillTooltipProperties(UObject* Tooltip) const
{
	ISoItemTooltipUI::Execute_AddDamageLine(Tooltip, ESoDmgType::Physical, BaseDmg.Physical, true, WeaponType == ESoWeaponType::EWT_DualWield, false);

	ISoItemTooltipUI::Execute_AddEmptyLine(Tooltip);

	ISoItemTooltipUI::Execute_AddSpecialLine(Tooltip, SpecialStrike);
	ISoItemTooltipUI::Execute_AddDamageLine(Tooltip, ESoDmgType::Physical, SpecialDmg.Physical, false, WeaponType == ESoWeaponType::EWT_DualWield, false);

	if (SpecialStrike != nullptr)
	{
		ISoItemTooltipUI::Execute_AddParamWithFloatLine(Tooltip, ESoItemParam::EIP_Cooldown, SpecialStrike->GetCooldownTime());
		ISoItemTooltipUI::Execute_AddParamWithBoolLine(Tooltip, ESoItemParamBool::EIPB_CooldownStopsInAir, !SpecialStrike->CanCountDownInAir_Implementation());
	}

	for (const FSoCharacterStrikeData& StrikeData : SpecialStrike->GetCharacterStrikes())
		for (const FSoStrikeEntry& Entry : StrikeData.StrikeList)
			for (TSubclassOf<USoEffectBase> EffectClass : Entry.HitEffects)
				if (const USoEffectBase* DefaultEffect = Cast<USoEffectBase>(EffectClass->GetDefaultObject()))
					ISoItemTooltipUI::Execute_AddEffectLine(Tooltip, DefaultEffect, false);

	// ISoItemTooltipUI::Execute_AddEmptyLine(Tooltip);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWeaponTemplate::IsValidStrikeIndex(int32 Index, bool bSpecial) const
{
	USoCharacterStrike* StrikePtr = bSpecial ? SpecialStrike : PrimaryStrike;
	return StrikePtr != nullptr && Index >= 0 && Index < StrikePtr->GetStrikeNum();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoWeaponTemplate::IsSlotCompatible(ESoItemSlot Slot) const
{
	return Slot == ESoItemSlot::EIS_Weapon0 || Slot == ESoItemSlot::EIS_Weapon1;
}
