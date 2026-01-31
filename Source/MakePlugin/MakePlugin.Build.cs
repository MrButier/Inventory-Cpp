// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MakePlugin : ModuleRules
{
	public MakePlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG"
		});
}
}
