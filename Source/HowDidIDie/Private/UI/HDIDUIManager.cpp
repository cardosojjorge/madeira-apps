#include "UI/HDIDUIManager.h"

#include "Core/HDIDGameMode.h"
#include "Core/HDIDPlayerController.h"
#include "Data/HDIDCatalog.h"
#include "Save/HDIDSaveGame.h"
#include "Systems/HDIDManagers.h"
#include "UI/HDIDWidgets.h"

namespace
{
	const TCHAR* Categories[] = {
		TEXT("People"), TEXT("Locations"), TEXT("Objects"), TEXT("Clues"),
		TEXT("Memories"), TEXT("Events"), TEXT("Timeline"), TEXT("Connections")
	};
}

void UHDIDUIManager::ShowMainMenu()
{
	bReplaceConfirm = false;
	Stack.Reset();
	if (Overlay)
	{
		Overlay->RemoveFromParent();
		Overlay = nullptr;
	}
	EnsureHud();
	if (Hud)
	{
		Hud->SetVisibility(ESlateVisibility::Collapsed);
	}
	Open(EHDIDScreen::Main);
}

void UHDIDUIManager::ShowGameplay()
{
	bReplaceConfirm = false;
	Stack.Reset();
	if (Overlay)
	{
		Overlay->RemoveFromParent();
		Overlay = nullptr;
	}
	EnsureHud();
	if (Hud)
	{
		Hud->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	SyncInputMode();
}

void UHDIDUIManager::PushChapters()
{
	Open(EHDIDScreen::Chapters);
}

void UHDIDUIManager::PushEvidence()
{
	Open(EHDIDScreen::EvidenceCategories);
}

void UHDIDUIManager::PushSettings()
{
	Open(EHDIDScreen::Settings);
}

void UHDIDUIManager::PushPause()
{
	if (Stack.Num() > 0 && Stack.Last() == EHDIDScreen::Pause)
	{
		return;
	}
	if (UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>())
	{
		Save->WriteStory();
	}
	Open(EHDIDScreen::Pause);
}

void UHDIDUIManager::ToggleNotebook()
{
	if (Stack.Contains(EHDIDScreen::Dialogue) || Stack.Contains(EHDIDScreen::ChapterEnd))
	{
		return;
	}
	if (Stack.Num() > 0 && (Stack.Last() == EHDIDScreen::NotebookCategories || Stack.Last() == EHDIDScreen::NotebookEntries))
	{
		while (Stack.Num() > 0 && (Stack.Last() == EHDIDScreen::NotebookCategories || Stack.Last() == EHDIDScreen::NotebookEntries))
		{
			Stack.Pop();
		}
		if (Overlay)
		{
			Overlay->RemoveFromParent();
			Overlay = nullptr;
		}
		if (Stack.Num() == 0)
		{
			SyncInputMode();
		}
		else
		{
			RebuildTop();
		}
		return;
	}
	Open(EHDIDScreen::NotebookCategories);
}

void UHDIDUIManager::ToggleTimeline(const FString& FocusEventId)
{
	if (Stack.Contains(EHDIDScreen::Dialogue) || Stack.Contains(EHDIDScreen::ChapterEnd))
	{
		return;
	}
	if (Stack.Num() > 0 && Stack.Last() == EHDIDScreen::Timeline)
	{
		Back();
		return;
	}
	const AHDIDGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AHDIDGameMode>() : nullptr;
	if (!Mode || !Mode->IsInSession())
	{
		return;
	}
	TimelineFocus = FocusEventId;
	Open(EHDIDScreen::Timeline);
	if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
	{
		Cards->NotifyTimelineOpened();
	}
}

void UHDIDUIManager::ShowDialogue(const FHDIDDialogueNode& Node, const TArray<FHDIDChoiceDef>& Choices)
{
	if (Stack.Num() > 0 && Stack.Last() == EHDIDScreen::Dialogue)
	{
		if (UHDIDDialogueWidget* Dialogue = Cast<UHDIDDialogueWidget>(Overlay))
		{
			Dialogue->Setup(this, Node, Choices);
			return;
		}
	}
	APlayerController* Controller = GetPlayer();
	if (!Controller)
	{
		return;
	}
	Stack.Add(EHDIDScreen::Dialogue);
	UHDIDDialogueWidget* Dialogue = CreateWidget<UHDIDDialogueWidget>(Controller, UHDIDDialogueWidget::StaticClass());
	Dialogue->Setup(this, Node, Choices);
	if (Overlay)
	{
		Overlay->RemoveFromParent();
	}
	Overlay = Dialogue;
	Dialogue->AddToViewport(30);
	SyncInputMode();
}

void UHDIDUIManager::CloseTop()
{
	if (Stack.Num() > 0 && Stack.Last() == EHDIDScreen::Dialogue)
	{
		Stack.Pop();
		if (Overlay)
		{
			Overlay->RemoveFromParent();
			Overlay = nullptr;
		}
		if (Stack.Num() == 0)
		{
			SyncInputMode();
		}
		else
		{
			RebuildTop();
		}
	}
}

void UHDIDUIManager::ShowChapterComplete(const FString& Title)
{
	if (Stack.Contains(EHDIDScreen::ChapterEnd))
	{
		return;
	}
	ChapterEndTitle = Title;
	Open(EHDIDScreen::ChapterEnd);
}

void UHDIDUIManager::Navigate(int32 Delta)
{
	if (Stack.Num() == 0)
	{
		return;
	}
	if (Stack.Last() == EHDIDScreen::Main)
	{
		bReplaceConfirm = false;
		if (UHDIDListWidget* List = Cast<UHDIDListWidget>(Overlay))
		{
			List->SetFooter(MenuFooter);
			List->Navigate(Delta);
		}
		return;
	}
	if (UHDIDListWidget* List = Cast<UHDIDListWidget>(Overlay))
	{
		List->Navigate(Delta);
	}
	else if (UHDIDDialogueWidget* Dialogue = Cast<UHDIDDialogueWidget>(Overlay))
	{
		Dialogue->Navigate(Delta);
	}
	else if (UHDIDTimelineWidget* Timeline = Cast<UHDIDTimelineWidget>(Overlay))
	{
		Timeline->Navigate(Delta);
	}
	else if (UHDIDSettingsWidget* Settings = Cast<UHDIDSettingsWidget>(Overlay))
	{
		Settings->Navigate(Delta);
	}
}

void UHDIDUIManager::NavigateHorizontal(int32 Delta)
{
	if (UHDIDTimelineWidget* Timeline = Cast<UHDIDTimelineWidget>(Overlay))
	{
		Timeline->NavigateHorizontal(Delta);
	}
	else if (UHDIDSettingsWidget* Settings = Cast<UHDIDSettingsWidget>(Overlay))
	{
		Settings->NavigateHorizontal(Delta);
	}
}

void UHDIDUIManager::Confirm()
{
	if (Stack.Num() == 0)
	{
		return;
	}
	AHDIDPlayerController* Controller = Cast<AHDIDPlayerController>(GetPlayer());
	const EHDIDScreen Screen = Stack.Last();
	const FString Id = SelectedId();

	if (Screen == EHDIDScreen::Main)
	{
		if (Id == TEXT("new"))
		{
			const UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
			if (Save && Save->HasStorySave() && !bReplaceConfirm)
			{
				bReplaceConfirm = true;
				if (UHDIDListWidget* List = Cast<UHDIDListWidget>(Overlay))
				{
					List->SetFooter(TEXT("NEW GAME replaces the saved hour."));
				}
				return;
			}
			if (Controller)
			{
				Controller->RequestNewGame();
			}
		}
		else if (Id == TEXT("continue") && Controller)
		{
			Controller->RequestContinue();
		}
		else if (Id == TEXT("chapters"))
		{
			PushChapters();
		}
		else if (Id == TEXT("evidence"))
		{
			PushEvidence();
		}
		else if (Id == TEXT("settings"))
		{
			PushSettings();
		}
		else if (Id == TEXT("quit") && Controller)
		{
			Controller->RequestQuit();
		}
		return;
	}

	if (Screen == EHDIDScreen::Chapters)
	{
		UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
		if (!Catalog || !Catalog->IsPlayable(Id))
		{
			ShowNotice(TEXT("This chapter is recorded. It is not open."));
			return;
		}
		if (Controller)
		{
			Controller->RequestChapter(Id);
		}
		return;
	}

	if (Screen == EHDIDScreen::EvidenceCategories || Screen == EHDIDScreen::NotebookCategories)
	{
		ShowEntries(Screen == EHDIDScreen::EvidenceCategories ? EHDIDScreen::EvidenceEntries : EHDIDScreen::NotebookEntries, Id);
		return;
	}

	if (Screen == EHDIDScreen::Pause)
	{
		if (Id == TEXT("resume"))
		{
			Back();
		}
		else if (Id == TEXT("notebook"))
		{
			Open(EHDIDScreen::NotebookCategories);
		}
		else if (Id == TEXT("settings"))
		{
			PushSettings();
		}
		else if (Id == TEXT("menu") && Controller)
		{
			Controller->ReturnToMenu();
		}
		else if (Id == TEXT("quit") && Controller)
		{
			Controller->RequestQuit();
		}
		return;
	}

	if (Screen == EHDIDScreen::Settings)
	{
		if (UHDIDSettingsWidget* Settings = Cast<UHDIDSettingsWidget>(Overlay))
		{
			Settings->Confirm();
		}
		return;
	}

	if (Screen == EHDIDScreen::Dialogue)
	{
		if (UHDIDDialogueManager* Dialogue = GetGameInstance()->GetSubsystem<UHDIDDialogueManager>())
		{
			if (UHDIDDialogueWidget* Widget = Cast<UHDIDDialogueWidget>(Overlay))
			{
				if (Widget->HasChoices())
				{
					Dialogue->Choose(Widget->GetSelectedIndex());
				}
				else
				{
					Dialogue->Advance();
				}
			}
		}
		return;
	}

	if (Screen == EHDIDScreen::Timeline)
	{
		if (UHDIDTimelineWidget* Timeline = Cast<UHDIDTimelineWidget>(Overlay))
		{
			Timeline->Confirm();
		}
		return;
	}

	if (Screen == EHDIDScreen::ChapterEnd)
	{
		if (Id == TEXT("menu") && Controller)
		{
			Controller->ReturnToMenu();
		}
		else
		{
			Back();
		}
	}
}

void UHDIDUIManager::Back()
{
	if (UHDIDSettingsWidget* Settings = Cast<UHDIDSettingsWidget>(Overlay))
	{
		if (Settings->IsWaiting())
		{
			Settings->FinishRebind();
			if (AHDIDPlayerController* Controller = Cast<AHDIDPlayerController>(GetPlayer()))
			{
				Controller->SetRebindCapture(false, NAME_None);
			}
			return;
		}
	}
	if (Stack.Num() == 0)
	{
		return;
	}
	if (Stack.Last() == EHDIDScreen::Main)
	{
		bReplaceConfirm = false;
		if (UHDIDListWidget* List = Cast<UHDIDListWidget>(Overlay))
		{
			List->SetFooter(MenuFooter);
		}
		return;
	}
	if (Stack.Last() == EHDIDScreen::Dialogue)
	{
		if (UHDIDDialogueManager* Dialogue = GetGameInstance()->GetSubsystem<UHDIDDialogueManager>())
		{
			Dialogue->Close();
		}
		return;
	}
	Stack.Pop();
	if (Overlay)
	{
		Overlay->RemoveFromParent();
		Overlay = nullptr;
	}
	if (Stack.Num() == 0)
	{
		SyncInputMode();
	}
	else
	{
		RebuildTop();
	}
}

bool UHDIDUIManager::ConsumesGameplay() const
{
	return Stack.Num() > 0;
}

void UHDIDUIManager::SetPrompt(const FString& Text, const FVector& WorldLocation, bool bVisible)
{
	EnsureHud();
	if (!Hud)
	{
		return;
	}
	FVector2D Screen(960.f, 800.f);
	bool bOnScreen = false;
	if (APlayerController* Controller = GetPlayer())
	{
		bOnScreen = Controller->ProjectWorldLocationToScreen(WorldLocation, Screen, true);
	}
	Hud->SetPrompt(Text, Screen, bVisible && bOnScreen);
}

void UHDIDUIManager::ShowCard(const FString& IndexLabel, const FString& Title, const FString& Body)
{
	EnsureHud();
	if (Hud)
	{
		Hud->ShowCard(IndexLabel, Title, Body);
	}
}

void UHDIDUIManager::ShowNotice(const FString& Text)
{
	EnsureHud();
	if (Hud)
	{
		Hud->ShowNotice(Text);
	}
	if (UHDIDListWidget* List = Cast<UHDIDListWidget>(Overlay))
	{
		List->SetFooter(Text);
	}
}

void UHDIDUIManager::ShowExamine(const FString& Title, const FString& Body, bool bHeld)
{
	EnsureHud();
	if (Hud)
	{
		Hud->ShowExamine(Title, Body, bHeld);
	}
}

void UHDIDUIManager::ToggleLocationCard()
{
	if (ConsumesGameplay())
	{
		return;
	}
	EnsureHud();
	if (Hud)
	{
		Hud->ToggleLocation();
	}
}

void UHDIDUIManager::SetLetterbox(float Alpha)
{
	EnsureHud();
	if (Hud)
	{
		Hud->SetLetterbox(Alpha);
	}
}

void UHDIDUIManager::SetSubtitle(const FString& Speaker, const FString& Line)
{
	EnsureHud();
	if (Hud)
	{
		Hud->SetSubtitle(Speaker, Line);
	}
}

APlayerController* UHDIDUIManager::GetPlayer() const
{
	return GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
}

bool UHDIDUIManager::HandleRebindKey(const FKey& Key)
{
	UHDIDSettingsWidget* Settings = Cast<UHDIDSettingsWidget>(Overlay);
	if (!Settings || !Settings->IsWaiting() || !Key.IsValid())
	{
		return false;
	}
	AHDIDPlayerController* Controller = Cast<AHDIDPlayerController>(GetPlayer());
	if (Key == EKeys::Escape)
	{
		Settings->FinishRebind();
		if (Controller)
		{
			Controller->SetRebindCapture(false, NAME_None);
		}
		return true;
	}
	if (Key == EKeys::MouseX || Key == EKeys::MouseY || Key == EKeys::Mouse2D || Key == EKeys::MouseScrollUp || Key == EKeys::MouseScrollDown || Key == EKeys::MouseWheelAxis)
	{
		return true;
	}
	if (Controller && Controller->GetInputSetup())
	{
		Controller->GetInputSetup()->SetSlotKey(Settings->GetWaitingSlot(), Key);
	}
	if (UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>())
	{
		if (UHDIDSettingsSave* Data = Save->GetSettings())
		{
			const FString SlotId = Settings->GetWaitingSlot().ToString();
			bool bFound = false;
			for (FHDIDKeyRebind& Rebind : Data->Rebinds)
			{
				if (Rebind.SlotId == SlotId)
				{
					Rebind.KeyName = Key.ToString();
					bFound = true;
				}
			}
			if (!bFound)
			{
				FHDIDKeyRebind Rebind;
				Rebind.SlotId = SlotId;
				Rebind.KeyName = Key.ToString();
				Data->Rebinds.Add(Rebind);
			}
			Save->WriteSettings();
		}
	}
	Settings->FinishRebind();
	if (Controller)
	{
		Controller->SetRebindCapture(false, NAME_None);
	}
	return true;
}

void UHDIDUIManager::Open(EHDIDScreen Screen)
{
	Stack.Add(Screen);
	RebuildTop();
}

void UHDIDUIManager::EnsureHud()
{
	if (Hud && Hud->IsInViewport())
	{
		return;
	}
	APlayerController* Controller = GetPlayer();
	if (!Controller)
	{
		return;
	}
	Hud = CreateWidget<UHDIDHudWidget>(Controller, UHDIDHudWidget::StaticClass());
	if (!Hud)
	{
		return;
	}
	Hud->Configure();
	Hud->AddToViewport(5);
}

UHDIDListWidget* UHDIDUIManager::MakeList(const FString& Heading, const FString& Footer, const TArray<FHDIDListItem>& Items, bool bRain)
{
	APlayerController* Controller = GetPlayer();
	if (!Controller)
	{
		return nullptr;
	}
	UHDIDListWidget* List = CreateWidget<UHDIDListWidget>(Controller, UHDIDListWidget::StaticClass());
	if (!List)
	{
		return nullptr;
	}
	List->Setup(this, Heading, Footer, Items, bRain);
	if (Overlay)
	{
		Overlay->RemoveFromParent();
	}
	Overlay = List;
	List->AddToViewport(30);
	SyncInputMode();
	return List;
}

void UHDIDUIManager::SyncInputMode()
{
	AHDIDPlayerController* Controller = Cast<AHDIDPlayerController>(GetPlayer());
	if (!Controller)
	{
		return;
	}
	const AHDIDGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AHDIDGameMode>() : nullptr;
	if (Stack.Num() == 0 && Mode && Mode->IsInSession())
	{
		Controller->EnterGameplay();
	}
	else
	{
		Controller->EnterMenu();
	}
}

void UHDIDUIManager::ShowEntries(EHDIDScreen Screen, const FString& Category)
{
	EntryCategory = Category;
	const AHDIDGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AHDIDGameMode>() : nullptr;
	if ((!Mode || !Mode->IsInSession()))
	{
		if (UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>())
		{
			if (Save->HasStorySave())
			{
				Save->LoadStory();
			}
		}
	}
	Open(Screen);
}

FString UHDIDUIManager::SelectedId() const
{
	if (const UHDIDListWidget* List = Cast<UHDIDListWidget>(Overlay))
	{
		return List->GetSelectedId();
	}
	return FString();
}

void UHDIDUIManager::RebuildTop()
{
	if (Stack.Num() == 0)
	{
		return;
	}
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	TArray<FHDIDListItem> Items;
	const EHDIDScreen Screen = Stack.Last();

	if (Screen == EHDIDScreen::Main)
	{
		FString Clock = Catalog ? Catalog->GetGameText().MenuClockFallback : TEXT("23:47");
		if (Catalog)
		{
			if (const FHDIDChapterFile* Chapter = Catalog->FindChapter(Catalog->GetFirstPlayableChapterId()))
			{
				if (!Chapter->Clock.IsEmpty())
				{
					Clock = Chapter->Clock;
				}
			}
		}
		const FString Title = Catalog ? Catalog->GetGameText().Title : TEXT("HOW DID I DIE?");
		const FString Tagline = Catalog ? Catalog->GetGameText().Tagline : TEXT("EVERY CHOICE HAS A MEMORY.");
		MenuFooter = Tagline + TEXT("\n") + Clock;
		auto Add = [&Items](const TCHAR* Id, const TCHAR* Label)
		{
			FHDIDListItem Item;
			Item.Id = Id;
			Item.Label = Label;
			Items.Add(Item);
		};
		Add(TEXT("new"), TEXT("NEW GAME"));
		Add(TEXT("continue"), TEXT("CONTINUE"));
		Add(TEXT("chapters"), TEXT("CHAPTERS"));
		Add(TEXT("evidence"), TEXT("EVIDENCE"));
		Add(TEXT("settings"), TEXT("SETTINGS"));
		Add(TEXT("quit"), TEXT("QUIT"));
		MakeList(Title, MenuFooter, Items, true);
		return;
	}

	if (Screen == EHDIDScreen::Chapters && Catalog)
	{
		for (const FString& ChapterId : Catalog->GetChapterIds())
		{
			const FHDIDChapterFile* Chapter = Catalog->FindChapter(ChapterId);
			FHDIDListItem Item;
			Item.Id = ChapterId;
			Item.Label = Chapter ? (ChapterId.Right(2) + TEXT("    ") + Chapter->Title) : ChapterId;
			Item.Detail = Catalog->IsPlayable(ChapterId) ? TEXT("OPEN") : TEXT("DATA ONLY");
			Items.Add(Item);
		}
		MakeList(TEXT("CHAPTERS"), TEXT("Only an open chapter can be played."), Items, false);
		return;
	}

	if (Screen == EHDIDScreen::EvidenceCategories || Screen == EHDIDScreen::NotebookCategories)
	{
		for (const TCHAR* Category : Categories)
		{
			FHDIDListItem Item;
			Item.Id = Category;
			Item.Label = Category;
			Items.Add(Item);
		}
		MakeList(Screen == EHDIDScreen::NotebookCategories ? TEXT("NOTEBOOK") : TEXT("EVIDENCE"), TEXT("Evidence is filed. Conclusions are not."), Items, false);
		return;
	}

	if (Screen == EHDIDScreen::EvidenceEntries || Screen == EHDIDScreen::NotebookEntries)
	{
		if (UHDIDInvestigationManager* Investigation = GetGameInstance()->GetSubsystem<UHDIDInvestigationManager>())
		{
			TArray<FHDIDNotebookEntry> Entries;
			Investigation->BuildNotebook(Entries);
			for (const FHDIDNotebookEntry& Entry : Entries)
			{
				if (Entry.Category != EntryCategory)
				{
					continue;
				}
				FHDIDListItem Item;
				Item.Id = Entry.SourceId;
				Item.Label = Entry.Title;
				Item.Detail = Entry.Body;
				Items.Add(Item);
			}
		}
		if (Items.Num() == 0)
		{
			FHDIDListItem Empty;
			Empty.Id = TEXT("empty");
			Empty.Label = TEXT("Nothing filed yet.");
			Empty.bEnabled = false;
			Items.Add(Empty);
		}
		MakeList(EntryCategory.ToUpper(), TEXT("Filed, not solved."), Items, false);
		return;
	}

	if (Screen == EHDIDScreen::Pause)
	{
		auto Add = [&Items](const TCHAR* Id, const TCHAR* Label)
		{
			FHDIDListItem Item;
			Item.Id = Id;
			Item.Label = Label;
			Items.Add(Item);
		};
		Add(TEXT("resume"), TEXT("RESUME"));
		Add(TEXT("notebook"), TEXT("NOTEBOOK"));
		Add(TEXT("settings"), TEXT("SETTINGS"));
		Add(TEXT("menu"), TEXT("MAIN MENU"));
		Add(TEXT("quit"), TEXT("QUIT"));
		MakeList(TEXT("PAUSED"), TEXT("The hour is waiting."), Items, true);
		return;
	}

	if (Screen == EHDIDScreen::ChapterEnd)
	{
		FHDIDListItem Remain;
		Remain.Id = TEXT("remain");
		Remain.Label = TEXT("REMAIN");
		Items.Add(Remain);
		FHDIDListItem Menu;
		Menu.Id = TEXT("menu");
		Menu.Label = TEXT("MAIN MENU");
		Items.Add(Menu);
		MakeList(TEXT("CHAPTER COMPLETE"), ChapterEndTitle + TEXT("\nThe hour has closed."), Items, true);
		return;
	}

	if (Screen == EHDIDScreen::Settings)
	{
		APlayerController* Controller = GetPlayer();
		if (!Controller)
		{
			return;
		}
		UHDIDSettingsWidget* Settings = CreateWidget<UHDIDSettingsWidget>(Controller, UHDIDSettingsWidget::StaticClass());
		Settings->Setup(this);
		if (Overlay)
		{
			Overlay->RemoveFromParent();
		}
		Overlay = Settings;
		Settings->AddToViewport(30);
		SyncInputMode();
		return;
	}

	if (Screen == EHDIDScreen::Timeline)
	{
		APlayerController* Controller = GetPlayer();
		if (!Controller)
		{
			return;
		}
		UHDIDTimelineWidget* Timeline = CreateWidget<UHDIDTimelineWidget>(Controller, UHDIDTimelineWidget::StaticClass());
		Timeline->Setup(this, TimelineFocus);
		if (Overlay)
		{
			Overlay->RemoveFromParent();
		}
		Overlay = Timeline;
		Timeline->AddToViewport(30);
		SyncInputMode();
	}
}
