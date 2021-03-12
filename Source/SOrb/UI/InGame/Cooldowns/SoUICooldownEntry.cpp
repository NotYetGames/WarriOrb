// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUICooldownEntry.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Character/SoCharacterStrike.h"
#include "Character/SoCharacter.h"
#include "Basic/Helpers/SoStaticHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownEntry::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownEntry::Setup(int32 InIndex, const UObject* InObjectWithCooldown, float InCounter)
{
	SetVisibility(ESlateVisibility::Visible);
	ObjectWithCooldown = InObjectWithCooldown;
	RemainingTime = InCounter;
	Index = InIndex;

	if (ObjectWithCooldown != nullptr)
	{
		Line->GetDynamicMaterial()->SetScalarParameterValue(PercentName, RemainingTime / ISoCooldown::Execute_GetCooldownDuration(ObjectWithCooldown));
		Icon->SetBrushFromTexture(ISoCooldown::Execute_GetCooldownIcon(ObjectWithCooldown));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownEntry::Setup(const USoUICooldownEntry* Reference)
{
	Setup(Reference->GetIndex(), Reference->GetObjectWithCooldown(), Reference->GetRemainingTime());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoUICooldownEntry::ClearAndHide()
{
	ObjectWithCooldown = nullptr;
	RemainingTime = -1.0f;
	Index = -1;
	StopAllAnimations();

	SetVisibility(ESlateVisibility::Hidden);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoUICooldownEntry::Update()
{
	if (ASoCharacter* Character = USoStaticHelper::GetPlayerCharacterAsSoCharacter(this))
	{
		const TArray<FSoCooldownCounter>& Cooldowns = Character->GetCooldowns();
		if (Cooldowns.IsValidIndex(Index) && Cooldowns[Index].Object == ObjectWithCooldown && ObjectWithCooldown != nullptr)
		{
			RemainingTime = FMath::Max(Cooldowns[Index].Counter, 0.0f);
			Line->GetDynamicMaterial()->SetScalarParameterValue(PercentName, RemainingTime / ISoCooldown::Execute_GetCooldownDuration(ObjectWithCooldown));
			return true;
		}
	}
	return false;
}
