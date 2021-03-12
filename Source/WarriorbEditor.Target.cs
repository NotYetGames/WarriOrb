// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;
using System.Collections.Generic;

public class WarriorbEditorTarget : TargetRules
{
	public WarriorbEditorTarget(TargetInfo Target) : base(Target)
	{
		Console.WriteLine("Using WarriorbEditor.Target.cs");

		Type = TargetType.Editor;
		ExtraModuleNames.Add("SOrb");
		ExtraModuleNames.Add("SOrbEditor");

		Init(Target);
	}

	public void Init(TargetInfo Target)
	{
		// Omits subfolders from public include paths to reduce compiler command line length.
		bLegacyPublicIncludePaths = false;

		// More Errors
		bUndefinedIdentifierErrors = true;
		bShadowVariableErrors = true;

		// Platform specific
		WindowsPlatform.bStrictConformanceMode = true;
	}
}
