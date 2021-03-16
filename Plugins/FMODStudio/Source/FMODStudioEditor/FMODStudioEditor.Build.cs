// Copyright (c), Firelight Technologies Pty, Ltd. 2012-2020.
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class FMODStudioEditor : ModuleRules
    {
    #if WITH_FORWARDED_MODULE_RULES_CTOR
        public FMODStudioEditor(ReadOnlyTargetRules Target) : base(Target)
    #else
        public FMODStudioEditor(TargetInfo Target)
    #endif
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
            PrivatePCHHeaderFile = "Private/FMODStudioEditorPrivatePCH.h";

            bFasterWithoutUnity = false;

            PrivateIncludePaths.AddRange(
                new string[] {
                    "FMODStudio/Private",
                    "FMODStudio/Public/FMOD",
                    Path.Combine(ModuleDirectory, "Classes")
                }
                );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "FMODStudio",
                    "InputCore",
                    "UnrealEd",
                    "Sequencer"
                }
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Slate",
                    "SlateCore",
                    "Settings",
                    "EditorStyle",
                    "LevelEditor",
                    "AssetTools",
                    "AssetRegistry",
                    "PropertyEditor",
                    "WorkspaceMenuStructure",
                    "Sockets",
                    "LevelSequence",
                    "MainFrame",
                    "MovieScene",
                    "MovieSceneTracks",
                    "MovieSceneTools"
                }
                );
        }
    }
}
