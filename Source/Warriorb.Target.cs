// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Tools.DotNETCommon;

using UnrealBuildTool;

public class WarriorbTarget : TargetRules
{
	// See https://docs.unrealengine.com/en-us/Programming/UnrealBuildSystem/TargetFiles for documentation
	public WarriorbTarget(TargetInfo Target) : base(Target)
	{
		Console.WriteLine("Using Warriorb.Target.cs");

		Type = TargetType.Game;
		ExtraModuleNames.Add("SOrb");
		//BuildEnvironment = TargetBuildEnvironment.Unique;

		Init(Target);
	}

	public void Init(TargetInfo Target)
	{
		InitFromPlatform(Target.Platform);
		ParseBuildFile();

		// Only enable steam on desktop
		bUsesSteam = WARRIORB_WITH_STEAM;
		//if (bUsesSteam)
		//{
		//	EnablePlugins.Add("NotYetSteam");
		//}
		//else
		//{
		//	DisablePlugins.Add("NotYetSteam");
		//}

		// IWYU
		bIWYU = true;
		bEnforceIWYU = true;

		// Omits subfolders from public include paths to reduce compiler command line length.
		bLegacyPublicIncludePaths = false;

		// More Errors
		bUndefinedIdentifierErrors = true;
		bShadowVariableErrors = true;

		// TODO

		//WindowsPlatform.CompanyName = "WTF";
		//WindowsPlatform.ProductName = "WTF";
		//WindowsPlatform.CopyrightNotice = "WTF";

		// Only on desktop
		WindowsPlatform.bStrictConformanceMode = IsSupportedDesktopPlatform;
		// WindowsPlatform.bUseBundledDbgHelp = false;

		// https://twitter.com/kantandev/status/1038037592174981121
		//bLegacyPublicIncludePaths = false;

		// Whether to turn on checks (asserts) for test/shipping builds.
		//bUseChecksInShipping = true;
		//bUseLoggingInShipping = true;

		//bLoggingToMemoryEnabled - true;
		//bForceEnableExceptions = true;
		//bForceEnableObjCExceptions = true;
		//bForceEnableRTTI = true;

		//bForceCompileDevelopmentAutomationTests = true;
		//bForceCompilePerformanceAutomationTests = true;

		// TODO set UE4_PROJECT_STEAMSHIPPINGID for shipping builds

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
	public string ProjectRootPath { get { return ProjectFile.Directory.ToString(); } }
	public string ProjectSourceDirPath { get { return ProjectRootPath + "/Source"; } }
	public string ProjectConfigDirPath { get { return ProjectRootPath + "/Config"; } }
	public string BuildConfigFilePath { get { return ProjectSourceDirPath + "/" + WARRIORB_BUILD_FILE_NAME; } }

	public bool WARRIORB_WITH_STEAM = false;
	public string WARRIORB_BUILD_VERSION;

	public void ParseBuildFile()
	{
		PrintBlue(String.Format("Reading from BuildConfigFile = {0}", BuildConfigFilePath));
		var AllFields = JsonObject.Read(new FileReference(BuildConfigFilePath));
		var PublicDefinitionsFields = AllFields.GetObjectField("PublicDefinitions");
		var SteamFields = AllFields.GetObjectField("Steam");

		WARRIORB_BUILD_VERSION = AllFields.GetStringField("Version");
		WARRIORB_WITH_STEAM = SteamFields.GetBoolField("IS_ENABLED");

		if (IsSupportedConsolePlatform)
		{
			// Aka !IsSupportedDesktopPlatform
			PrintYellow("Disabling Steam support because we are building for console");
			WARRIORB_WITH_STEAM = false;
		}
	}

	//
	// Other
	//

	static string IntToString(int value)
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

		// Use JSON file
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
