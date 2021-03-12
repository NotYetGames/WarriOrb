// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#include "SoGameSingleton.h"

#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Particles/ParticleSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WARRIORB_WITH_VIDEO_DEMO
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "FileMediaSource.h"
#endif // WARRIORB_WITH_VIDEO_DEMO

#include "IO/DlgConfigParser.h"

#include "FMODEvent.h"
#include "FMODBus.h"
#include "FMODVCA.h"

#include "Basic/Helpers/SoStaticHelper.h"
#include "Basic/Helpers/SoStringHelper.h"
#include "Basic/SoGameMode.h"
#include "Effects/SoEffectBase.h"
#include "Settings/SoGameSettings.h"
#include "Settings/Input/SoInputHelper.h"
#include "SplineLogic/SoEditorGameInterface.h"


DEFINE_LOG_CATEGORY_STATIC(LogSoSingleton, All, All);

// https://stackoverflow.com/questions/27490858/how-can-you-compare-two-character-strings-statically-at-compile-time/27491087#27491087
constexpr bool strings_equal(char const * a, char const * b) {
	return *a == *b && (*a == '\0' || strings_equal(a + 1, b + 1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameSingleton::USoGameSingleton(const FObjectInitializer& ObjectInitializer)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameSingleton::~USoGameSingleton()
{
}


#if WITH_EDITOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty != nullptr ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// Regenerate DependentLevelMap
	// if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, DependentLevels))
	// {
	// 	DependentLevelMap.Empty();
	//
	// 	for (FSoDependentLevelDesc& DependentDesc : DependentLevels)
	// 	{
	// 		// Key
	// 		const FName Name = FName(*DependentDesc.Level.GetAssetName());
	//
	// 		// Value
	// 		TArray<FName> List;
	// 		for (const FSoftObjectPath& Ref : DependentDesc.ClaimedLevels)
	// 			if (Ref.IsValid())
	// 				List.Add(FName(*Ref.GetAssetName()));
	//
	// 		if (DependentLevelMap.Contains(Name))
	// 			DependentDesc.Level = FSoftObjectPath("");
	// 		else
	// 			DependentLevelMap.Add(Name, FSoDependentLevels{ List });
	// 	}
	// }

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, InputKeyboardTexturesMap))
		UpdateRuntimeInputKeyboardTextureMap();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, InputGamepadTexturesMap))
		UpdateRuntimeInputGamepadTextureMap();

	UpdateEnemyData();
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		UE_LOG(LogSoSingleton, Log, TEXT("PostInitProperties: UpdateRuntimeData"));
		UpdateRuntimeTextData();
		UpdateRuntimeStreamData();
		UpdateRuntimeInputKeyboardTextureMap();
		UpdateRuntimeInputGamepadTextureMap();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
USoGameSingleton* USoGameSingleton::GetInstance()
{
	if (!GEngine)
		return nullptr;

	USoGameSingleton* Instance = Cast<USoGameSingleton>(GEngine->GameSingleton);
#if WITH_EDITOR
	if (Instance == nullptr)
	{
		UE_LOG(LogSoSingleton, Error, TEXT("Set the game singleton class to USoGameSingleton in \
											Project Settings->Engine->General(Show advanced stuff has to be checked"));
	}
#endif // WITH_EDITOR

	return Instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// const FSoDependentLevels* USoGameSingleton::GetDependentLevels(FName ClaimedLevelName)
// {
// 	const USoGameSingleton* Singleton = GetInstance();
// 	if (!Singleton)
// 		return nullptr;
//
// 	return Singleton->DependentLevelMap.Find(ClaimedLevelName);
// }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FName USoGameSingleton::GetDefaultSkyPresetFromSplineName(const FString& SplineName, bool bStartSide)
{
	FString PreTag = "";
	if (!bStartSide && SplineName.Len() > 6 && SplineName.Mid(2, 2).Equals("To"))
		PreTag = SplineName.Mid(4, 2);
	else
		PreTag = SplineName.Left(2);

	const FSoLevelParams* DataPtr = Get().LevelDataForTags.Find(PreTag);
	if (DataPtr != nullptr)
		return DataPtr->SkyPresetName;

	return NAME_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoGameSingleton::GetTextForInputActionName(const FName ActionName)
{
	if (ActionName != NAME_None)
		return FSoInputActionName::GetTextForActionName(ActionName);

	return FText::GetEmpty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FText USoGameSingleton::GetTextForInputActionNameType(ESoInputActionNameType ActionNameType)
{
	const FName ActionName = FSoInputActionName::ActionNameTypeToActionName(ActionNameType);
	return GetTextForInputActionName(ActionName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoGameSingleton::GetIconForUICommand(ESoUICommand Command, ESoInputDeviceType DeviceType, const FSoInputUICommandPriorities& Priority)
{
	UTexture2D* FoundIcon = nullptr;
	const auto& GameSettings = USoGameSettings::Get();
	const FName ActionName = FSoInputActionName::UICommandToActionName(Command);
	if (ActionName != NAME_None)
	{
		// Query input settings to find out the key used
		if (DeviceType == ESoInputDeviceType::Keyboard)
		{
			// keyboard
			const TArray<FInputActionKeyMapping> ActionMappings = GameSettings.GetKeyboardInputActionMappingsForActionName(ActionName, false);
			const int32 ActionIndex = ActionMappings.IsValidIndex(Priority.KeyboardPriority) ? Priority.KeyboardPriority : 0;

			if (ActionMappings.IsValidIndex(ActionIndex))
			{
				const FInputActionKeyMapping KeyMapping = ActionMappings[ActionIndex];
				FSoInputKeyboardKeyTextures Textures;
				if (GetKeyboardKeyTexturesFromInputActionKeyMapping(KeyMapping, Textures))
					FoundIcon = Textures.GetTexture(true);
			}
		}
		else
		{
			// gamepad
			const TArray<FInputActionKeyMapping> ActionMappings = GameSettings.GetGamepadInputActionMappingsForActionName(ActionName, false);
			const int32 ActionIndex = ActionMappings.IsValidIndex(Priority.GamepadPriority) ? Priority.GamepadPriority : 0;

			if (ActionMappings.IsValidIndex(ActionIndex))
			{
				const FInputActionKeyMapping KeyMapping = ActionMappings[ActionIndex];
				FSoInputGamepadKeyTextures Textures;
				if (GetGamepadKeyTexturesFromInputActionKeyMapping(KeyMapping, false, Textures))
					FoundIcon = Textures.GetTextureForDeviceType(DeviceType);
			}
		}
	}

	return FoundIcon != nullptr ? FoundIcon : Get().InputInvalidIconPtr.Get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoGameSingleton::GetIconForInputActionNameType(ESoInputActionNameType ActionNameType, ESoInputDeviceType DeviceType, bool bPressed)
{
	const FName ActionName = FSoInputActionName::ActionNameTypeToActionName(ActionNameType);
	if (ActionName == NAME_None)
	{
		UE_LOG(LogSoSingleton,
			Error,
			TEXT("GetIconForInputActionNameType could not get ActionName != NAME_None and valid GameSettings for ActionNameType = %d"),
			static_cast<int32>(ActionNameType));
	}

	return GetIconForInputActionName(ActionName, DeviceType, bPressed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoGameSingleton::GetIconForInputActionName(FName ActionName, ESoInputDeviceType DeviceType, bool bPressed)
{
	UTexture2D* FoundIcon = nullptr;
	const auto& GameSettings = USoGameSettings::Get();

	// Query input settings to find out the key used
	if (ActionName != NAME_None)
	{
		FInputActionKeyMapping KeyMapping;
		if (DeviceType == ESoInputDeviceType::Keyboard)
		{
			// keyboard
			const TArray<FInputActionKeyMapping> ActionMappings = GameSettings.GetKeyboardInputActionMappingsForActionName(ActionName, false);
			if (USoInputHelper::GetFirstInputActionKeyMapping(ActionMappings, KeyMapping))
			{
				FSoInputKeyboardKeyTextures Textures;
				if (GetKeyboardKeyTexturesFromInputActionKeyMapping(KeyMapping, Textures))
					FoundIcon = Textures.GetTexture(bPressed);
			}
		}
		else
		{
			// gamepad
			const TArray<FInputActionKeyMapping> ActionMappings = GameSettings.GetGamepadInputActionMappingsForActionName(ActionName, false);
			if (USoInputHelper::GetFirstInputActionKeyMapping(ActionMappings, KeyMapping))
			{
				FSoInputGamepadKeyTextures Textures;
				if (GetGamepadKeyTexturesFromInputActionKeyMapping(KeyMapping, bPressed, Textures))
					FoundIcon = Textures.GetTextureForDeviceType(DeviceType);
			}
		}
	}

	if (FoundIcon == nullptr)
	{
		UE_LOG(LogSoSingleton,
			Warning,
			TEXT("GetIconForInputActionNameType could not find a valid icon for ActionName = `%s` and DeviceType = %d"),
			*ActionName.ToString(), static_cast<int32>(DeviceType));
		return Get().InputInvalidIconPtr.Get();
	}

	return FoundIcon;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoGameSingleton::GetIconForInputKey(FKey Key, ESoInputDeviceType DeviceType, bool bPressed)
{
	if (!Key.IsValid())
		return nullptr;

	return GetIconForInputKeyName(Key.GetFName(), DeviceType, bPressed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoGameSingleton::GetIconForInputKeyName(FName KeyName, ESoInputDeviceType DeviceType, bool bPressed)
{
	UTexture2D* FoundIcon = nullptr;
	if (KeyName != NAME_None)
	{
		if (DeviceType == ESoInputDeviceType::Keyboard)
		{
			// keyboard
			FSoInputKeyboardKeyTextures Textures;
			if (GetKeyboardKeyTexturesFromKeyName(KeyName, Textures))
				FoundIcon = Textures.GetTexture(bPressed);
		}
		else
		{
			// gamepad
			FSoInputGamepadKeyTextures Textures;
			if (GetGamepadKeyTexturesFromKeyName(KeyName, bPressed, Textures))
				FoundIcon = Textures.GetTextureForDeviceType(DeviceType);
		}
	}

	return FoundIcon;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoGameSingleton::GetIconForDamageType(ESoDmgType DamageType)
{
	const auto& Singleton = Get();
	return Singleton.DamageTypesTexturesMapPtr.Contains(DamageType) ? Singleton.DamageTypesTexturesMapPtr.Find(DamageType)->Get() : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSingleton::GetKeyboardKeyTexturesFromKeyName(FName KeyName, FSoInputKeyboardKeyTextures& OutTextures)
{
	if (const FSoInputKeyboardKeyTextures* Textures = Get().InputKeyboardTexturesMap.Find(KeyName))
	{
		OutTextures = *Textures;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSingleton::GetGamepadKeyTexturesFromKeyName(FName KeyName, bool bSpecialHighlight, FSoInputGamepadKeyTextures& OutTexture)
{
	const auto& Singleton = Get();

	// Highlight
	if (bSpecialHighlight)
	{
		if (const FSoInputGamepadKeyTextures * Texture = Singleton.HighlightedGamepadTexturesMap.Find(KeyName))
		{
			OutTexture = *Texture;
			return true;
		}
	}

	// Normal
	if (const FSoInputGamepadKeyTextures * Texture = Singleton.InputGamepadTexturesMap.Find(KeyName))
	{
		OutTexture = *Texture;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSingleton::GetEnemyGroupDescription(const UObject* WorldContextObject, FName InName, FSoEnemyGroupDesc& OutDesc)
{
	if (WorldContextObject == nullptr)
		return false;

	const auto& Singleton = Get();
	const FName MapName = WorldContextObject->GetWorld()->GetFName();
	const FSoEnemyGroupDescList* CurrentMapDescriptions = Singleton.EnemiesDescriptionPerMap.Find(MapName);
	if (CurrentMapDescriptions == nullptr)
		return false;

	const FSoEnemyGroupDesc* GroupDescription = CurrentMapDescriptions->GroupDefeatedTexts.Find(InName);
	if (GroupDescription == nullptr)
		return false;

	OutDesc = *GroupDescription;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FLinearColor& USoGameSingleton::GetColorForDamageType(ESoDmgType DmgType, bool bAgainstEnemy)
{
	const auto& Singleton = Get();
	if (bAgainstEnemy)
	{
		if (DmgType == ESoDmgType::Physical)
			return Singleton.DamageColorPhysical;

		return Singleton.DamageColorMagic;
	}

	const FLinearColor* Color = Singleton.DamageTypeColors.Find(DmgType);
	if (Color != nullptr)
		return *Color;

	return FLinearColor::White;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FLinearColor& USoGameSingleton::GetColorForItemRarity(int32 RarityValue)
{
	const auto& Singleton = Get();
	if (Singleton.ItemRarityColors.IsValidIndex(RarityValue))
		return Singleton.ItemRarityColors[RarityValue];

	return FLinearColor::White;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UTexture2D* USoGameSingleton::GetBackgroundIconForItemType(ESoItemType ItemType)
{
	const auto& Singleton = Get();
	if (Singleton.ItemBackgroundColorsMapPtr.Contains(ItemType))
		return Singleton.ItemBackgroundColorsMapPtr.Find(ItemType)->Get();

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UFMODEvent* USoGameSingleton::GetSFX(ESoSFX SFX)
{
	const auto& Singleton = Get();
	if (Singleton.SFXMapPtr.Contains(SFX))
		return Singleton.SFXMapPtr.Find(SFX)->Get();

	UE_LOG(LogSoSingleton, Warning, TEXT("GetSFX: SFX = %s does not exist"), *USoStringHelper::SFXToString(SFX));
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const FText& USoGameSingleton::GetEnemyVoiceText(UFMODEvent* Voice)
{
	for (const FSoVoiceEntryRandomText& Entry : Get().VoiceTextsArrayRandom)
		if (Entry.EventPtr == Voice && Entry.Texts.Num() > 0)
		{
			return Entry.Texts[FMath::RandHelper(Entry.Texts.Num())];
		}

	for (const FSoVoiceEntry& Entry : Get().VoiceTextsArray)
		if (Entry.EventPtr == Voice)
			return Entry.Text;

	return FText::GetEmpty();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UFMODEvent* USoGameSingleton::GetWallHitSFXFromPhysicalMaterial(UPhysicalMaterial* PhysicalMaterial)
{
	const auto& Singleton = Get();
	if (Singleton.WallHitSFXMapPtr.Contains(PhysicalMaterial))
		return Singleton.WallHitSFXMapPtr.Find(PhysicalMaterial)->Get();

	return Singleton.WallHitSFXInvalidPtr.Get();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UParticleSystem* USoGameSingleton::GetWallHitVFXFromPhysicalMaterial(UPhysicalMaterial* PhysicalMaterial)
{
	const auto& Singleton = Get();
	if (Singleton.WallHitVFXMapPtr.Contains(PhysicalMaterial))
		return Singleton.WallHitVFXMapPtr.Find(PhysicalMaterial)->Get();

	return Singleton.WallHitVFXInvalidPtr.Get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::UpdateRuntimeTextData()
{
#define LOCTEXT_NAMESPACE "ItemsCommon"
	ItemTypeTexts.SetNum(static_cast<int32>(ESoItemType::EIT_MAX) + 1);
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_Shard)] = LOCTEXT("shard", "Shard");
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_UsableItem)] = LOCTEXT("usable", "Usable");
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_QuestItem)] = LOCTEXT("QuestItem", "Quest Item");
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_MemoryStone)] = LOCTEXT("MemoryStone", "Memory Stone");
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_Key)] = LOCTEXT("key", "Key");
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_RuneStone)] = LOCTEXT("RuneStone", "Runestone");
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_Weapon)] = LOCTEXT("weapon", "Weapon");
	ItemTypeTexts[static_cast<int32>(ESoItemType::EIT_Jewelry)] = LOCTEXT("jewelry", "Valuable");

	ShardTypeTexts.SetNum(static_cast<int32>(ESoShardType::NumOf) + 1);
	ShardTypeTexts[static_cast<int32>(ESoShardType::Defensive)] = LOCTEXT("defensive", "Shard of Protection");
	ShardTypeTexts[static_cast<int32>(ESoShardType::Offensive)] = LOCTEXT("offensive", "Shard of Justice");
	ShardTypeTexts[static_cast<int32>(ESoShardType::Special)] = LOCTEXT("special", "Shard of Wonder");
	ShardTypeTexts[static_cast<int32>(ESoShardType::NumOf)] = LOCTEXT("invalid_shard_max", "Invalid");

	WeaponTypeTexts.SetNum(static_cast<int32>(ESoWeaponType::EWT_MAX) + 1);
	WeaponTypeTexts[static_cast<int32>(ESoWeaponType::EWT_OneHanded)] = LOCTEXT("onehanded", "One-Handed Weapon");
	WeaponTypeTexts[static_cast<int32>(ESoWeaponType::EWT_TwoHanded)] = LOCTEXT("twohanded", "Two-Handed Weapon");
	WeaponTypeTexts[static_cast<int32>(ESoWeaponType::EWT_DualWield)] = LOCTEXT("dualwield", "Dual Wield Weapon");
	WeaponTypeTexts[static_cast<int32>(ESoWeaponType::EWT_MAX)] = LOCTEXT("invalid_weapon_max", "Invalid Weapon");

	UsableTypeTexts.SetNum(static_cast<int32>(ESoUsableItemType::EUIT_MAX) + 1);
	UsableTypeTexts[static_cast<int32>(ESoUsableItemType::EUIT_Potion)] = LOCTEXT("potion", "Something to drink");
	UsableTypeTexts[static_cast<int32>(ESoUsableItemType::EUIT_Throwable)] = LOCTEXT("throwable", "Something to throw");
	UsableTypeTexts[static_cast<int32>(ESoUsableItemType::EUIT_Spell)] = LOCTEXT("spell", "Something to cast with");
	UsableTypeTexts[static_cast<int32>(ESoUsableItemType::EUIT_Crossbow)] = LOCTEXT("crossbow", "Something to shoot with");

	QuestItemTypeTexts.SetNum(static_cast<int32>(ESoQuestItemType::EQIT_MAX) + 1);
	QuestItemTypeTexts[static_cast<int32>(ESoQuestItemType::EQIT_Key)] = LOCTEXT("something_unlock", "Something to unlock with");
	QuestItemTypeTexts[static_cast<int32>(ESoQuestItemType::EQIT_KeyLikeItem)] = LOCTEXT("keylikeitem", "Something interesting");
	QuestItemTypeTexts[static_cast<int32>(ESoQuestItemType::EQIT_Item)] = LOCTEXT("item", "Something interesting");
	QuestItemTypeTexts[static_cast<int32>(ESoQuestItemType::EQIT_MemoryStone)] = LOCTEXT("MemoryStone", "Memory Stone");
	QuestItemTypeTexts[static_cast<int32>(ESoQuestItemType::EQIT_Book)] = LOCTEXT("book", "Something to read");

	ItemParamTexts.SetNum(static_cast<int32>(ESoItemParam::EIP_MAX) + 1);
	ItemParamTexts[static_cast<int32>(ESoItemParam::EIP_AttackSpeed)] = LOCTEXT("speed", "Attack Speed");
	ItemParamTexts[static_cast<int32>(ESoItemParam::EIP_Health)] = LOCTEXT("health", "Health");
	ItemParamTexts[static_cast<int32>(ESoItemParam::EIP_SpellCapacityCost)] = LOCTEXT("spellcapacitycost", "Capacity Cost");
	ItemParamTexts[static_cast<int32>(ESoItemParam::EIP_SpellUsageCount)] = LOCTEXT("spellusagecount", "Charge");
	ItemParamTexts[static_cast<int32>(ESoItemParam::EIP_Cooldown)] = LOCTEXT("cooldown", "Cooldown");
#undef LOCTEXT_NAMESPACE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::UpdateEnemyDataForCurrentMap()
{
#if WITH_EDITOR
	if (GEditor == nullptr)
		return;

	const FString Path = FSoEditorGameInterface::GetEnemyGroupDataConfigPath(GEditor->GetEditorWorldContext().World());
	if (!FPaths::FileExists(Path))
	{
		return;
	}

	// Read config for current map
	FSoEnemyGroups GroupData;
	FDlgConfigParser ConfigParser("So");
	ConfigParser.InitializeParser(Path);
	ConfigParser.ReadAllProperty(FSoEnemyGroups::StaticStruct(), &GroupData);

	const FName MapName = GEditor->GetEditorWorldContext().World()->GetFName();
	FSoEnemyGroupDescList* CurrentMapDescriptions = EnemiesDescriptionPerMap.Find(MapName);
	if (CurrentMapDescriptions == nullptr)
		CurrentMapDescriptions = &EnemiesDescriptionPerMap.Add(MapName);

	// 1. Iterate through enemy groups, add missing entries or move them back from trash
	for (const auto& Pair : GroupData.EnemyGroupMap)
	{
		const FName EnemyGroupName = Pair.Key;

		// Already contains the group name, continue
		if (CurrentMapDescriptions->GroupDefeatedTexts.Contains(EnemyGroupName))
			continue;

		// Does not contains the Group Name
		auto* Unmatched = CurrentMapDescriptions->GroupDefeatedTexts_Unmatched.Find(EnemyGroupName);
		if (Unmatched != nullptr)
		{
			// Has an unmatched text, copy it to to the matched ones and delete it from unmatched
			CurrentMapDescriptions->GroupDefeatedTexts.Add(Pair.Key, *Unmatched);
			CurrentMapDescriptions->GroupDefeatedTexts_Unmatched.Remove(EnemyGroupName);
		}
		else
		{
			// Add it to matched
			CurrentMapDescriptions->GroupDefeatedTexts.Add(EnemyGroupName);
		}
	}

	// 2. Iterate through data, move obsolete entries to trash, update text namespaces and keys
	TArray<FName> NamesToRemove;
	for (auto& Pair : CurrentMapDescriptions->GroupDefeatedTexts)
	{
		auto* EnemyNumPtr = GroupData.EnemyGroupMap.Find(Pair.Key);
		if (EnemyNumPtr == nullptr)
		{
			CurrentMapDescriptions->GroupDefeatedTexts_Unmatched.Add(Pair.Key, Pair.Value);
			NamesToRemove.Add(Pair.Key);
		}
	}
	for (auto Name : NamesToRemove)
		CurrentMapDescriptions->GroupDefeatedTexts.Remove(Name);

#endif // WITH_EDITOR
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::UpdateEnemyData()
{
	UpdateEnemyDataForCurrentMap();
	static const FString TextNamespace(TEXT("EnemyGroupData"));

	// Update Namespace and key
	for (auto& MapKeyValue : EnemiesDescriptionPerMap)
	{
		const FName MapName = MapKeyValue.Key;
		FSoEnemyGroupDescList& Descriptions =  MapKeyValue.Value;
		for (auto& GroupKeyValue : Descriptions.GroupDefeatedTexts)
		{
			const FName GroupName = GroupKeyValue.Key;
			auto& GroupDescription = GroupKeyValue.Value;
			GroupDescription.DisplayName = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
				*GroupDescription.DisplayName.ToString(),
				*TextNamespace,
				*(MapName.ToString() + "_" + GroupName.ToString()  + "_name")
			);
			GroupDescription.Description = FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(
				*GroupDescription.Description.ToString(),
				*TextNamespace,
				*(MapName.ToString() + "_" + GroupName.ToString()  + "_desc")
			);
		}

		// Make the unmatched texts unlocalizable
		for (auto& GroupKeyValue : Descriptions.GroupDefeatedTexts_Unmatched)
		{
			auto& GroupDescription = GroupKeyValue.Value;
			GroupDescription.DisplayName = FText::AsCultureInvariant(GroupDescription.DisplayName);
			GroupDescription.Description = FText::AsCultureInvariant(GroupDescription.Description);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::UpdateRuntimeStreamData()
{
	// Load the values from the reference into something that is garbage collected
	// References:
	// - https://jdelezenne.github.io/Codex/UE4/Assets%20Streaming.html
	// - https://docs.unrealengine.com/en-US/Programming/Assets/AsyncLoading/index.html
	PersistentObjects.Empty();

	// NOTE: could bManageActiveHandle = True and keep the handles alive instead of keeping the UObject :thinking:

	// LevelsWeatherVFXArray
	{
		TArray<FSoftObjectPath> ObjectPathsToStream;
		for (const auto& Value : LevelsWeatherVFXArray)
		{
			ObjectPathsToStream.Add(Value.ParticlesPtr.ToSoftObjectPath());
		}
		StreamableManager.RequestAsyncLoad(ObjectPathsToStream, [this]()
		{
			for (const auto& Value : LevelsWeatherVFXArray)
			{
				AddToPersistentObjects(Value.ParticlesPtr.Get());
			}
		});
	}

	RequestAsyncLoadMapValues(ItemBackgroundColorsMapPtr);
	RequestAsyncLoadMapValues(DamageTypesTexturesMapPtr);
	RequestAsyncLoadValue(InputInvalidIconPtr);

	//
	// SFX & Audio
	//

	// SFXMapPtr
	RequestAsyncLoadMapValues(SFXMapPtr);

	// VoiceTextsArray
	{
		TArray<FSoftObjectPath> ObjectPathsToStream;
		for (const auto& Value : VoiceTextsArray)
		{
			ObjectPathsToStream.Add(Value.EventPtr.ToSoftObjectPath());
		}
		StreamableManager.RequestAsyncLoad(ObjectPathsToStream, [this]()
		{
			for (const auto& Value : VoiceTextsArray)
			{
				if (Value.EventPtr.Get() != nullptr)
					AddToPersistentObjects(Value.EventPtr.Get());
				else
					UE_LOG(LogSoSingleton, Error, TEXT("Invalid Object in USoGameSingleton::VoiceTextsArray"));
			}
		});
	}
	RequestAsyncLoadValue(WallHitSFXInvalidPtr);
	RequestAsyncLoadMapKeysAndValues(WallHitSFXMapPtr);
	RequestAsyncLoadValue(MenuMusicPtr);
	RequestAsyncLoadValue(LoadingMusicPtr);
	RequestAsyncLoadValue(VCA_MasterPtr);
	RequestAsyncLoadValue(VCA_SFXPtr);
	RequestAsyncLoadValue(VCA_UIPtr);
	RequestAsyncLoadValue(VCA_MusicPtr);
	RequestAsyncLoadValue(VCA_MutePtr);
	RequestAsyncLoadArray(MenuBusesToPauseArrayPtr);
	RequestAsyncLoadArray(BusesToIgnoreForSpeedChangeArrayPtr);

	// VCA_Menu and VCA_AmbientDialogue
	VCA_BlendArray.SetNum(2);
	VCA_BlendArray[0] = &VCA_Menu;
	VCA_BlendArray[1] = &VCA_AmbientDialogue;
	{
		TArray<FSoftObjectPath> ObjectPathsToStream;
		for (const auto& Value : VCA_BlendArray)
		{
			ObjectPathsToStream.Add(Value->VCAPtr.ToSoftObjectPath());
		}
		StreamableManager.RequestAsyncLoad(ObjectPathsToStream, [this]()
		{
			for (const auto& Value : VCA_BlendArray)
			{
				AddToPersistentObjects(Value->VCAPtr.Get());
			}
		});
	}

	//
	// WallHit
	//

	RequestAsyncLoadValue(WallHitVFXInvalidPtr);
	RequestAsyncLoadMapKeysAndValues(WallHitVFXMapPtr);

	//
	// Media player
	//

#if WARRIORB_WITH_VIDEO_DEMO
	RequestAsyncLoadValue(DemoMediaSourcePtr);
	RequestAsyncLoadValue(VideoMediaPlayerPtr);
	RequestAsyncLoadValue(VideoMediaTexturePtr);
#endif // WARRIORB_WITH_VIDEO_DEMO
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ValueType>
void USoGameSingleton::RequestAsyncLoadValue(const TSoftObjectPtr<ValueType>& Value)
{
	const FSoftObjectPath ValuePath = Value.ToSoftObjectPath();
	if (!ValuePath.IsValid())
		return;

	StreamableManager.RequestAsyncLoad(ValuePath, [this, ValuePath]()
	{
		AddToPersistentObjects(ValuePath.ResolveObject());
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ValueType>
void USoGameSingleton::RequestAsyncLoadArray(const TArray<TSoftObjectPtr<ValueType>>& Array)
{
	TArray<FSoftObjectPath> ObjectPathsToStream;
	for (const auto& Value : Array)
	{
		const FSoftObjectPath ValuePath = Value.ToSoftObjectPath();
		if (ValuePath.IsValid())
			ObjectPathsToStream.Add(ValuePath);
	}
	StreamableManager.RequestAsyncLoad(ObjectPathsToStream, [this, ObjectPathsToStream]()
	{
		for (const auto& Path : ObjectPathsToStream)
		{
			AddToPersistentObjects(Path.ResolveObject());
		}
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename KeyType, typename ValueType>
void USoGameSingleton::RequestAsyncLoadMapValues(const TMap<KeyType, TSoftObjectPtr<ValueType>>& Map)
{
	TArray<FSoftObjectPath> ObjectPathsToStream;
	for (const auto& KeyValue : Map)
	{
		const FSoftObjectPath ValuePath = KeyValue.Value.ToSoftObjectPath();
		if (ValuePath.IsValid())
			ObjectPathsToStream.Add(ValuePath);
	}

	StreamableManager.RequestAsyncLoad(ObjectPathsToStream, [this, ObjectPathsToStream]()
	{
		for (const auto& Path : ObjectPathsToStream)
		{
			AddToPersistentObjects(Path.ResolveObject());
		}
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename KeyType, typename ValueType>
void USoGameSingleton::RequestAsyncLoadMapKeysAndValues(const TMap<TSoftObjectPtr<KeyType>, TSoftObjectPtr<ValueType>>& Map)
{
	TArray<FSoftObjectPath> ObjectPathsToStream;
	for (const auto& KeyValue : Map)
	{
		const FSoftObjectPath KeyPath = KeyValue.Key.ToSoftObjectPath();
		const FSoftObjectPath ValuePath = KeyValue.Value.ToSoftObjectPath();

		if (KeyPath.IsValid())
			ObjectPathsToStream.Add(KeyPath);
		if (ValuePath.IsValid())
			ObjectPathsToStream.Add(ValuePath);
	}
	StreamableManager.RequestAsyncLoad(ObjectPathsToStream, [this, ObjectPathsToStream]()
	{
		for (const auto& Path : ObjectPathsToStream)
		{
			AddToPersistentObjects(Path.ResolveObject());
		}
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::UpdateRuntimeInputKeyboardTextureMap()
{
	// Collect current
	TMap<FName, FSoInputKeyboardKeyTextures> NewMap;
	const TArray<FKey>& KeyboardKeys = FSoInputKey::GetAllKeyboardKeys();
	check(KeyboardKeys.Num() > 0);

	for (const FKey Key : KeyboardKeys)
	{
		FSoInputKeyboardKeyTextures Temp;
		NewMap.Add(Key.GetFName(), Temp);
	}

	// Restore mappings to the user set ones
	for (auto& Elem : InputKeyboardTexturesMap)
	{
		if (NewMap.Contains(Elem.Key))
		{
			NewMap.FindChecked(Elem.Key) = Elem.Value;
		}
	}

	// Update
	InputKeyboardTexturesMap = NewMap;
	check(InputKeyboardTexturesMap.Num() > 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void USoGameSingleton::UpdateRuntimeInputGamepadTextureMap()
{
	// Collect current
	TMap<FName, FSoInputGamepadKeyTextures> NewMap;
	const TArray<FKey>& GamepadKeys = FSoInputKey::GetAllGamepadKeys();
	check(GamepadKeys.Num() > 0)

	for (const FKey Key : GamepadKeys)
	{
		FSoInputGamepadKeyTextures Temp;
		NewMap.Add(Key.GetFName(), Temp);
	}

	// Restore Mappings to the user set one
	for (const auto& Elem : InputGamepadTexturesMap)
		if (NewMap.Contains(Elem.Key))
			NewMap.FindChecked(Elem.Key) = Elem.Value;

	// Update
	InputGamepadTexturesMap = NewMap;
	check(InputGamepadTexturesMap.Num() > 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool USoGameSingleton::IsBlacklistedGPUName(const FString& Name)
{
	for (const FString& BlacklistedName : Get().BlacklistedGPUNames)
	{
		if (Name.Contains(BlacklistedName))
		{
			return true;
		}
	}

	return false;
}
