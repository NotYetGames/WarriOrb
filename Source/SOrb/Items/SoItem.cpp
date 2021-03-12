// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoItem.h"

#include "Basic/Helpers/SoStringHelper.h"
#include "Items/ItemTemplates/SoWeaponTemplate.h"


DEFINE_LOG_CATEGORY(LogSoItem);

FSoItem FSoItem::Invalid;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSoItem::operator==(const FSoItem& Other) const
{
	return Template == Other.Template && Amount == Other.Amount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoItem::UpdateSHA1(FSHA1& HashState) const
{
	USoStringHelper::UpdateSHA1_UObject(HashState, Template);
	USoStringHelper::UpdateSHA1_Int32(HashState, Amount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSoItem::GetStaticEffects(TArray<TSubclassOf<USoEffectBase>>& OutEffects) const
{
	if (Template != nullptr)
		Template->GetStaticEffects(OutEffects);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* FSoItem::GetIcon()
{
	return (Template == nullptr) ? nullptr : Template->GetIcon();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32 FSoItem::GetValue(bool bBuyPrice) const
{
	if (Template == nullptr)
		return 0;

	const int32 BuyPrice = Template->GetValue();
	if (bBuyPrice)
		return BuyPrice;

	return BuyPrice * 3 / 4;
}
