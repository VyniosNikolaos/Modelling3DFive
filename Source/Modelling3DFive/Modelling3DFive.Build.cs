// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Modelling3DFive : ModuleRules
{
	public Modelling3DFive(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent", "GeometryScriptingCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {
			"GeometryCore",
			"GeometryFramework",
			"DynamicMesh",
			"MeshDescription",
			"StaticMeshDescription",
			"MeshConversion",
			"GeometryAlgorithms"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
