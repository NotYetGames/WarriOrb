// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;


public class NotYetLoadingScreen : ModuleRules
{


	//
	// DEBUG
	//


	public NotYetLoadingScreen(ReadOnlyTargetRules Target)
		: base(Target)
	{
		Console.WriteLine("Using NotYetLoadingScreen.Build.cs");

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;

		PublicIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Public")
				// ... add public include paths required here ...
			});


		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Private")
				// ... add other private include paths required here ...
			});

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"MoviePlayer",
				"Slate",
				"SlateCore",
				"InputCore",
				"RenderCore",
				"Engine",
				"RHI"
			});

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		// DEBUG FLAGS
	}

	static string BoolToIntString(bool value)
	{
		return value ? "1" : "0";
	}
}
