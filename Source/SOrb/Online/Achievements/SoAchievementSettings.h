// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "SoAchievement.h"

#include "SoAchievementSettings.generated.h"

// Custom input settings we need
UCLASS(Config = SoAchievementSettings, DefaultConfig, meta = (DisplayName = "Warriorb Achievements"))
class SORB_API USoAchievementSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	// UDeveloperSettings interface
	/** Gets the settings container name for the settings, either Project or Editor */
	FName GetContainerName() const override { return "Project"; }
	/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
	FName GetCategoryName() const override { return "Game"; };
	/** The unique name for your section of settings, uses the class's FName. */
	FName GetSectionName() const override { return "Warriorb Achievements"; };

#if WITH_EDITOR
	/** Gets the description for the section, uses the classes ToolTip by default. */
	FText GetSectionDescription() const override;

	/** Whether or not this class supports auto registration or if the settings have a custom setup */
	bool SupportsAutoRegistration() const override { return true; }

	// UObject interface
	bool CanEditChange(const UProperty* InProperty) const override;
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	void PostLoad() override;
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

	void CheckAndFixIntegrity();

	const TArray<FName>& GetDemoAchievementNames() const { return DemoAchievementNames; }
	const TArray<FName>& GetAchievementNames() const { return AchievementNames; }
	
protected:
	// All the achievement names for the Demo
	// Useful mostly for the console commands
	UPROPERTY(Config, EditAnywhere, Category = "Demo", DisplayName = "Achievement Names")
	TArray<FName> DemoAchievementNames;

	// All the achievement name for the main game
	// Useful mostly for the console commands
	UPROPERTY(Config, EditAnywhere, Category = "Game")
	TArray<FName> AchievementNames;
};
