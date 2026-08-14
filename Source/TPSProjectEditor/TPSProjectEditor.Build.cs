// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPSProjectEditor : ModuleRules
{
	public TPSProjectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TPSProject"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetTools",
			"HTTP",
			"Json",
			"UnrealEd"
		});
	}
}
