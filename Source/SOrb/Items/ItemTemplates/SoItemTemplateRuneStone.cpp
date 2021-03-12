// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoItemTemplateRuneStone.h"

#include "Basic/SoGameInstance.h"
#include "Effects/SoEffectBase.h"
#include "Items/SoItemTooltipUI.h"
#include "Basic/Helpers/SoStringHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemTemplateRuneStone::FillTooltipProperties(UObject* Tooltip) const
{
	if (Effect == nullptr)
		return;

	const USoEffectBase* DefaultEffect = Cast<USoEffectBase>(Effect->GetDefaultObject());
	if (DefaultEffect != nullptr)
		ISoItemTooltipUI::Execute_AddEffectLine(Tooltip, DefaultEffect, true);

	if (CapacityCost > 0)
		ISoItemTooltipUI::Execute_AddParamWithIntLine(Tooltip, ESoItemParam::EIP_SpellCapacityCost, CapacityCost);

	if (UsageCount > 0)
		ISoItemTooltipUI::Execute_AddParamWithIntLine(Tooltip, ESoItemParam::EIP_SpellUsageCount, UsageCount);
	if (Cooldown > 0.0f)
		ISoItemTooltipUI::Execute_AddParamWithFloatLine(Tooltip, ESoItemParam::EIP_Cooldown, Cooldown);

	ISoItemTooltipUI::Execute_AddParamWithBoolLine(Tooltip, ESoItemParamBool::EIPB_SpellOnlyInAir, !bCanBeUsedInAir);
	ISoItemTooltipUI::Execute_AddParamWithBoolLine(Tooltip, ESoItemParamBool::EIPB_SpellRestricted, !bCanBeUsedInSKFreeZone);
	ISoItemTooltipUI::Execute_AddParamWithBoolLine(Tooltip, ESoItemParamBool::EIPB_CooldownStopsInAir, !bCooldownReducedInAir);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoItemTemplateRuneStone::GetDescForLevel(int32 Level, bool bLevelUp) const
{
	const TArray<FText>& TextArray = bLevelUp ? LevelUpBasedDescOverride : LevelBasedDescOverride;

	if (TextArray.IsValidIndex(Level))
		return TextArray[Level];

	return Description;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoItemTemplateRuneStone::UpdateLocalizedFields()
{
	const FString ObjectName = USoStringHelper::GetObjectBaseName(this);

	// Update LevelBasedDescOverride
	for (int32 Index = 0; Index < LevelBasedDescOverride.Num(); Index++)
	{
		FText& OverrideText = LevelBasedDescOverride[Index];
		OverrideText = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
			*OverrideText.ToString(),
			*TextNamespace,
			*(ObjectName + "_level_desc_" + FString::FromInt(Index))
		);
	}

	// Update LevelUpBasedDescOverride
	for (int32 Index = 0; Index < LevelUpBasedDescOverride.Num(); Index++)
	{
		FText& OverrideText = LevelUpBasedDescOverride[Index];
		OverrideText = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
			*OverrideText.ToString(),
			*TextNamespace,
			*(ObjectName + "_level_up_desc_" + FString::FromInt(Index))
		);
	}

	Super::UpdateLocalizedFields();
}
