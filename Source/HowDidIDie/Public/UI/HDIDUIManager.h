#pragma once

#include "CoreMinimal.h"
#include "Data/HDIDTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HDIDUIManager.generated.h"

class AHDIDPlayerController;
class UHDIDListWidget;
class UHDIDDialogueWidget;
class UHDIDTimelineWidget;
class UHDIDSettingsWidget;
class UHDIDHudWidget;
class UUserWidget;

UENUM()
enum class EHDIDScreen : uint8
{
	Main,
	Chapters,
	EvidenceCategories,
	EvidenceEntries,
	Pause,
	NotebookCategories,
	NotebookEntries,
	Settings,
	Dialogue,
	Timeline,
	ChapterEnd
};

/** Intended Blueprint wrapper: BP_UIManager. */
UCLASS()
class HOWDIDIDIE_API UHDIDUIManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void ShowMainMenu();
	void ShowGameplay();
	void PushChapters();
	void PushEvidence();
	void PushSettings();
	void PushPause();
	void ToggleNotebook();
	void ToggleTimeline(const FString& FocusEventId);
	void ShowDialogue(const FHDIDDialogueNode& Node, const TArray<FHDIDChoiceDef>& Choices);
	void CloseTop();
	void ShowChapterComplete(const FString& Title);
	void Navigate(int32 Delta);
	void NavigateHorizontal(int32 Delta);
	void Confirm();
	void Back();
	bool IsOverlayOpen() const { return Stack.Num() > 0; }
	bool ConsumesGameplay() const;
	void SetPrompt(const FString& Text, const FVector& WorldLocation, bool bVisible);
	void ShowCard(const FString& IndexLabel, const FString& Title, const FString& Body);
	void ShowNotice(const FString& Text);
	void ShowExamine(const FString& Title, const FString& Body, bool bHeld);
	void ToggleLocationCard();
	void SetLetterbox(float Alpha);
	void SetSubtitle(const FString& Speaker, const FString& Line);
	APlayerController* GetPlayer() const;

	bool HandleRebindKey(const FKey& Key);

private:
	void Open(EHDIDScreen Screen);
	void SyncInputMode();
	void ShowEntries(EHDIDScreen Screen, const FString& Category);
	FString SelectedId() const;

	void RebuildTop();
	void EnsureHud();
	UHDIDListWidget* MakeList(const FString& Heading, const FString& Footer, const TArray<FHDIDListItem>& Items, bool bRain);

	TArray<EHDIDScreen> Stack;
	FString EntryCategory;
	FString TimelineFocus;
	FString ChapterEndTitle;
	FString MenuFooter;
	bool bReplaceConfirm = false;

	UPROPERTY()
	TObjectPtr<UUserWidget> Overlay;

	UPROPERTY()
	TObjectPtr<UHDIDHudWidget> Hud;
};
