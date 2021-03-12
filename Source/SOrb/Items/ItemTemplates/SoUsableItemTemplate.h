// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTemplate.h"
#include "CharacterBase/SoIMortalTypes.h"

#include "SoUsableItemTemplate.generated.h"

class UMaterialInstance;
class UStaticMesh;
class ASoProjectile;
class USoEffectBase;
class UFMODEvent;

/**
*
*/
UCLASS()
class SORB_API USoUsableItemTemplate : public USoItemTemplate
{
	GENERATED_BODY()

public:

	USoUsableItemTemplate() { ItemType = ESoItemType::EIT_UsableItem; }

	virtual void FillTooltipProperties(UObject* Tooltip) const override;

	bool IsSlotCompatible(ESoItemSlot Slot) const override;
	ESoItemSlot GetDefaultSlot() const override { return ESoItemSlot::EIS_Item0; }

	bool IsEquipable() const override { return true; }

	bool IsStackable() const override { return StackSize > 1 || !bDestroyOnZeroAmount; }
	int32 GetMaxStackNum() const override { return StackSize; }

	bool CanBePutTogether(const USoItemTemplate* TheOther) const override;

	bool DestroyOnRunOut() const override { return bDestroyOnZeroAmount; }

	FName GetUsableItemName() const { return UsableItemName; }

	const FText& GetSubTypeText() const override;

	UStaticMesh* GetStaticMesh() const { return Mesh; }
	UMaterialInstance* GetMaterial() const { return MaterialOverride; }

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable, AssetRegistrySearchable)
	ESoUsableItemType UsableType;

	// unique identifier of each type (e.g. black grenade, test stone, etc.)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable, AssetRegistrySearchable)
	FName UsableItemName;

	// if > 1 more item can be stacked together
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable, AssetRegistrySearchable)
	int32 StackSize;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable, AssetRegistrySearchable)
	bool bDestroyOnZeroAmount = true;

	/* droppable stuff / crossbow: */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable)
	TSubclassOf<ASoProjectile> ProjectileType;

	/** .x: spline offset, .y: vertical offset */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable)
	FVector2D SpawnOffset = FVector2D(20.0f, 30.0f);

	/* for potions */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable)
	TArray<TSubclassOf<USoEffectBase>> Effects;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable)
	float HealAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable)
	TMap<ESoDmgType, int32> HealthBonus;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = ItemUsable)
	UFMODEvent* SFXOnUse;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Visual)
	UStaticMesh* Mesh;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Visual)
	UMaterialInstance* MaterialOverride;
};
