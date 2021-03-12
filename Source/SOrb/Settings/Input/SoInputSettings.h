// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerInput.h"
#include "Engine/DeveloperSettings.h"
#include "SoInputSettingsTypes.h"

#include "SoInputSettings.generated.h"


// Tells us to what key we want to Rebind the ActionName
USTRUCT(BlueprintType)
struct SORB_API FSoInputPresetKeyRebind
{
	GENERATED_USTRUCT_BODY()
public:
	FInputActionKeyMapping ToUnrealActionMapping() const;

public:
	// Action name to rebind
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FName ActionName;

	// Key to rebind to
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FInputChord Key;
};


// Represents an input preset, how do we want to reassign all keys?
USTRUCT(BlueprintType)
struct SORB_API FSoInputPreset
{
	GENERATED_USTRUCT_BODY()
public:
	FSoInputPreset();

	// Fix any changing configurable option names, we removed/added new option
	static void CheckAndFixIntegrityKeyRebinds(TArray<FSoInputPresetKeyRebind>& KeyRebinds);
	static void ResetToDefaultsKeyRebinds(TArray<FSoInputPresetKeyRebind>& KeyRebinds);
	
	void CheckAndFixIntegrity();
	void ResetToDefaults();

	FORCEINLINE bool IsKeyboard() const { return DeviceType == ESoInputDeviceType::Keyboard; }

public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	ESoInputDeviceType DeviceType = ESoInputDeviceType::Keyboard;

	// All the key rebinds for this preset
	UPROPERTY(BlueprintReadOnly, EditAnywhere, EditFixedSize)
	TArray<FSoInputPresetKeyRebind> KeyRebinds;
};


// Custom input settings we need
UCLASS(Config = SoInputSettings, DefaultConfig, meta = (DisplayName = "Warriorb Input"))
class SORB_API USoInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	// UDeveloperSettings interface
	/** Gets the settings container name for the settings, either Project or Editor */
	FName GetContainerName() const override { return TEXT("Project"); }
	/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
	FName GetCategoryName() const override { return TEXT("Game"); };
	/** The unique name for your section of settings, uses the class's FName. */
	FName GetSectionName() const override { return TEXT("Warriorb Input"); };

#if WITH_EDITOR
	/** Gets the description for the section, uses the classes ToolTip by default. */
	FText GetSectionDescription() const override
	{
		return FText::FromString(TEXT("Configure the custom input settings"));
	}

	/** Whether or not this class supports auto registration or if the settings have a custom setup */
	bool SupportsAutoRegistration() const override { return true; }

	// UObject interface
	bool CanEditChange(const UProperty* InProperty) const override;
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	void PostInitProperties() override;

	// Own methods
	/** Saves the settings to the config file depending on the settings of this class. */
	void SaveSettings()
	{
		const UClass* ThisClass = GetClass();
		if (ThisClass->HasAnyClassFlags(CLASS_DefaultConfig))
		{
			UpdateDefaultConfigFile();
		}
		else if (ThisClass->HasAnyClassFlags(CLASS_GlobalUserConfig))
		{
			UpdateGlobalUserConfigFile();
		}
		else
		{
			SaveConfig();
		}
	}

	
	const FSoInputPreset& GetKeyboardDefaultArrowsPreset() const { return KeyboardDefaultArrowsPreset; }

protected:
	void ReloadRuntimeValues();
	
protected:
	// This is like this because it looks nicer in the config file instead of one long line
	UPROPERTY(Config, EditAnywhere, Category = "Keyboard Default Arrows Preset", DisplayName = "Device Type")
	ESoInputDeviceType KeyboardDefaultArrowsPresetDeviceType = ESoInputDeviceType::Keyboard;
	UPROPERTY(Config, EditAnywhere, EditFixedSize, Category = "Keyboard Default Arrows Preset", DisplayName = "Key Rebinds")
	TArray<FSoInputPresetKeyRebind> KeyboardDefaultArrowsPresetKeyRebinds;

	// Set at runtime
	FSoInputPreset KeyboardDefaultArrowsPreset;
};
