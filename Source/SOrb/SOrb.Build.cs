// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Tools.DotNETCommon;

using UnrealBuildTool;


public class SOrb : ModuleRules
{
	// PublicDefinition flags
	//
	// NOTE: These flags have the default values for the desktop (usually)
	//

	// Enables/disable GameAnalytics plugin
	// NOTE: WARRIORB_WITH_ANALYTICS needs be also true
	public bool WARRIORB_WITH_GAMEANALYTICS = false;

	// Enables/disable the analytics providers, all of THEM
	public bool WARRIORB_WITH_ANALYTICS = false;

	// Enable/disable the default collection of analytics
	public readonly bool WARRIORB_COLLECT_ANALYTICS = true;

	// off by default
	public bool WARRIORB_WITH_ONLINE = false;

	// Enables/disable SDL2 plugin
	public readonly bool WARRIORB_WITH_SDL2 = false;

	// Enable/disable the char fake shadow
	public readonly bool WARRIORB_USE_CHARACTER_SHADOW = true;

	// Enable/disable the usage of the unreal save system
	// If this is enabled we use the unreal save (ISaveGameSystem) for saving and loading. These files are binary
	// NOTE: usually on consoles as our own implementation of the save does not support those platforms
	public readonly bool WARRIORB_USE_UNREAL_SAVE_SYSTEM = false;

	// Special variable we can override to fake non editor behaviour in editor
	// Usually just equal with WITH_EDITOR
	public readonly bool WARRIORB_WITH_EDITOR = false;

	//
	// INPUT
	//

	// Should we use the keyboard arrow preset as the default?
	public readonly bool WARRIORB_USE_ARROW_PRESET_AS_DEFAULT = true;

	//
	// DEBUG
	//

	// Simulate non editor behaviour
	// Set it to true to have non-editor behavior in editor regarding game start, saves, etc.
	// Mostly used in places where !WITH_EDITOR would be used
	// NOTE: does not affect analytics, use WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST for that
	public const bool WARRIORB_NON_EDITOR_TEST = false;

	// Simulate the collecting of events analytics like in the non editor
	public const bool WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST = false;

	// Simulate as we are on a console
	public const bool WARRIORB_CONSOLE_TEST = false;

	// Simulate like the platform does not support keyboard
	public const bool WARRIORB_NO_KEYBOARD_TEST = false;

	public SOrb(ReadOnlyTargetRules Target) : base(Target)
	{
		Console.WriteLine("Using SOrb.Build.cs");
		Init();

		if (!IsSupportedConsolePlatform && !IsSupportedDesktopPlatform)
		{
			Fail("Target platform is not supported");
		}

		// Aka WITH_EDITOR && !WARRIORB_NON_EDITOR_TEST
		WARRIORB_WITH_EDITOR = Target.bBuildEditor && !WARRIORB_NON_EDITOR_TEST;

		// Do not collect analytics in editor, only if the debug flag is set to true
		// Aka !WITH_EDITOR || WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST
		WARRIORB_COLLECT_ANALYTICS = !Target.bBuildEditor || WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST;

		// Console flags take precedence
		if (IsPlatformSwitch)
		{
			if (WARRIORB_USE_CHARACTER_SHADOW)
			{
				PrintYellow("Disabling the Character fake shadow (WARRIORB_USE_CHARACTER_SHADOW)");
				WARRIORB_USE_CHARACTER_SHADOW = false;
			}
		}
		if (IsSupportedConsolePlatform)
		{
			PrintYellow("Disabling Analytics & GameAnalytics support because we are building for console");
			WARRIORB_WITH_ANALYTICS = false;
			WARRIORB_COLLECT_ANALYTICS = false;

			if (WARRIORB_WITH_SDL2)
			{
				PrintYellow("Disabling SDL2 support because we are building for console");
				WARRIORB_WITH_SDL2 = false;
			}
			if (WARRIORB_WITH_DISCORD)
			{
				PrintYellow("Disabling Discord support because we are building for console");
				WARRIORB_WITH_DISCORD = false;
			}
			if (WARRIORB_WITH_STEAM)
			{
				PrintYellow("Disabling Steam support because we are building for console");
				WARRIORB_WITH_STEAM = false;
			}

			PrintYellow("Enabling (WARRIORB_USE_UNREAL_SAVE_SYSTEM) the the Unreal Save System (ISaveGameSystem)");
			WARRIORB_USE_UNREAL_SAVE_SYSTEM = true;
		}
		if (Target.bBuildEditor)
		{
			if (WARRIORB_NON_EDITOR_TEST)
			{
				PrintYellow("[DEBUG] WARRIORB_NON_EDITOR_TEST is set to TRUE, WARRIORB_WITH_EDITOR will be set to FALSE");
			}
			if (WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST)
			{
				PrintYellow("[DEBUG] WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST is set to TRUE, WARRIORB_COLLECT_ANALYTICS will be set to TRUE");
			}

			if (WARRIORB_WITH_DISCORD)
			{
				PrintYellow("Disabling WARRIORB_WITH_DISCORD because we are building for Editor");
				WARRIORB_WITH_DISCORD = false;
			}
			if (WARRIORB_WITH_STEAM)
			{
				PrintYellow("Disabling WARRIORB_WITH_STEAM because we are building for Editor");
				WARRIORB_WITH_STEAM = false;
			}
		}

		if (IsPlatformXboxOne)
		{
			PrintYellow("Enabling online because XBOX");
			WARRIORB_WITH_ONLINE = true;
		}

		if (WARRIORB_DEMO)
		{
			PrintBlue("[DEMO] BUILD [DEMO]");
			PrintYellow("[DEMO] BUILD [DEMO]");
			PrintRed("[DEMO] BUILD [DEMO]");
			Console.WriteLine("");

			if (WARRIORB_WITH_VIDEO_INTRO)
			{
				PrintYellow("Disabling WARRIORB_WITH_VIDEO_INTRO because we are building for Demo");
				WARRIORB_WITH_VIDEO_INTRO = false;
			}
		}
		if (!WARRIORB_WITH_ANALYTICS)
		{
			WARRIORB_WITH_GAMEANALYTICS = false;
			PrintYellow("Disabling GameAnalytics (WARRIORB_WITH_ANALYTICS) because WARRIORB_WITH_ANALYTICS is false");
		}
		if (WARRIORB_WITH_STEAM)
		{
			PrintYellow("Enabling online because we have steam enabled");
			WARRIORB_WITH_ONLINE = true;

			// Disable some flags that should only be used in a NON demo build
			if (WARRIORB_DEMO || Target.bBuildEditor)
			{
				if (WARRIORB_RELAUNCH_IN_STEAM)
				{
					PrintYellow("Disabling WARRIORB_RELAUNCH_IN_STEAM because we are building for Demo/Editor");
					WARRIORB_RELAUNCH_IN_STEAM = false;
				}

				if (WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES)
				{
					PrintYellow("Disabling WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES because we are building for Demo/Editor");
					WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES = false;
				}
			}

			// Disable some flags that should only be used in a Windows64 build
			if (!IsPlatformWindows64)
			{
				if (WARRIORB_RELAUNCH_IN_STEAM)
				{
					PrintYellow("Disabling WARRIORB_RELAUNCH_IN_STEAM because we are NOT building for Windows64");
					WARRIORB_RELAUNCH_IN_STEAM = false;
				}

				if (WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES)
				{
					PrintYellow("Disabling WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES because we are NOT building for Windows64");
					WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES = false;
				}
			}
		}
		else
		{
			if (WARRIORB_RELAUNCH_IN_STEAM)
			{
				PrintYellow("Disabling WARRIORB_RELAUNCH_IN_STEAM because we are NOT building for Steam");
				WARRIORB_RELAUNCH_IN_STEAM = false;
			}
			if (WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES)
			{
				PrintYellow("Disabling WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES because we are NOT building for Steam");
				WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES = false;
			}
		}

		//
		// Flags to use in C++
		//

		// DEBUG FLAGS
		PublicDefinitions.Add("WARRIORB_NON_EDITOR_TEST=" + BoolToIntString(WARRIORB_NON_EDITOR_TEST));
		PublicDefinitions.Add("WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST=" + BoolToIntString(WARRIORB_NON_EDITOR_COLLECT_ANALYTICS_TEST));
		PublicDefinitions.Add("WARRIORB_WITH_EDITOR=" + BoolToIntString(WARRIORB_WITH_EDITOR));
		PublicDefinitions.Add("WARRIORB_CONSOLE_TEST=" + BoolToIntString(WARRIORB_CONSOLE_TEST));
		PublicDefinitions.Add("WARRIORB_NO_KEYBOARD_TEST=" + BoolToIntString(WARRIORB_NO_KEYBOARD_TEST));

		// Plugins
		PublicDefinitions.Add("WARRIORB_WITH_ONLINE=" + BoolToIntString(WARRIORB_WITH_ONLINE));

		// Enable video demo support
		PublicDefinitions.Add("WARRIORB_WITH_VIDEO_DEMO=" + BoolToIntString(WARRIORB_WITH_VIDEO_DEMO));

		PublicDefinitions.Add("WARRIORB_WITH_VIDEO_INTRO=" + BoolToIntString(WARRIORB_WITH_VIDEO_INTRO));

		// Enable demo support aka WarriOrb: Prologue
		PublicDefinitions.Add("WARRIORB_DEMO=" + BoolToIntString(WARRIORB_DEMO));

		// Game flags
		PublicDefinitions.Add("WARRIORB_USE_CHARACTER_SHADOW=" + BoolToIntString(WARRIORB_USE_CHARACTER_SHADOW));
		PublicDefinitions.Add("WARRIORB_USE_UNREAL_SAVE_SYSTEM=" + BoolToIntString(WARRIORB_USE_UNREAL_SAVE_SYSTEM));
		PublicDefinitions.Add("WARRIORB_USE_ARROW_PRESET_AS_DEFAULT=" + BoolToIntString(WARRIORB_USE_ARROW_PRESET_AS_DEFAULT));
		PublicDefinitions.Add("WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT=" + BoolToIntString(WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT));
		PublicDefinitions.Add(String.Format("WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS=\"{0}\"", WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS));
		PublicDefinitions.Add(String.Format("WARRIORB_PASSWORD_SAVE_FILE=\"{0}\"", WARRIORB_PASSWORD_SAVE_FILE));

		if (WARRIORB_WITH_ONLINE)
		{
			PublicDependencyModuleNames.Add("OnlineSubsystem");
			PublicDependencyModuleNames.Add("OnlineSubsystemUtils");
			if (IsPlatformXboxOne)
				PublicDependencyModuleNames.Add("OnlineSubsystemLive");
			else
				PublicDependencyModuleNames.Add("OnlineSubsystemNull");
			//PublicDependencyModuleNames.Add("OnlineFramework");
		}

		SetupBuildVersion();
		SetupSteam();
		SetupLoadingScreen();
		SetupSDL();
		SetupAsusAuraSDK();
		SetupDiscord();
		SetupAnalytics();

		// We need this dependency to get some key information from viewport when moving object along spline
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		PublicIncludePaths.AddRange(new string[] {
			Path.Combine(ModuleDirectory)
		});

		// Print Normal flags
		Console.WriteLine("PublicDefinitions Flags:");
		foreach (string Defintion in PublicDefinitions)
		{
			PrintBlue(Defintion);
		}
		Console.WriteLine("");

		//Console.WriteLine("ProjectDefinitions Flags:");
		//foreach (string Defintion in Target.ProjectDefinitions)
		//{
		//	PrintBlue(Defintion);
		//}
		//Console.WriteLine("");

		// Now get the base of UE4's modules dir (could also be Developer, Editor, ThirdParty)
		//string source_runtime_path = Path.Combine(EngineRoot, "Source", "Runtime");
		//if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
		//{
		//	PublicIncludePaths.Add(Path.Combine(source_runtime_path, "ApplicationCore", "Private"));
		//}

		// Debug info disable
		// PrintBlue(string.Format("ENGINE_VERSION_MAJOR={0}", Target.Version.MajorVersion));
		// PrintBlue(string.Format("ENGINE_VERSION_MINOR={0}", Target.Version.MinorVersion));
		// PrintBlue(string.Format("ENGINE_VERSION_HOTFIX={0}", Target.Version.PatchVersion));
		// PrintBlue(string.Format("ENGINE_IS_LICENSEE_VERSION={0}", Target.Version.IsLicenseeVersion? "true" : "false"));
		// PrintBlue(string.Format("ENGINE_IS_PROMOTED_BUILD={0}", Target.Version.IsPromotedBuild? "true" : "false"));
		// PrintBlue(string.Format("CURRENT_CHANGELIST={0}", Target.Version.Changelist));
		// PrintBlue(string.Format("COMPATIBLE_CHANGELIST={0}", Target.Version.EffectiveCompatibleChangelist));
		// PrintBlue(string.Format("BRANCH_NAME=\"{0}\"", Target.Version.BranchName));
		// PrintBlue(string.Format("BUILD_VERSION=\"{0}\"", Target.BuildVersion));
	}

	public void Init()
	{
		InitFromPlatform(Target.Platform);
		ParseBuildFile();

		//
		// Common
		//

		// Enable IWYU
		// https://docs.unrealengine.com/latest/INT/Programming/UnrealBuildSystem/IWYUReferenceGuide/index.html
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;
		//PrivatePCHHeaderFile = "SOrb.h";

		// Faster compile time optimization
		// MinFilesUsingPrecompiledHeaderOverride = 1;
		// If this is enabled, it makes each file compile by itself, default UE is to bundle files together into Modules
		bFasterWithoutUnity = false;

		// Other, note older CPUS than 2012 don't work
		bUseAVX = false;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"ApplicationCore",
			"CoreUObject",
			"Engine",
			"InputCore",
			"AIModule",
			"Json",
			"AnimationCore",
			"AnimGraphRuntime",
			"UMG",
			"Slate",
			"SlateCore",
			"ApexDestruction",

			"MovieScene",
			"LevelSequence",
			"MediaAssets",
			"Media",
			"AudioMixer",

			"RHI", // getting system stuff

			// Init stuff
			"SoBeforeGame"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			// Plugins
			 "DlgSystem",
			 "FMODStudio",
			 "NotYetLoadingScreen"
		});
	}

	public string CurrentDirectoryPath
	{
		get
		{
			return Path.GetFullPath(ModuleDirectory);
		}
	}
	public string WarriorbVersionFilePath
	{
		get
		{
			return Path.Combine(CurrentDirectoryPath, "Resources", "WarriorbVersion.h");
		}
	}

	public string EngineRootPath
	{
		// Get the engine path. Ends with "Engine/"
		get
		{
			return Path.GetFullPath(Target.RelativeEnginePath);
		}
	}

	public void SetupSteam()
	{
		// C++ Flags
		PublicDefinitions.Add("WARRIORB_WITH_STEAM=" + BoolToIntString(WARRIORB_WITH_STEAM));
		PublicDefinitions.Add("WARRIORB_STEAM_APP_ID=" + IntToString(WARRIORB_STEAM_APP_ID));
		PublicDefinitions.Add("WARRIORB_STEAM_DEMO_APP_ID=" + IntToString(WARRIORB_STEAM_DEMO_APP_ID));
		PublicDefinitions.Add("WARRIORB_RELAUNCH_IN_STEAM=" + BoolToIntString(WARRIORB_RELAUNCH_IN_STEAM));
		PublicDefinitions.Add("WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES=" + BoolToIntString(WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES));
		PublicDefinitions.Add(String.Format("WARRIORB_STEAM_LIBRARY_WINDOWS_API64_SHA1_SUM=\"{0}\"", WARRIORB_STEAM_LIBRARY_WINDOWS_API64_SHA1_SUM));

		ConfigHierarchy ConfigEngineHierarchy = ConfigCache.ReadHierarchy(ConfigHierarchyType.Engine, new DirectoryReference(ProjectRootPath), Target.Platform);
		Dictionary<string, string> SteamReplacements = new Dictionary<string, string>();

		// Steam
		if (WARRIORB_WITH_STEAM)
		{
			// Replace the steam id with our own defined one
			// Same for the Game Version

			// SteamDevAppId
			{
				int CurrentAppid;
				ConfigEngineHierarchy.GetInt32("OnlineSubsystemSteam", "SteamDevAppId", out CurrentAppid);
				string FormatString = "SteamDevAppId={0}";
				SteamReplacements.Add(String.Format(FormatString, CurrentAppid), String.Format(FormatString, SteamAppid));
			}

			// GameVersion
			{
				string CurrentGameVersion;
				ConfigEngineHierarchy.GetString("OnlineSubsystemSteam", "GameVersion", out CurrentGameVersion);
				string FormatString = "GameVersion={0}";
				SteamReplacements.Add(String.Format(FormatString, CurrentGameVersion), String.Format(FormatString, WARRIORB_BUILD_VERSION));
			}

			// Relaunch in Steam?
			// https://partner.steamgames.com/doc/api/steam_api#SteamAPI_RestartAppIfNecessary
			// NOTE: disabled at it is broken anyways
			// {
			// 	bool CurrentRelaunchInSteam;
			// 	ConfigEngineHierarchy.GetBool("OnlineSubsystemSteam", "bRelaunchInSteam", out CurrentRelaunchInSteam);
			// 	string FormatString = "bRelaunchInSteam={0}";
			// 	SteamReplacements.Add(String.Format(FormatString, CurrentRelaunchInSteam), String.Format(FormatString, WARRIORB_RELAUNCH_IN_STEAM));
			// }

			// Include plugins
			PrivateDependencyModuleNames.Add("NotYetSteam");
			DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
		}

		// bEnabled
		// {
		// 	bool CurrentEnabled;
		// 	ConfigEngineHierarchy.GetBool("OnlineSubsystemSteam", "bEnabled", out CurrentEnabled);
		// 	string FormatString = "bEnabled={0}";
		// 	SteamReplacements.Add(String.Format(FormatString, CurrentEnabled), String.Format(FormatString, WARRIORB_WITH_STEAM));
		// }

		PatchFile(DefaultEngineConfigPath, SteamReplacements);
	}

	public void SetupLoadingScreen()
	{
		// Loading screen replacements
		Dictionary<string, string> LoadingReplacements = new Dictionary<string, string>();
		string Format = "(Image={0},";
		string LoadingImageFinal = String.Format(Format, LoadingImage);
		string LoadingImageDemoFinal = String.Format(Format, LoadingImageDemo);
		if (WARRIORB_DEMO)
		{
			// Replace main game image with demo
			LoadingReplacements.Add(LoadingImageFinal, LoadingImageDemoFinal);
		}
		else
		{
			// Replace demo image with main game
			LoadingReplacements.Add(LoadingImageDemoFinal, LoadingImageFinal);
		}
		PatchFile(DefaultNYLoadingScreenSettingsConfigPath, LoadingReplacements);
	}

	public void SetupSDL()
	{
		// C++ Flags
		PublicDefinitions.Add("WARRIORB_WITH_SDL2=" + BoolToIntString(WARRIORB_WITH_SDL2));
		if (WARRIORB_WITH_SDL2)
		{
			PrivateDependencyModuleNames.Add("NotYetSDL2");
		}

		// For USoPlatformeHelper
		// if (IsPlatformLinux)
		// {
		// 	AddEngineThirdPartyPrivateStaticDependencies(Target, "SDL2");
		// }
	}

	public void SetupAsusAuraSDK()
	{
		PublicDefinitions.Add("WARRIORB_USE_ASUS_AURA_SDK=" + BoolToIntString(WARRIORB_USE_ASUS_AURA_SDK));
	}

	public void SetupDiscord()
	{
		PublicDefinitions.Add("WARRIORB_WITH_DISCORD=" + BoolToIntString(WARRIORB_WITH_DISCORD));
		PublicDefinitions.Add("WARRIORB_DISCORD_CLIENT_ID=" + Int64ToString(WARRIORB_DISCORD_CLIENT_ID));
		PublicDefinitions.Add("WARRIORB_DISCORD_DEMO_CLIENT_ID=" + Int64ToString(WARRIORB_DISCORD_DEMO_CLIENT_ID));

		if (WARRIORB_WITH_DISCORD)
		{
			PrivateDependencyModuleNames.Add("DiscordGameSDK");
			// PrivateDependencyModuleNames.Add("DiscordGameSDKLibrary");
			PrivateIncludePathModuleNames.Add("DiscordGameSDK");
		}
	}

	public void SetupAnalytics()
	{
		// C++ Flags
		// Enable/disable analytics entirely or only the GameAnalytics plugin
		PublicDefinitions.Add("WARRIORB_WITH_GAMEANALYTICS=" + BoolToIntString(WARRIORB_WITH_GAMEANALYTICS));
		PublicDefinitions.Add("WARRIORB_WITH_ANALYTICS=" + BoolToIntString(WARRIORB_WITH_ANALYTICS));
		PublicDefinitions.Add("WARRIORB_COLLECT_ANALYTICS=" + BoolToIntString(WARRIORB_COLLECT_ANALYTICS));

		if (WARRIORB_WITH_ANALYTICS)
		{
			PrivateDependencyModuleNames.Add("Analytics");
			PrivateIncludePathModuleNames.Add("Analytics");

			if (WARRIORB_WITH_GAMEANALYTICS)
			{
				PrivateDependencyModuleNames.Add("GameAnalytics");
				PrivateIncludePathModuleNames.Add("GameAnalytics");
			}
		}
	}

	public void SetupBuildVersion()
	{
		string BUILD_VERSION = WARRIORB_BUILD_VERSION;
		string BRANCH = "None";
		string COMMIT = "None";
		string DEMO = WARRIORB_DEMO ? "-DEMO" : "";

		// Read build version
		GetCurrentBranchAndCommit(ref BRANCH, ref COMMIT);

		Console.WriteLine("");
		PrintBlue(String.Format("BUILD_VERSION = {0}, BRANCH = {1}, COMMIT = {2}, DEMO = {3}", BUILD_VERSION, BRANCH, COMMIT, WARRIORB_DEMO));
		Console.WriteLine("");

		// The current build number
		// NOTE: this also sets the GameAnalytics version for all platforms, so you do not need to update those values.
		PublicDefinitions.Add(String.Format("WARRIORB_BUILD_VERSION=\"{0}\"", BUILD_VERSION));
		PublicDefinitions.Add(String.Format("WARRIORB_BUILD_BRANCH=\"{0}\"", BRANCH));
		PublicDefinitions.Add(String.Format("WARRIORB_BUILD_COMMIT=\"{0}\"", COMMIT));

		// Write to file so that other resources can use it
		string VersionFileText = @"
// DO NOT MODIFY - FILE AUTOMATICALLY GENERATED in SOrb.Build.cs - DO NOT MODIFY
#define WARRIORB_BUILD_VERSION {0}
#define WARRIORB_BUILD_BRANCH {1}
#define WARRIORB_BUILD_COMMIT {2}
#define WARRIORB_BUILD_ALL {0}-{1}-{2}{3}
// DO NOT MODIFY - FILE AUTOMATICALLY GENERATED in SOrb.Build.cs - DO NOT MODIFY
";
		string VersionFileTextFormat = String.Format(VersionFileText, BUILD_VERSION, BRANCH, COMMIT, DEMO);
		File.WriteAllText(WarriorbVersionFilePath, VersionFileTextFormat);

		// Do replacements in GeneralProjectSettings
		ConfigHierarchy ConfigGameHierarchy = ConfigCache.ReadHierarchy(ConfigHierarchyType.Game, new DirectoryReference(ProjectRootPath), Target.Platform);
		Dictionary<string, string> GeneralProjectSettingsReplacements = new Dictionary<string, string>();

		// ProjectVersion
		{
			string CurrentProjectVersion;
			ConfigGameHierarchy.GetString("/Script/EngineSettings.GeneralProjectSettings", "ProjectVersion", out CurrentProjectVersion);
			string FormatString = "ProjectVersion={0}";
			GeneralProjectSettingsReplacements.Add(String.Format(FormatString, CurrentProjectVersion), String.Format(FormatString, WARRIORB_BUILD_VERSION));
		}

		// ProjectName
		{
			string CurrentProjectName;
			ConfigGameHierarchy.GetString("/Script/EngineSettings.GeneralProjectSettings", "ProjectName", out CurrentProjectName);
			string FormatString = "ProjectName={0}";
			string ReplacementString = WARRIORB_DEMO ? ProjectNameDemo : ProjectName;
			GeneralProjectSettingsReplacements.Add(String.Format(FormatString, CurrentProjectName), String.Format(FormatString, ReplacementString));
		}

		// ProjectDisplayedTitle
		{
			string CurrentProjectDisplayedTitle;
			ConfigGameHierarchy.GetString("/Script/EngineSettings.GeneralProjectSettings", "ProjectDisplayedTitle", out CurrentProjectDisplayedTitle);
			string FormatString = "ProjectDisplayedTitle={0}";
			string ReplacementString = WARRIORB_DEMO ? ProjectDisplayedTitleDemo : ProjectDisplayedTitle;
			GeneralProjectSettingsReplacements.Add(String.Format(FormatString, CurrentProjectDisplayedTitle), String.Format(FormatString, ReplacementString));
		}

		PatchFile(DefaultGameConfigPath, GeneralProjectSettingsReplacements);
	}

	//
	// BEGIN COMMON
	//

	//
	// Platform
	//

	// Constant variables because C# is stupid
	public bool IsSupportedDesktopPlatform = false;
	public bool IsSupportedConsolePlatform = false;

	public bool IsPlatformWindows64 = false;
	public bool IsPlatformLinux = false;
	public bool IsPlatformSwitch = false;
	public bool IsPlatformXboxOne = false;

	public void InitFromPlatform(UnrealTargetPlatform TargetPlatform)
	{
		// Checks if we can have some flags
		IsPlatformWindows64 = TargetPlatform == UnrealTargetPlatform.Win64;
		IsPlatformLinux = TargetPlatform == UnrealTargetPlatform.Linux;
		IsSupportedDesktopPlatform = IsPlatformWindows64 || IsPlatformLinux;

		IsPlatformSwitch = TargetPlatform == UnrealTargetPlatform.Switch;
		IsPlatformXboxOne = TargetPlatform == UnrealTargetPlatform.XboxOne;
		IsSupportedConsolePlatform = IsPlatformXboxOne || IsPlatformSwitch;
	}

	//
	// From Build file
	//

	public const string WARRIORB_BUILD_FILE_NAME = "WarriorbBuild.json";
	public string ProjectRootPath { get { return Target.ProjectFile.Directory.ToString(); } }
	public string ProjectSourceDirPath { get { return ProjectRootPath + "/Source"; } }

	public string ProjectConfigDirPath { get { return ProjectRootPath + "/Config"; } }
	public string DefaultEngineConfigPath { get { return ProjectConfigDirPath + "/DefaultEngine.ini"; } }
	public string DefaultGameConfigPath { get { return ProjectConfigDirPath + "/DefaultGame.ini"; } }
	public string DefaultNYLoadingScreenSettingsConfigPath { get { return ProjectConfigDirPath + "/DefaultNYLoadingScreenSettings.ini"; } }

	public string BuildConfigFilePath { get { return ProjectSourceDirPath + "/" + WARRIORB_BUILD_FILE_NAME; } }


	//
	// Steam
	//

	public bool WARRIORB_WITH_STEAM = false;
	public int WARRIORB_STEAM_APP_ID = 0;
	public int WARRIORB_STEAM_DEMO_APP_ID = 0;

	// https://partner.steamgames.com/doc/api/steam_api#SteamAPI_RestartAppIfNecessary
	public bool WARRIORB_RELAUNCH_IN_STEAM = false;

	public bool WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES = false;

	public string WARRIORB_STEAM_LIBRARY_WINDOWS_API64_SHA1_SUM = "";

	//
	// Discord
	//

	public bool WARRIORB_WITH_DISCORD = false;
	public long WARRIORB_DISCORD_CLIENT_ID = 0;
	public long WARRIORB_DISCORD_DEMO_CLIENT_ID = 0;



	//
	// Video
	//

	// Enable/disable the video demo (for example at dreamhack/devplay)
	// Next time maybe look at: GIsDemoMode
	public bool WARRIORB_WITH_VIDEO_DEMO = false;
	public bool WARRIORB_WITH_VIDEO_INTRO = false;

	//
	// Other
	//

	public bool WARRIORB_DEMO = false;

	public bool WARRIORB_USE_ASUS_AURA_SDK = false;

	public string WARRIORB_BUILD_VERSION;

	public bool WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT = false;

	public string WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS;

	public string WARRIORB_PASSWORD_SAVE_FILE;

	public string LoadingImage;
	public string LoadingImageDemo;

	public string ProjectName;
	public string ProjectNameDemo;
	public string ProjectDisplayedTitle;
	public string ProjectDisplayedTitleDemo;


	public int SteamAppid { get { return WARRIORB_DEMO ? WARRIORB_STEAM_DEMO_APP_ID : WARRIORB_STEAM_APP_ID; } }

	public void ParseBuildFile()
	{
		PrintBlue(String.Format("Reading from BuildConfigFile = {0}", BuildConfigFilePath));
		var AllFields = JsonObject.Read(new FileReference(BuildConfigFilePath));
		var PublicDefinitionsFields = AllFields.GetObjectField("PublicDefinitions");
		var LoadingFields = AllFields.GetObjectField("Loading");
		var SteamFields = AllFields.GetObjectField("Steam");
		var VideoFields = AllFields.GetObjectField("Video");

		ProjectName = AllFields.GetStringField("Name");
		ProjectNameDemo = AllFields.GetStringField("NameDemo");
		ProjectDisplayedTitle = AllFields.GetStringField("DisplayedTitle");
		ProjectDisplayedTitleDemo = AllFields.GetStringField("DisplayedTitleDemo");

		WARRIORB_BUILD_VERSION = AllFields.GetStringField("Version");
		WARRIORB_DEMO = PublicDefinitionsFields.GetBoolField("WARRIORB_DEMO");
		WARRIORB_USE_ASUS_AURA_SDK = PublicDefinitionsFields.GetBoolField("WARRIORB_USE_ASUS_AURA_SDK");

		WARRIORB_WITH_STEAM = SteamFields.GetBoolField("IS_ENABLED");
		WARRIORB_STEAM_APP_ID = SteamFields.GetIntegerField("APP_ID");
		WARRIORB_STEAM_DEMO_APP_ID = SteamFields.GetIntegerField("APP_ID_DEMO");
		WARRIORB_RELAUNCH_IN_STEAM = SteamFields.GetBoolField("RELAUNCH_IN_STEAM");
		WARRIORB_STEAM_CHECK_IF_LIBRARY_CHECKSUM_MATCHES = SteamFields.GetBoolField("CHECK_IF_LIBRARY_CHECKSUM_MATCHES");
		WARRIORB_STEAM_LIBRARY_WINDOWS_API64_SHA1_SUM = SteamFields.GetStringField("LIBRARY_WINDOWS_API64_SHA1_SUM");

		JsonObject DiscordFields;
		if (AllFields.TryGetObjectField("Discord", out DiscordFields))
		{
			WARRIORB_WITH_DISCORD = DiscordFields.GetBoolField("IS_ENABLED");
			WARRIORB_DISCORD_CLIENT_ID = Convert.ToInt64(DiscordFields.GetStringField("CLIENT_ID"));
			WARRIORB_DISCORD_DEMO_CLIENT_ID = Convert.ToInt64(DiscordFields.GetStringField("CLIENT_ID_DEMO"));
		}

		WARRIORB_WITH_VIDEO_DEMO = VideoFields.GetBoolField("DEMO");
		WARRIORB_WITH_VIDEO_INTRO = VideoFields.GetBoolField("INTRO");

		WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT = PublicDefinitionsFields.GetBoolField("WARRIORB_ALLOW_CONSOLE_CHEATS_DEFAULT");
		WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS = PublicDefinitionsFields.GetStringField("WARRIORB_PASSWORD_ENABLE_CONSOLE_CHEATS");
		WARRIORB_PASSWORD_SAVE_FILE = PublicDefinitionsFields.GetStringField("WARRIORB_PASSWORD_SAVE_FILE");

		LoadingImage = LoadingFields.GetStringField("Image");
		LoadingImageDemo = LoadingFields.GetStringField("ImageDemo");
	}

	//
	// Other
	//

	static void PatchFileSingle(string filename, string oldValue, string newValue)
	{
		string text = File.ReadAllText(filename);
		text = text.Replace(oldValue, newValue);
		File.WriteAllText(filename, text);
	}

	static void PatchFile(string filename, Dictionary<string, string> Replacements)
	{
		if (Replacements.Count == 0)
			return;

		string text = File.ReadAllText(filename);
		foreach (var KeyValue in Replacements)
		{
			text = text.Replace(KeyValue.Key, KeyValue.Value);
		}
		File.WriteAllText(filename, text);
	}

	static string IntToString(int value)
	{
		return value.ToString();
	}

	static string Int64ToString(long value)
	{
		return value.ToString();
	}

	static string BoolToIntString(bool value)
	{
		return value ? "1" : "0";
	}

	static void PrintBlue(string String)
	{
		Console.ForegroundColor = ConsoleColor.Blue;
		Console.WriteLine(String);
		Console.ResetColor();
	}

	static void PrintRed(string String)
	{
		Console.ForegroundColor = ConsoleColor.Red;
		Console.WriteLine(String);
		Console.ResetColor();
	}

	static void PrintGreen(string String)
	{
		Console.ForegroundColor = ConsoleColor.Green;
		Console.WriteLine(String);
		Console.ResetColor();
	}

	static void PrintYellow(string String)
	{
		Console.ForegroundColor = ConsoleColor.Yellow;
		Console.WriteLine(String);
		Console.ResetColor();
	}

	//
	// Print out a build message
	// Why error? Well, the UE masks all other errors. *shrug*
	//
	private void Trace(string msg)
	{
		Log.TraceInformation("Warriorb: " + msg);
	}

	// Trace helper
	private void Trace(string format, params object[] args)
	{
		Trace(string.Format(format, args));
	}

	// Raise an error
	private void Fail(string message)
	{
		Trace(message);
		throw new Exception(message);
	}

	public static bool IsLinux
	{
		get
		{
		   return RuntimeInformation.IsOSPlatform(OSPlatform.Linux);
		}
	}

	public static bool ExistsOnPath(string exeName)
	{
		try
		{
			using (Process p = new Process())
			{
				p.StartInfo.CreateNoWindow = true;
				p.StartInfo.UseShellExecute = false;
				p.StartInfo.RedirectStandardError = true;
				p.StartInfo.RedirectStandardOutput = true;
				p.StartInfo.FileName = IsLinux ? "whereis" : "where";
				p.StartInfo.Arguments = exeName;
				p.Start();
				p.WaitForExit();
				return p.ExitCode == 0;
			}
		}
		catch (Exception)
		{
			return false;
		}
	}

	public static bool RunCommand(string workingDirectory,  string commandName, string arguments, ref string output)
	{
		using (Process p = new Process())
		{
			p.StartInfo.WorkingDirectory = workingDirectory;
			p.StartInfo.CreateNoWindow = true;
			p.StartInfo.UseShellExecute = false;
			p.StartInfo.RedirectStandardError = true;
			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.FileName = commandName;
			p.StartInfo.Arguments = arguments;
			p.Start();
			output = p.StandardOutput.ReadToEnd();
			p.WaitForExit();
			return p.ExitCode == 0;
		}
	}

	public static bool GetGitBranch(string path, ref string branch)
	{
		if (RunCommand(path, "git", "rev-parse --abbrev-ref HEAD", ref branch))
		{
			branch = branch.Trim();
			return true;
		}
		return false;
	}

	public static bool GetGitCommit(string path, ref string commit)
	{
		if (RunCommand(path, "git", "rev-parse --short HEAD", ref commit))
		{
			commit = commit.Trim();
			return true;
		}
		return false;
	}

	public bool GetCurrentBranchAndCommit(ref string branch, ref string commit)
	{
		// Try first from git command
		if (GeBranchAndCommitFromGit(ref branch, ref commit))
			return true;

		return false;
	}

	public bool GeBranchAndCommitFromGit(ref string branch, ref string commit)
	{
		if (!ExistsOnPath("git"))
		{
			PrintYellow("git does not exist in path");
			return false;
		}

		PrintBlue("Using git");
		if (!GetGitBranch(ProjectRootPath, ref branch))
		{
			PrintRed("Can't get git branch");
			return false;
		}
		if (!GetGitCommit(ProjectRootPath, ref commit))
		{
			PrintRed("Can't get git commit");
			return false;
		}
		return true;
	}

	//
	// END COMMON
	//
}
