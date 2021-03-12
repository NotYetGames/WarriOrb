// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTemplate.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "Logic/SoCooldown.h"
#include "SoItemTemplateRuneStone.generated.h"

class UFMODEvent;
class UTexture2D;
class USoEffectBase;

/**
*
*/
UCLASS()
class SORB_API USoItemTemplateRuneStone : public USoItemTemplate, public ISoCooldown
{
	GENERATED_BODY()

public:

	USoItemTemplateRuneStone() { ItemType = ESoItemType::EIT_RuneStone; }

	float GetCooldownDuration_Implementation() const override { return Cooldown; }
	UTexture2D* GetCooldownIcon_Implementation() const override { return Icon; }
	bool CanCountDownInAir_Implementation() const override { return bCooldownReducedInAir; }

	const TSubclassOf<USoEffectBase>& GetEffect() const { return Effect; }

	void FillTooltipProperties(UObject* Tooltip) const override;
	void UpdateLocalizedFields() override;


	UFUNCTION(BlueprintPure)
	const FText& GetDescForLevel(int32 Level, bool bLevelUp) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuneStoneData)
	float Cooldown = -1.0f;

	// == 0: auto cast
	//  < 0: unlimited usage
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuneStoneData)
	int32 UsageCount = 1;

	// amount of capacity it takes from the Spellcaster if equipped
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuneStoneData)
	int32 CapacityCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuneStoneData)
	TSubclassOf<USoEffectBase> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuneStoneData)
	bool bCanBeUsedInAir = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuneStoneData)
	bool bCanBeUsedInSKFreeZone = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuneStoneData)
	bool bCooldownReducedInAir = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable)
	UFMODEvent* SFXOnUse;



	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable, Meta = (MultiLine = true))
	TArray<FText> LevelBasedDescOverride;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable, Meta = (MultiLine = true))
	TArray<FText> LevelUpBasedDescOverride;
};
