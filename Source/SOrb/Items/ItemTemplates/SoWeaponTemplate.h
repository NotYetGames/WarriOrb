// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTemplate.h"
#include "CharacterBase/SoIMortalTypes.h"
#include "Character/SoCharacterDataTypes.h"

#include "SoWeaponTemplate.generated.h"

class USoCharacterStrike;
class UStaticMesh;

/**
*
*/
UCLASS()
class SORB_API USoWeaponTemplate : public USoItemTemplate
{
	GENERATED_BODY()

public:

	USoWeaponTemplate() { ItemType = ESoItemType::EIT_Weapon; }

	virtual void PostLoad() override;

	virtual const FText& GetSubTypeText() const override;

	virtual void FillTooltipProperties(UObject* Tooltip) const override;

	ESoWeaponType GetWeaponType() const { return WeaponType; }

	// const TMap<ESoDamageType, int32>& GetDamageMap(bool bSpecialStrike) const { return bSpecialStrike ? SpecialDamage : BaseDamage; }
	const FSoDmg& GetDmg(bool bSpecial) const { return bSpecial ? SpecialDmg : BaseDmg; }

	virtual void GetStaticEffects(TArray<TSubclassOf<USoEffectBase>>& OutEffects) const override { OutEffects = StaticEffects; }

	bool IsValidStrikeIndex(int32 Index, bool bSpecial) const;
	USoCharacterStrike* GetStrike(bool bSpecial) const { return bSpecial ? SpecialStrike : PrimaryStrike; }


	ESoItemSlot GetDefaultSlot() const override { return ESoItemSlot::EIS_Weapon0; }
	bool IsSlotCompatible(ESoItemSlot Slot) const override;

	virtual bool IsEquipable() const override { return true; }

	UStaticMesh* GetStaticMeshNew() const { return MeshNew; }

	int32 GetInterruptModifier() const { return InterruptModifier; }
	float GetCriticalMultiplier() const { return CriticalMultiplier; }
	float GetAutoAimDistance() const { return AutoAimDistance; }

	const FSoAnimationSet& GetNewAnimations() const { return NewAnimations; }

	const FLinearColor& GetTrailColor() const { return TrailColor; }
	const FLinearColor& GetTrailAnimLight() const { return TrailAnimLight; }

	FName GetBoneName() const { return BoneName; }
	FName GetBoneNameSecondary() const { return BoneNameSecondary; }
	bool GetInverseRollAnimsIfArmed() const { return bInverseRollAnimsIfArmed; }

protected:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon, AssetRegistrySearchable)
	ESoWeaponType WeaponType;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	FSoDmg BaseDmg;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	FSoDmg SpecialDmg;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	TArray<TSubclassOf<USoEffectBase>> StaticEffects;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	int32 InterruptModifier;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	float CriticalMultiplier = 2.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	bool bInverseRollAnimsIfArmed = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	float AutoAimDistance = 400;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	USoCharacterStrike* PrimaryStrike;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	USoCharacterStrike* SpecialStrike;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	FSoAnimationSet NewAnimations;


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	FLinearColor TrailColor = FLinearColor(4.8f, 5.0f, 1.2f, 1.0f);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemWeapon)
	FLinearColor TrailAnimLight = FLinearColor(100.0f, 7.8f, 0.0f, 1.0f);


	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Visual)
	UStaticMesh* MeshNew;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Visual)
	FName BoneName = FName("Weapon_R");

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Visual)
	FName BoneNameSecondary = FName("Weapon_L");
};
