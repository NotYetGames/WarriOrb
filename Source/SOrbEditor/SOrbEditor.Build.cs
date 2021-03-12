// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class SOrbEditor : ModuleRules
{
	public SOrbEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		Console.WriteLine("Using SOrbEditor.Build.cs");

		// Enable IWYU
		// https://docs.unrealengine.com/latest/INT/Programming/UnrealBuildSystem/IWYUReferenceGuide/index.html
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

		// MinFilesUsingPrecompiledHeaderOverride = 1;
		bFasterWithoutUnity = false;
		//PrivatePCHHeaderFile = "Public/SOrbEditor.h";

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
			"AnimGraph",
			"BlueprintGraph",
			"UnrealEd",

			"SOrb"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"InputCore",
			"SlateCore",
			"Slate",
			"EditorStyle",
			"LevelEditor",
			"EditorScriptingUtilities",
			"ContentBrowser",
			"AssetRegistry",

			"DlgSystem",
			"NotYetEditorMode"
		});

		PrivateIncludePathModuleNames.AddRange(new string[] { "AssetTools" });

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory)
			});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");
		// if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
		// {
		//		if (UEBuildConfiguration.bCompileSteamOSS == true)
		//		{
		//			DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
		//		}
		// }
	}
}
