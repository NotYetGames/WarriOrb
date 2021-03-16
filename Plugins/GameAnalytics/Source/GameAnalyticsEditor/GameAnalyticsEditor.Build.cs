// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class GameAnalyticsEditor : ModuleRules
    {
#if WITH_FORWARDED_MODULE_RULES_CTOR
        public GameAnalyticsEditor(ReadOnlyTargetRules Target) : base(Target)
#else
        public GameAnalyticsEditor(TargetInfo Target)
#endif
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "RenderCore",
                    "RHI",
                    "Slate",
                    "SlateCore",
                    "EditorStyle",
                    "EditorWidgets",
                    "DesktopWidgets",
                    "PropertyEditor",
                    "SharedSettingsWidgets",
                    "SourceControl",
                    "UnrealEd",
                    "Http",
                    "Json",
                    "JsonUtilities",
					"InputCore"
                }
            );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
                    "Settings"
                }
            );

            PublicIncludePathModuleNames.AddRange(
                new string[]
                {
                    "Settings"
                }
            );
        }
    }
}