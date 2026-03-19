// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VarcoSound : ModuleRules
{
	public VarcoSound(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Slate",
				"SlateCore"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"EditorStyle",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"LevelEditor",
				"AssetTools",
				"PropertyEditor",
				"Json",
				"JsonUtilities",
				"HTTP",
				"AssetRegistry",
				"ImageWrapper",
				"AudioCapture",
				"AudioCaptureCore"
			}
		);
		
		// Add ThirdParty include path for minimp3
		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory));
		
        // PCH memory limit increase (Prevents Live Coding C1076 error)
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDefinitions.Add("_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING");

            // Increase MSVC compiler memory limit
            bUseUnity = false; // Disable Unity build to reduce PCH load
            
            // Additional settings for PCH memory issues
            MinFilesUsingPrecompiledHeaderOverride = 1;
            bUsePrecompiled = false;
        }
    }
}
