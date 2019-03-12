// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VictoryBPLibrary : ModuleRules
{
	public VictoryBPLibrary(TargetInfo Target)
	{
        PublicDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject", 
				"Engine", 
				"InputCore",
				"RHI"
			}
		);
		//Private Paths
        PrivateIncludePaths.AddRange(new string[] { 
			"VictoryBPLibrary/Public",
			"VictoryBPLibrary/Private"
		});
	}
}
