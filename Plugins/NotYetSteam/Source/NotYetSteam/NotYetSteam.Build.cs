// Copyright (c) Daniel Butum. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class NotYetSteam : ModuleRules
{
	public NotYetSteam(ReadOnlyTargetRules Target) : base(Target)
	{
		Console.WriteLine("Using NotYetSteam.Build.cs");
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;
		//MinFilesUsingPrecompiledHeaderOverride = 1;
		//bFasterWithoutUnity = true;

		// Steam
		const string SteamVersion = "Steamv139";
		bool bSteamSDKFound = Directory.Exists(Target.UEThirdPartySourceDirectory + "Steamworks/" + SteamVersion) == true;
		PublicDefinitions.Add("STEAMSDK_FOUND=" + (bSteamSDKFound ? "1" : "0"));
		PublicDefinitions.Add("STEAM_SDK_ROOT_PATH=TEXT(\"Binaries/ThirdParty/Steamworks\")");

		//PublicDefinitions.Add("STEAM_SDK_VER=TEXT(\"1.39\")");
		//PublicDefinitions.Add("STEAM_SDK_VER_PATH=TEXT(\"Steam" + SteamVersion + "\")");

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				Path.Combine(ModuleDirectory, "Public")
			}
		);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				Path.Combine(ModuleDirectory, "Private")
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",

				// General online modules
				"OnlineSubsystem",
				"OnlineSubsystemUtils"//,
				//"OnlineSubsystemSteam"
				// ... add other public dependencies that you statically link with here ...
			}
		);

		PublicDependencyModuleNames.Add("Steamworks");
		PublicIncludePathModuleNames.Add("Steamworks");
		//AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");

		DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
	}
}
