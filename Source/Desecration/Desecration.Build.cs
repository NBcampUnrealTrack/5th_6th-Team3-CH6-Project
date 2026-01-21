// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Desecration : ModuleRules
{
	public Desecration(ReadOnlyTargetRules Target) : base(Target)
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
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Desecration",
			"Desecration/Variant_Platforming",
			"Desecration/Variant_Platforming/Animation",
			"Desecration/Variant_Combat",
			"Desecration/Variant_Combat/AI",
			"Desecration/Variant_Combat/Animation",
			"Desecration/Variant_Combat/Gameplay",
			"Desecration/Variant_Combat/Interfaces",
			"Desecration/Variant_Combat/UI",
			"Desecration/Variant_SideScrolling",
			"Desecration/Variant_SideScrolling/AI",
			"Desecration/Variant_SideScrolling/Gameplay",
			"Desecration/Variant_SideScrolling/Interfaces",
			"Desecration/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
