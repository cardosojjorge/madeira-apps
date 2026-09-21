using UnrealBuildTool;
using System.Collections.Generic;

public class HowDidIDieTarget : TargetRules
{
	public HowDidIDieTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		// Unreal5_4 keeps the documented UE 5.4 fallback compiling.
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("HowDidIDie");
	}
}
