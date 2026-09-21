#include "CoreMinimal.h"
#include "Data/HDIDNarrativeValidator.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "HowDidIDieEditor"

class FHowDidIDieEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		StartupHandle = UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FHowDidIDieEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (StartupHandle.IsValid())
		{
			UToolMenus::UnRegisterStartupCallback(StartupHandle);
			StartupHandle.Reset();
		}
	}

private:
	void RegisterMenus()
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& Section = Menu->AddSection("HDID", LOCTEXT("Section", "HOW DID I DIE?"));
		Section.AddMenuEntry(
			"HDIDValidateNarrative",
			LOCTEXT("Validate", "Validate Narrative"),
			LOCTEXT("ValidateTip", "Check chapter JSON, clues, branches, cross-chapter references, and save fields."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FHowDidIDieEditorModule::Validate)));
	}

	static void Validate()
	{
		const FHDIDValidationReport Report = FHDIDNarrativeValidator::ValidateProject();
		FHDIDNarrativeValidator::LogReport(Report);
		FString Body;
		if (Report.IsValid())
		{
			Body = TEXT("Narrative data valid: 10 chapters, Chapter 01 playable, late game locked.");
		}
		else
		{
			Body = FString::Printf(TEXT("NARRATIVE VALIDATION FAILED (%d)\n"), Report.Errors.Num());
			const int32 Shown = FMath::Min(Report.Errors.Num(), 12);
			for (int32 Index = 0; Index < Shown; ++Index)
			{
				Body += TEXT("\n- ");
				Body += Report.Errors[Index];
			}
			if (Report.Errors.Num() > Shown)
			{
				Body += TEXT("\n\nSee the Output Log for the rest.");
			}
		}
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Body), FText::FromString(TEXT("HOW DID I DIE?")));
	}

	FDelegateHandle StartupHandle;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHowDidIDieEditorModule, HowDidIDieEditor)
