// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Tools.DotNETCommon;

using UnrealBuildTool;


public class SoBeforeGame : ModuleRules
{
	public SoBeforeGame(ReadOnlyTargetRules Target) : base(Target)
	{
		Console.WriteLine("Using SoBeforeGame.Build.cs");

		// Enable IWYU
		// https://docs.unrealengine.com/latest/INT/Programming/UnrealBuildSystem/IWYUReferenceGuide/index.html
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

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Json",
		});
	}
}
