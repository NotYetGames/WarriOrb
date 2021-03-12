// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/StreamableManager.h"

#include "Items/SoItemTypes.h"
#include "UI/General/SoUITypes.h"

#include "Settings/Input/SoInputSettingsTypes.h"
#include "Settings/SoAudioSettingsTypes.h"
#include "Settings/Input/SoInputNames.h"
#include "Levels/SoLevelTypes.h"
#include "SaveFiles/SoWorldStateTable.h"
#include "Levels/SoEpisodeLevelTypes.h"

#include "SoGameSingleton.generated.h"

class UFMODEvent;
class UParticleSystem;
class UPhysicalMaterial;
class UFileMediaSource;
class UMediaSource;
class UMediaPlayer;
class UMediaTexture;
class UFMODVCA;
class UFMODBus;
class UTexture2D;
class ITargetPlatform;

USTRUCT(BlueprintType, Blueprintable)
struct FSoVCABlend
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, meta = (DisplayName = "VCA"))
	TSoftObjectPtr<UFMODVCA> VCAPtr;

	UPROPERTY(EditAnywhere)
	float BlendDuration = 1.5f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "1"))
	float MinValue = 0.0f;

	UPROPERTY()
	float Target = 1.0f;

	UPROPERTY()
	float Counter = 1.0f;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoCharacterProgressInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTexture2D* Image;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoEnemyGroupDesc
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Meta = (MultiLine = true))
	FText Description;
};

USTRUCT(BlueprintType, Blueprintable)
struct FSoEnemyGroupDescList
{
	GENERATED_USTRUCT_BODY()

public:
	// Key: is the group name
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FSoEnemyGroupDesc> GroupDefeatedTexts;

	// used but not used anymore, maybe we will use them later
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<FName, FSoEnemyGroupDesc> GroupDefeatedTexts_Unmatched;
};


USTRUCT(BlueprintType, Blueprintable)
struct FSoNameArray
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	TArray<FName> Names;
};


USTRUCT(BlueprintType, Blueprintable)
struct FSoVideoSubtitle
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EndTime;
};



UCLASS(Config = SoGameSingleton, DefaultConfig, Blueprintable, BlueprintType, meta = (DisplayName = "Warriorb Singleton"))
class SORB_API USoGameSingleton : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USoGameSingleton(const FObjectInitializer& ObjectInitializer);
	~USoGameSingleton();

	// UDeveloperSettings interface
	/** Gets the settings container name for the settings, either Project or Editor */
	FName GetContainerName() const override { return TEXT("Project"); }
	/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
	FName GetCategoryName() const override { return TEXT("Game"); };
	/** The unique name for your section of settings, uses the class's FName. */
	FName GetSectionName() const override { return TEXT("Warriorb Singleton"); };

#if WITH_EDITOR
	/** Gets the description for the section, uses the classes ToolTip by default. */
	FText GetSectionDescription() const override
	{
		return FText::FromString(TEXT("Configure the custom singleton settings"));
	}

	/** Whether or not this class supports auto registration or if the settings have a custom setup */
	bool SupportsAutoRegistration() const override { return true; }

	// UObject interface
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif  // WITH_EDITOR

	void PostInitProperties() override;

	// NOTE: postload is useless for the singleton
	//void PostLoad() override;

	UFUNCTION(BlueprintPure, DisplayName = "Get So Game Singleton", Category = GameSingleton)
	static USoGameSingleton* GetInstance();
	static USoGameSingleton& Get()
	{
		auto* Instance = GetInstance();
		check(IsValid(Instance));
		return *Instance;
	}

	// Saves
	static FSoStateTable& GetStateTable() { return Get().StateTable; }

	//
	// Levels
	//

	// Gets all the levels that depend ClaimedLevelName
	// static const FSoDependentLevels* GetDependentLevels(FName ClaimedLevelName);

	/** return the name of the preset for the level for the pretag, or NAME_None if there isn't any */
	static FName GetDefaultSkyPresetFromSplineName(const FString& SplineName, bool bStartSide);

	//
	// Localization
	//

	UFUNCTION(BlueprintPure, Category = Localization)
	static const FText& GetTextForItemType(ESoItemType ItemType)
	{
		return Get().ItemTypeTexts[static_cast<int32>(ItemType)];
	}

	UFUNCTION(BlueprintPure, Category = Localization)
	static const FText& GetTextForShardType(ESoShardType ShardType)
	{
		return Get().ShardTypeTexts[static_cast<int32>(ShardType)];
	}

	UFUNCTION(BlueprintPure, Category = Localization)
	static const FText& GetTextForWeaponType(ESoWeaponType WeaponType)
	{
		return Get().WeaponTypeTexts[static_cast<int32>(WeaponType)];
	}

	UFUNCTION(BlueprintPure, Category = Localization)
	static const FText& GetTextForUsableType(ESoUsableItemType UsableType)
	{
		return Get().UsableTypeTexts[static_cast<int32>(UsableType)];
	}

	UFUNCTION(BlueprintPure, Category = Localization)
	static const FText& GetTextForQuestItemType(ESoQuestItemType QuestItemType)
	{
		return Get().QuestItemTypeTexts[static_cast<int32>(QuestItemType)];
	}

	UFUNCTION(BlueprintPure, Category = Localization)
	static const FText& GetTextForItemParamType(ESoItemParam ParamType)
	{
		return Get().ItemParamTexts[static_cast<int32>(ParamType)];
	}

	UFUNCTION(BlueprintPure, Category = Localization)
	static FText GetTextForInputActionName(FName ActionName);

	UFUNCTION(BlueprintPure, Category = Localization)
	static FText GetTextForInputActionNameType(ESoInputActionNameType ActionNameType);

	//
	// Icons
	//

	UFUNCTION(BlueprintPure, Category = Icons)
	static UTexture2D* GetIconForUICommand(ESoUICommand Command, ESoInputDeviceType DeviceType, const FSoInputUICommandPriorities& Priority);

	UFUNCTION(BlueprintPure, Category = Icons)
	static UTexture2D* GetIconForInputActionNameType(ESoInputActionNameType ActionNameType, ESoInputDeviceType DeviceType, bool bPressed = true);

	UFUNCTION(BlueprintPure, Category = Icons)
	static UTexture2D* GetIconForInputActionName(FName ActionName, ESoInputDeviceType DeviceType, bool bPressed = true);

	UFUNCTION(BlueprintPure, Category = Icons)
	static UTexture2D* GetIconForInputKey(FKey Key, ESoInputDeviceType DeviceType, bool bPressed = true);

	UFUNCTION(BlueprintPure, Category = Icons)
	static UTexture2D* GetIconForInputKeyName(FName KeyName, ESoInputDeviceType DeviceType, bool bPressed = true);

	UFUNCTION(BlueprintPure, Category = Icons)
	static UTexture2D* GetIconForDamageType(ESoDmgType DamageType);

	// The default size for UI icons
	UFUNCTION(BlueprintPure, Category = Icons)
	static FORCEINLINE int32 GetDefaultWidthOrHeightForUIIcons(ESoInputDeviceType DeviceType)
	{
		return DeviceType == ESoInputDeviceType::Keyboard ? KeyboardDefaultIconSize : KeyboardDefaultIconSize * GamepadIconsMultiplier;
	}

	// Gets the proper size for the width or height depending on the device type
	UFUNCTION(BlueprintPure, Category = Icons)
	static FORCEINLINE int32 GetWidthOrHeightForUIIcons(ESoInputDeviceType DeviceType, int32 KeyboardIconWidthOrHeight)
	{
		return DeviceType == ESoInputDeviceType::Keyboard ? KeyboardIconWidthOrHeight : KeyboardIconWidthOrHeight * GamepadIconsMultiplier;
	}

	// Vector variant
	UFUNCTION(BlueprintPure, Category = Icons)
	static FORCEINLINE FVector2D GetDefaultVectorWidthHeightForUIIcons(ESoInputDeviceType DeviceType)
	{
		const int32 WidthHeight = GetDefaultWidthOrHeightForUIIcons(DeviceType);
		return FVector2D(WidthHeight, WidthHeight);
	}

	// Vector variant
	UFUNCTION(BlueprintPure, Category = Icons)
	static FORCEINLINE FVector2D GetVectorWidthHeightForUIIcons(ESoInputDeviceType DeviceType, FVector2D KeyboardIconSize)
	{
		return DeviceType == ESoInputDeviceType::Keyboard ?
			KeyboardIconSize :
			FVector2D(
				GetWidthOrHeightForUIIcons(DeviceType, FMath::TruncToInt(KeyboardIconSize.X)),
				GetWidthOrHeightForUIIcons(DeviceType, FMath::TruncToInt(KeyboardIconSize.Y)));
	}


	UFUNCTION(BlueprintPure, Category = UIIcons)
	static bool GetKeyboardKeyTexturesFromInputActionKeyMapping(const FInputActionKeyMapping& Mapping, FSoInputKeyboardKeyTextures& OutTextures)
	{
		return GetKeyboardKeyTexturesFromKeyName(Mapping.Key.GetFName(), OutTextures);
	}

	UFUNCTION(BlueprintPure, Category = UIIcons)
	static bool GetKeyboardKeyTexturesFromKeyName(FName KeyName, FSoInputKeyboardKeyTextures& OutTextures);

	UFUNCTION(BlueprintPure, Category = UIIcons)
	static bool GetGamepadKeyTexturesFromInputActionKeyMapping(const FInputActionKeyMapping& Mapping, bool bSpecialHighlight, FSoInputGamepadKeyTextures& OutTexture)
	{
		return GetGamepadKeyTexturesFromKeyName(Mapping.Key.GetFName(), bSpecialHighlight, OutTexture);
	}

	UFUNCTION(BlueprintPure, Category = UIIcons)
	static bool GetGamepadKeyTexturesFromKeyName(FName KeyName, bool bSpecialHighlight, FSoInputGamepadKeyTextures& OutTexture);

	//
	// Enemy
	//

	UFUNCTION(BlueprintPure, Category = Enemy, meta = (WorldContext = "WorldContextObject"))
	static bool GetEnemyGroupDescription(const UObject* WorldContextObject, FName InName, FSoEnemyGroupDesc& OutDesc);

	//
	// Color
	//

	UFUNCTION(BlueprintPure, Category = Color)
	static const FLinearColor& GetColorForDamageType(ESoDmgType DmgType, bool bAgainstEnemy = false);

	UFUNCTION(BlueprintPure, Category = Color)
	static const FLinearColor& GetColorForItemRarity(int32 RarityValue);

	UFUNCTION(BlueprintPure, Category = Color)
	static UTexture2D* GetBackgroundIconForItemType(ESoItemType ItemType);

	//
	// SFX & VFX
	//

	UFUNCTION(BlueprintPure, Category = SFX)
	static UFMODEvent* GetSFX(ESoSFX SFX);

	UFUNCTION(BlueprintPure, Category = SFX)
	static const FText& GetEnemyVoiceText(UFMODEvent* Voice);

	UFUNCTION(BlueprintPure, Category = SFX)
	static UFMODEvent* GetWallHitSFXFromPhysicalMaterial(UPhysicalMaterial* PhysicalMaterial);

	UFUNCTION(BlueprintPure, Category = SFX)
	static UParticleSystem* GetWallHitVFXFromPhysicalMaterial(UPhysicalMaterial* PhysicalMaterial);

	//
	// Hardware
	//
	static bool IsBlacklistedGPUName(const FString& Name);


	// Load streamables

	template <typename ValueType>
	void RequestAsyncLoadValue(const TSoftObjectPtr<ValueType>& Value);
	template <typename ValueType>
	void RequestAsyncLoadArray(const TArray<TSoftObjectPtr<ValueType>>& Array);
	template <typename KeyType, typename ValueType>
	void RequestAsyncLoadMapValues(const TMap<KeyType, TSoftObjectPtr<ValueType>>& Map);
	template <typename KeyType, typename ValueType>
	void RequestAsyncLoadMapKeysAndValues(const TMap<TSoftObjectPtr<KeyType>, TSoftObjectPtr<ValueType>>& Map);

protected:
	//
	// Internal methods to update and persistent objects
	//

	// Update the texts
	void UpdateRuntimeTextData();

	void UpdateEnemyDataForCurrentMap();
	void UpdateEnemyData();

	// Streams the data the singleton always needs to have loaded
	void UpdateRuntimeStreamData();

	// Fill missing InputKeyboardTexturesMap
	void UpdateRuntimeInputKeyboardTextureMap();

	// Fill missing InputGamepadTexturesMap
	void UpdateRuntimeInputGamepadTextureMap();

	void AddToPersistentObjects(UObject* Object)
	{
		check(IsValid(Object));
		PersistentObjects.Add(Object);
	}

protected:
	static constexpr int32 KeyboardDefaultIconSize = 48;
	static constexpr float GamepadIconsMultiplier = 1.25f;

public:
	// A native class for managing streaming assets in and keeping them in memory.
	FStreamableManager StreamableManager;

	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	FSoStateTable StateTable;

	// For Garbage Collection
	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	TArray<UObject*> PersistentObjects;

	//
	//  Levels
	//

	/**
	 * Key: Tag: AA, TL, AG, etc
	 * Value: The level pointer and params
	 */
	UPROPERTY(EditAnywhere, Config, Category = LevelDesign, meta = (AllowedClasses = "World"))
	TMap<FString, FSoLevelParams> LevelDataForTags;

	// What weather effect in which level name
	UPROPERTY(EditAnywhere, Config, Category = LevelDesign, meta = (DisplayName = "Levels Weather VFX"))
	TArray<FSoLevelWeatherVFX> LevelsWeatherVFXArray;

	// The main menu level that is loaded on startup
	UPROPERTY(EditAnywhere, Config, Category = LevelDesign, meta = (AllowedClasses = "World"))
	FSoftObjectPath MainMenuLevel;

	// NOTE: Not used
	// Defined by the user so that we can fill the DependentLevelMap
	// UPROPERTY(EditAnywhere, Config, Category = LevelDesign, meta = (AllowedClasses = "World"))
	// TArray<FSoDependentLevelDesc> DependentLevels;

	// NOTE: Not used
	/**
	 * Key: claimed level name
	 * Value: list of dependent levels, they are all active if claimed level is active
	 * Filled from DependentLevels.
	 * Also O(1) access.
	 */
	// UPROPERTY(VisibleAnywhere, Config, Category = LevelDesign)
	// TMap<FName, FSoDependentLevels> DependentLevelMap;

	UPROPERTY(EditAnywhere, Config, Category = Chapter, meta = (AllowedClasses = "World"))
	FSoftObjectPath FirstChapterLevel;

	UPROPERTY(EditAnywhere, Config, Category = Chapter, meta = (AllowedClasses = "World"))
	FSoftObjectPath LastChapterLevel;

	// Contains data about each chapter
	UPROPERTY(EditAnywhere, Config, Category = Chapter)
	TArray<FSoChapterMapParams> ChaptersArray;


	/**
	 *  Each key is a level name
	 *  The array is a list of levels to unload when the key Level loads
	 */
	UPROPERTY(EditAnywhere, Config, Category = Chapter, meta = (AllowedClasses = "World"))
	TMap<FName, FSoNameArray> LevelUnloadMapForSwitch;



	// Contains all the episodes data
	UPROPERTY(EditAnywhere, Config, Category = Episode, meta = (DisplayName = "Episodes"))
	TArray<FSoEpisodeMapParams> EpisodesArray;

	// Used for the demo
	UPROPERTY(EditAnywhere, Config, Category = Episode, meta = (AllowedClasses = "World"))
	FSoftObjectPath DemoEpisodeLevel;


	//
	// Progress
	//

	// Contains data related to player progress in main game
	UPROPERTY(EditAnywhere, Config, Category = Progress)
	TMap<FName, FSoCharacterProgressInfo> CharacterProgressInfo;

	//
	// Enemy
	//


	// Key: The map name
	// Value: Texts for that enemies map
	UPROPERTY(EditAnywhere, Config, Category = Enemy)
	TMap<FName, FSoEnemyGroupDescList> EnemiesDescriptionPerMap;

	//
	// Color and damage textures
	//

	// Used when NOT bAgainstEnemy
	UPROPERTY(EditAnywhere, Config, Category = Color)
	TMap<ESoDmgType, FLinearColor> DamageTypeColors;

	// Used bAgainstEnemy
	UPROPERTY(EditAnywhere, Config, Category = Color)
	FLinearColor DamageColorPhysical;

	// Used bAgainstEnemy
	UPROPERTY(EditAnywhere, Config, Category = Color)
	FLinearColor DamageColorMagic;

	UPROPERTY(EditAnywhere, Config, Category = Color)
	TArray<FLinearColor> ItemRarityColors;

	UPROPERTY(EditAnywhere, Config, Category = Color, meta = (DisplayName = "Item Background Colors"))
	TMap<ESoItemType, TSoftObjectPtr<UTexture2D>> ItemBackgroundColorsMapPtr;

	// Maps from the damage type to the texture
	UPROPERTY(EditAnywhere, Config, Category = Damage, meta = (DisplayName = "Damage Types Textures"))
	TMap<ESoDmgType, TSoftObjectPtr<UTexture2D>> DamageTypesTexturesMapPtr;


	//
	//  Icons
	//

	// The icon to use when all else fails, can't find it most likely
	UPROPERTY(EditAnywhere, Config, Category = Input, meta = (DisplayName = "Input Invalid Icon"))
	TSoftObjectPtr<UTexture2D> InputInvalidIconPtr;

	/**
	 * Holds all the keyboard keys texture mappings
	 * Key: FKey::Name
	 * Value: Texture for this key if any
	 */
	UPROPERTY(EditAnywhere, Config, Category = Input, meta = (DisplayName = "Input Keyboard Textures"))
	TMap<FName, FSoInputKeyboardKeyTextures> InputKeyboardTexturesMap;

	/**
	 * Holds all the gamepad buttons/keys texture mappings for all variants
	 * Key: FKey::Name
	 * Value: All the available textures for this gamepad key if any
	 */
	UPROPERTY(EditAnywhere, Config, Category = Input, meta = (DisplayName = "Input Gamepad Textures"))
	TMap<FName, FSoInputGamepadKeyTextures> InputGamepadTexturesMap;

	/**
	 *  Holds extra, special highlighted images for some controller button which are used
	 *  if both pressed and unpressed state must be visualized (decals)
	 */
	UPROPERTY(EditAnywhere, Config, Category = Input, meta = (DisplayName = "Highlighted Gamepad Textures"))
	TMap<FName, FSoInputGamepadKeyTextures> HighlightedGamepadTexturesMap;


	//
	//  Texts
	//

	UPROPERTY(EditAnywhere, Config, Category = Loading, AdvancedDisplay)
	TArray<FText> GeneralLoadingScreenTexts;

	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	TArray<FText> ItemTypeTexts;

	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	TArray<FText> ShardTypeTexts;

	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	TArray<FText> WeaponTypeTexts;

	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	TArray<FText> UsableTypeTexts;

	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	TArray<FText> QuestItemTypeTexts;

	UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	TArray<FText> ItemParamTexts;


	//
	// SFX & Audio
	//

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "SFX"))
	TMap<ESoSFX, TSoftObjectPtr<UFMODEvent>> SFXMapPtr;

	//TMap<UFMODEvent*, FText> EnemyVoiceTextMap;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "Voice Texts"))
	TArray<FSoVoiceEntry> VoiceTextsArray;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "Voice Texts (Random)"))
	TArray<FSoVoiceEntryRandomText> VoiceTextsArrayRandom;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "Wall Hit SFX Invalid"))
	TSoftObjectPtr<UFMODEvent> WallHitSFXInvalidPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "Wall Hits SFX"))
	TMap<TSoftObjectPtr<UPhysicalMaterial>, TSoftObjectPtr<UFMODEvent>> WallHitSFXMapPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "Menu Music"))
	TSoftObjectPtr<UFMODEvent> MenuMusicPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "Loading Music"))
	TSoftObjectPtr<UFMODEvent> LoadingMusicPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "VCA Master"))
	TSoftObjectPtr<UFMODVCA> VCA_MasterPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "VCA SFX"))
	TSoftObjectPtr<UFMODVCA> VCA_SFXPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "VCA UI SFX"))
	TSoftObjectPtr<UFMODVCA> VCA_UIPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "VCA Music"))
	TSoftObjectPtr<UFMODVCA> VCA_MusicPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "VCA Mute"))
	TSoftObjectPtr<UFMODVCA> VCA_MutePtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio,  meta = (DisplayName = "Menu Buses To Pause"))
	TArray<TSoftObjectPtr<UFMODBus>> MenuBusesToPauseArrayPtr;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "VCA Menu"))
	FSoVCABlend VCA_Menu;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "VCA Amb Dialogue"))
	FSoVCABlend VCA_AmbientDialogue;

	// VCAMenu and VCA_AmbientDialogue
	TArray<FSoVCABlend*> VCA_BlendArray;

	UPROPERTY(EditAnywhere, Config, Category = Audio)
	float MinWaitBeforeMusicRestart = 30.0f;

	UPROPERTY(EditAnywhere, Config, Category = Audio)
	float MaxWaitBeforeMusicRestart = 45.0f;

	UPROPERTY(EditAnywhere, Config, Category = Audio, meta = (DisplayName = "Buses To Ignore For Speed Change"))
	TArray<TSoftObjectPtr<UFMODBus>> BusesToIgnoreForSpeedChangeArrayPtr;

	//
	// WallHit
	//

	UPROPERTY(EditAnywhere, Config, Category = Particles, meta = (DisplayName = "Wall Hit VFX Invalid"))
	TSoftObjectPtr<UParticleSystem> WallHitVFXInvalidPtr;

	UPROPERTY(EditAnywhere, Config, Category = Particles, meta = (DisplayName = "Wall Hit VFX"))
	TMap<TSoftObjectPtr<UPhysicalMaterial>, TSoftObjectPtr<UParticleSystem>> WallHitVFXMapPtr;

	//
	// Media player
	//

	UPROPERTY(EditAnywhere, Config, Category = VideoPlayer, meta = (DisplayName = "Video Media Player"))
	TSoftObjectPtr<UMediaPlayer> VideoMediaPlayerPtr;

	UPROPERTY(EditAnywhere, Config, Category = VideoPlayer, meta = (DisplayName = "Video Media Texture"))
	TSoftObjectPtr<UMediaTexture> VideoMediaTexturePtr;

	UPROPERTY(EditAnywhere, Config, Category = VideoPlayer, meta = (DisplayName = "Demo Media Source"))
	TSoftObjectPtr<UMediaSource> DemoMediaSourcePtr;

	UPROPERTY(EditAnywhere, Config, Category = VideoPlayer, meta = (DisplayName = "Intro Media Source"))
	TSoftObjectPtr<UMediaSource> IntroMediaSourcePtr;

	UPROPERTY(EditAnywhere, Config, Category = VideoPlayer, meta = (DisplayName = "Intro Subtitle"))
	TArray<FSoVideoSubtitle> IntroSubs;

	UPROPERTY(EditAnywhere, Config, Category = VideoPlayer, meta = (DisplayName = "Intro sounds"))
	TSoftObjectPtr<UFMODEvent> IntroSounds;

	//
	// Crash
	//

	// Non Steam
	UPROPERTY(EditAnywhere, Config, Category = Crash)
	FString FeedbackURL;

	// Steam
	UPROPERTY(EditAnywhere, Config, Category = Crash)
	FString FeedbackURLSteam;

	// Demo non Steam
	UPROPERTY(EditAnywhere, Config, Category = Crash)
	FString FeedbackURLDemo;

	// Demo Steam
	UPROPERTY(EditAnywhere, Config, Category = Crash)
	FString FeedbackURLDemoSteam;

	//
	// Hadware
	//

	// List of GPUs that are not allowed
	UPROPERTY(EditAnywhere, Config, Category = Hadware)
	TArray<FString> BlacklistedGPUNames;

	//
	// OLD DATA
	//

	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// TMap<FName, UParticleSystem*> LevelWeatherVFXMap;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// TMap<ESoItemType, UTexture2D*> ItemBackgroundColorsMap;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// TMap<ESoDamageType, UTexture2D*> DamageTypesTexturesMap;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// UTexture2D* InputInvalidIcon = nullptr;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// TMap<ESoSFX, UFMODEvent*> SFXMap;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// UFMODEvent* WallHitSFXInvalid;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// TMap<UPhysicalMaterial*, UFMODEvent*> WallHitSFXMap;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// UParticleSystem* WallHitVFXInvalid;
	// UPROPERTY(VisibleAnywhere, Category = Runtime, AdvancedDisplay)
	// TMap<UPhysicalMaterial*, UParticleSystem*> WallHitVFXMap;
};
