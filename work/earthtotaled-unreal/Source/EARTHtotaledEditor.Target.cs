using UnrealBuildTool;
using System.Collections.Generic;
public class EARTHtotaledEditorTarget : TargetRules
{
    public EARTHtotaledEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("EARTHtotaled");
    }
}
