// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ZippyEditorTarget : TargetRules
{
	public ZippyEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		
		ExtraModuleNames.Add("Zippy");
	}
}
