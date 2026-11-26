using UnrealBuildTool;
public class EARTHtotaled : ModuleRules
{
    public EARTHtotaled(ReadOnlyTargetRules Target) : base(Target)
    {
        if (Target.Platform == UnrealTargetPlatform.Win64) PublicSystemLibraries.AddRange(new[] { "ole32.lib", "shell32.lib", "uuid.lib" });
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "ApplicationCore", "ProceduralMeshComponent", "Json", "JsonUtilities", "Slate", "SlateCore" });
    }
}
