// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DFInventory : ModuleRules
{
	public DFInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new []
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"NetCore",
			"OnlineSubsystem",
			"CoreOnline",
			"GameplayTags",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new []
		{
			"FunctionalTesting",
			"UnrealEd"
		});
	}
}
