// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "SoItemTypes.generated.h"


////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoItemType : uint8
{
	// all melee weapons
	EIT_Weapon			UMETA(DisplayName = "Weapon"),
	
	EIT_Shard			UMETA(DisplayName = "Shard"),

	// can be equipped to the "item0-2" slots
	EIT_UsableItem		UMETA(DisplayName = "UsableItem"),

	EIT_Jewelry			UMETA(DisplayName = "Jewelry"),
	
	// books, keys, quest items
	EIT_QuestItem		UMETA(DisplayName = "QuestItem"),
	EIT_Key				UMETA(DisplayName = "Key"),
	EIT_MemoryStone		UMETA(DisplayName = "MemoryStone"),
	
	EIT_RuneStone		UMETA(DisplayName = "RuneStone"),

	EIT_MAX				UMETA(DisplayName = "Max"),
};

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoWeaponType : uint8
{
	EWT_OneHanded			UMETA(DisplayName = "OneHanded"),
	EWT_DualWield			UMETA(DisplayName = "DualWield"),
	EWT_TwoHanded			UMETA(DisplayName = "TwoHanded"),

	EWT_MAX					UMETA(Hidden),
};

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoShardType : uint8
{
	Offensive		UMETA(DisplayName = "Offensive"),
	Defensive		UMETA(DisplayName = "Defensive"),
	Special			UMETA(DisplayName = "Special"),

	NumOf				UMETA(Hidden),
};


////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoUsableItemType : uint8
{
	// switch to drop mode on use
	EUIT_Throwable				UMETA(DisplayName = "Throwable"),
	// drink anim on use
	EUIT_Potion					UMETA(DisplayName = "Potion"),
	
	EUIT_Spell					UMETA(DisplayName = "Spell"),

	EUIT_Hammer					UMETA(DisplayName = "Hammer"),

	// fast, needs bullets, they fly horizontal
	EUIT_Crossbow				UMETA(DisplayName = "Crossbow"),
	
	EUIT_MAX					UMETA(Hidden),
};

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoQuestItemType : uint8
{
	// can be used to open things /o.o/
	EQIT_Key				UMETA(DisplayName = "Key"),
	EQIT_KeyLikeItem		UMETA(DisplayName = "KeyLikeItem"),
	EQIT_Item				UMETA(DisplayName = "Item"),
	EQIT_MemoryStone		UMETA(DisplayName = "MemoryStone"),

	// can be read
	EQIT_Book				UMETA(Hidden),

	EQIT_MAX				UMETA(Hidden),
};


////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoItemSlot : uint8
{
	EIS_ShardDefensive0		UMETA(DisplayName = "Defensive0"),
	EIS_ShardDefensive1		UMETA(DisplayName = "Defensive1"),
	EIS_ShardOffensive0		UMETA(DisplayName = "Offensive0"),
	EIS_ShardOffensive1		UMETA(DisplayName = "Offensive1"),
	EIS_ShardSpecial		UMETA(DisplayName = "Special"),

	// if the amount of item slots changes make sure that the related codes in SoPlayerCharacterSheet are updated properly
	// (Soulkeeper handling, item toggle)
	EIS_Item0		UMETA(DisplayName = "Item0"),
	EIS_Item1		UMETA(DisplayName = "Item1"),
	// EIS_Item2		UMETA(DisplayName = "Item2"),

	EIS_Weapon0		UMETA(DisplayName = "Weapon0"),
	EIS_Weapon1		UMETA(DisplayName = "Weapon1"),
	// EIS_Weapon2		UMETA(DisplayName = "Weapon2"),

	EIS_Max			UMETA(DisplayName = "Max"),
};


////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoItemParamType : uint8
{
	EIPT_NiceStuff			UMETA(DisplayName = "NiceStuff"),
	EIPT_NotNiceStuff		UMETA(DisplayName = "NotNiceStuff"),
	EIPT_MAX				UMETA(Hidden),
};

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoItemParam : uint8
{
	// ints
	EIP_Health				UMETA(DisplayName = "Health"),
	EIP_SpellCapacityCost	UMETA(DisplayName = "SpellCapacityCost"),
	EIP_SpellUsageCount		UMETA(DisplayName = "SpellUsageCount"),

	// floats
	EIP_AttackSpeed			UMETA(DisplayName = "AttackSpeed"),
	EIP_Cooldown			UMETA(DisplayName = "Cooldown"),

	EIP_MAX					UMETA(Hidden),
};

////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoItemParamBool : uint8
{
	EIPB_SpellOnlyInAir				UMETA(DisplayName = "SpellOnlyInAir"),
	EIPB_SpellRestricted			UMETA(DisplayName = "SpellRestricted"),
	EIPB_CooldownStopsInAir			UMETA(DisplayName = "CooldownStopsInAir"),

	EIP_MAX					UMETA(Hidden),
};


////////////////////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class ESoItemPropertyBlock : uint8
{
	EIPB_Damage			UMETA(DisplayName = "Damage"),
	EIPB_Effect			UMETA(DisplayName = "Effect"),
	EIPB_SideEffect		UMETA(DisplayName = "SideEffect"),

	EIPB_Max			UMETA(Hidden),
};
