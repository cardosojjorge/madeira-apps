using UnrealBuildTool;

public class HowDidIDieEditor : ModuleRules
{
	public HowDidIDieEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"ToolMenus",
			"Slate",
			"SlateCore",
			"HowDidIDie"
		});
	}
}
