using UnrealBuildTool;
using System.IO;

public class OpenPursuitLab : ModuleRules
{
    public OpenPursuitLab(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true; // Shared policy core uses standard exceptions.
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "NetCore" });
        // Declared integration dependencies; no vehicle/Mass/StateTree adapters exist yet.
        PrivateDependencyModuleNames.AddRange(new[] { "ChaosVehicles", "MassEntity", "StateTreeModule" });
        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../include")));
    }
}
