// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DFInventoryEditor : ModuleRules
{
	public DFInventoryEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"DeveloperSettings",
			"PropertyEditor",
			"DFInventory",
			"AssetRegistry",
			"Slate",
			"SlateCore",
			"BlueprintGraph",
		});
	}
}
