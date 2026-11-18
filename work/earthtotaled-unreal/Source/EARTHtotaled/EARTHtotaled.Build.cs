using UnrealBuildTool;
public class EARTHtotaled : ModuleRules
{
    public EARTHtotaled(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent", "Json", "JsonUtilities", "Slate", "SlateCore" });
    }
}
