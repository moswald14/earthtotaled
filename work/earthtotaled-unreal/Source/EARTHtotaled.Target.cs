using UnrealBuildTool;
using System.Collections.Generic;
public class EARTHtotaledTarget : TargetRules
{
    public EARTHtotaledTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EARTHtotaled");
    }
}
