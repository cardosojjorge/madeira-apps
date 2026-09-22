#include "Systems/HDIDManagers.h"

#include "AudioDevice.h"
#include "Core/HDIDGameMode.h"
#include "Core/HDIDPlayerCharacter.h"
#include "Core/HDIDPlayerController.h"
#include "Core/HDIDUtil.h"
#include "Data/HDIDCatalog.h"
#include "Engine/World.h"
#include "HowDidIDie.h"
#include "Save/HDIDSaveGame.h"
#include "TimerManager.h"
#include "UI/HDIDUIManager.h"
#include "World/HDIDGreyboxWorld.h"

namespace
{
	UHDIDSaveGame* StoryOf(const UObject* Object)
	{
		if (!Object || !Object->GetWorld() || !Object->GetWorld()->GetGameInstance())
		{
			return nullptr;
		}
		UHDIDSaveManager* Save = Object->GetWorld()->GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
		return Save ? Save->GetStory() : nullptr;
	}

	void Autosave(const UObject* Object)
	{
		if (!Object || !Object->GetWorld())
		{
			return;
		}
		UGameInstance* GI = Object->GetWorld()->GetGameInstance();
		if (!GI)
		{
			return;
		}
		UHDIDSaveManager* Save = GI->GetSubsystem<UHDIDSaveManager>();
		if (!Save)
		{
			return;
		}
		if (APlayerController* PC = Object->GetWorld()->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				Save->RememberTransform(Pawn->GetActorLocation(), PC->GetControlRotation());
			}
		}
		Save->WriteStory();
	}

	AHDIDGreyboxWorld* GreyboxOf(const UObject* Object)
	{
		if (!Object || !Object->GetWorld())
		{
			return nullptr;
		}
		if (AHDIDGameMode* Mode = Object->GetWorld()->GetAuthGameMode<AHDIDGameMode>())
		{
			return Mode->GetGreybox();
		}
		return nullptr;
	}

	AHDIDPlayerCharacter* CharacterOf(const UObject* Object)
	{
		if (!Object || !Object->GetWorld())
		{
			return nullptr;
		}
		if (APlayerController* PC = Object->GetWorld()->GetFirstPlayerController())
		{
			return Cast<AHDIDPlayerCharacter>(PC->GetPawn());
		}
		return nullptr;
	}
}

void UHDIDConsequenceManager::ApplyConsequence(const FHDIDConsequenceDef& Consequence, TMap<FString, int32>& Relationships)
{
	for (const TPair<FString, FString>& Flag : Consequence.SetFlags)
	{
		DerivedFlags.Add(Flag.Key, Flag.Value);
	}
	for (const TPair<FString, int32>& Change : Consequence.Relationship)
	{
		int32& Value = Relationships.FindOrAdd(Change.Key);
		Value += Change.Value;
	}
}

void UHDIDConsequenceManager::GetActiveConsequences(TArray<const FHDIDConsequenceDef*>& Out) const
{
	Out.Reset();
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (!Story || !Catalog)
	{
		return;
	}
	TArray<FString> Ids;
	Ids.Append(Story->OneShotChoices);
	for (const TPair<FString, FString>& Branch : Story->ChoiceByBranch)
	{
		Ids.Add(Branch.Value);
	}
	for (const FString& ChoiceId : Ids)
	{
		if (const FHDIDChoiceDef* Choice = Catalog->FindChoice(ChoiceId))
		{
			if (const FHDIDConsequenceDef* Consequence = Catalog->FindConsequence(Choice->ConsequenceID))
			{
				Out.Add(Consequence);
			}
		}
	}
}

void UHDIDConsequenceManager::Rebuild()
{
	DerivedFlags.Reset();
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (!Story || !Catalog)
	{
		return;
	}
	TMap<FString, int32> Relationships;
	for (const FString& DialogueId : Story->DialoguesCompleted)
	{
		if (const FHDIDDialogueNode* Node = Catalog->FindDialogue(DialogueId))
		{
			for (const TPair<FString, int32>& Change : Node->RelationshipChanges)
			{
				int32& Value = Relationships.FindOrAdd(Change.Key);
				Value += Change.Value;
			}
			for (const FString& ConsequenceId : Node->Consequences)
			{
				if (const FHDIDConsequenceDef* Consequence = Catalog->FindConsequence(ConsequenceId))
				{
					ApplyConsequence(*Consequence, Relationships);
				}
			}
		}
	}
	TArray<const FHDIDConsequenceDef*> Active;
	GetActiveConsequences(Active);
	TArray<FString> Reveals;
	for (const FHDIDConsequenceDef* Consequence : Active)
	{
		ApplyConsequence(*Consequence, Relationships);
		Reveals.Append(Consequence->RevealClues);
		for (const FString& EventId : Consequence->UnlockTimeline)
		{
			if (UHDIDTimelineManager* Timeline = GetGameInstance()->GetSubsystem<UHDIDTimelineManager>())
			{
				Timeline->UnlockEvent(EventId);
			}
		}
	}
	Story->Relationships = Relationships;
	if (UHDIDInvestigationManager* Investigation = GetGameInstance()->GetSubsystem<UHDIDInvestigationManager>())
	{
		for (const FString& ClueId : Reveals)
		{
			Investigation->DiscoverClue(ClueId);
		}
	}
}

TMap<FString, FString> UHDIDConsequenceManager::GetFlags() const
{
	TMap<FString, FString> Flags;
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Flags = Story->BaseFlags;
	}
	for (const TPair<FString, FString>& Flag : DerivedFlags)
	{
		Flags.Add(Flag.Key, Flag.Value);
	}
	return Flags;
}

bool UHDIDConsequenceManager::ConditionsMet(const TArray<FHDIDCondition>& Conditions) const
{
	return HDIDConditionsMet(Conditions, GetFlags());
}

bool UHDIDConsequenceManager::AnyConditionMet(const TArray<FHDIDCondition>& Conditions) const
{
	return HDIDAnyConditionMet(Conditions, GetFlags());
}

void UHDIDSoulEvaluationManager::Recompute()
{
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	UHDIDInvestigationManager* Investigation = GetGameInstance()->GetSubsystem<UHDIDInvestigationManager>();
	UHDIDConsequenceManager* Consequence = GetGameInstance()->GetSubsystem<UHDIDConsequenceManager>();
	if (!Story || !Catalog || !Investigation || !Consequence)
	{
		return;
	}
	FHDIDSoulValues Soul;
	FHDIDDedicationValues Dedication;
	Dedication.AlternateTimelines = Story->BranchRevisionCount;
	Dedication.FlashbacksDiscovered = Story->MemoriesFound.Num();
	Dedication.CrossChapterConnections = Story->CrossChapterDiscoveries.Num();
	for (const FString& ClueId : Story->CluesFound)
	{
		const FHDIDClueDef* Clue = Catalog->FindClue(ClueId);
		if (!Clue)
		{
			continue;
		}
		const bool bActive = Investigation->IsClueActive(ClueId);
		if (!bActive && Clue->BranchExclusive)
		{
			continue;
		}
		++Dedication.CluesFound;
		if (Clue->Optional)
		{
			++Dedication.OptionalCluesFound;
		}
		if (Clue->Hidden)
		{
			++Dedication.HiddenObjects;
		}
		Soul.Add(Clue->SoulOnDiscover);
	}
	TArray<const FHDIDConsequenceDef*> Active;
	Consequence->GetActiveConsequences(Active);
	for (const FHDIDConsequenceDef* Item : Active)
	{
		Soul.Add(Item->Soul);
		if (Item->HelpedNpc)
		{
			++Dedication.NPCsHelped;
		}
		if (Item->OptionalDialogue)
		{
			++Dedication.OptionalDialogue;
		}
	}
	for (const FString& DialogueId : Story->DialoguesCompleted)
	{
		if (const FHDIDDialogueNode* Node = Catalog->FindDialogue(DialogueId))
		{
			Soul.Add(Node->SoulChanges);
		}
	}
	Story->Soul = Soul;
	Story->Dedication = Dedication;
}

void UHDIDInvestigationManager::DiscoverClue(const FString& ClueId)
{
	if (ClueId.IsEmpty())
	{
		return;
	}
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDClueDef* Clue = Catalog ? Catalog->FindClue(ClueId) : nullptr;
	if (!Story || !Clue || Story->CluesFound.Contains(ClueId))
	{
		return;
	}
	Story->CluesFound.Add(ClueId);
	if (!Clue->GrantFlag.IsEmpty())
	{
		if (UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>())
		{
			Save->SetBaseFlag(Clue->GrantFlag, Clue->GrantValue);
		}
	}
	if (Clue->CrossChapter)
	{
		Story->CrossChapterDiscoveries.AddUnique(ClueId + TEXT("@") + Clue->CrossTarget);
	}
	for (const FString& Speaker : Clue->RelatedCharacters)
	{
		Story->SpeakersHeard.AddUnique(Speaker);
	}
	if (UHDIDTimelineManager* Timeline = GetGameInstance()->GetSubsystem<UHDIDTimelineManager>())
	{
		for (const FString& EventId : Clue->RelatedEvents)
		{
			Timeline->UnlockEvent(EventId);
		}
		Timeline->UnlockFromClue(ClueId);
	}
	if (UHDIDSoulEvaluationManager* Soul = GetGameInstance()->GetSubsystem<UHDIDSoulEvaluationManager>())
	{
		Soul->Recompute();
	}
	if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
	{
		Cards->NotifyDiscover(ClueId);
	}
	Autosave(this);
}

bool UHDIDInvestigationManager::IsClueDiscovered(const FString& ClueId) const
{
	UHDIDSaveGame* Story = StoryOf(this);
	return Story && Story->CluesFound.Contains(ClueId);
}

bool UHDIDInvestigationManager::IsClueActive(const FString& ClueId) const
{
	if (!IsClueDiscovered(ClueId))
	{
		return false;
	}
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDClueDef* Clue = Catalog ? Catalog->FindClue(ClueId) : nullptr;
	if (!Clue || !Clue->BranchExclusive)
	{
		return Clue != nullptr;
	}
	if (UHDIDConsequenceManager* Consequence = GetGameInstance()->GetSubsystem<UHDIDConsequenceManager>())
	{
		return Consequence->ConditionsMet(Clue->ActiveIf);
	}
	return false;
}

void UHDIDInvestigationManager::NotifySpeaker(const FString& SpeakerId)
{
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->SpeakersHeard.AddUnique(SpeakerId);
	}
}

void UHDIDInvestigationManager::NotifyLocation(const FString& LocationId)
{
	if (LocationId.IsEmpty())
	{
		return;
	}
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->LocationsEntered.AddUnique(LocationId);
	}
}

void UHDIDInvestigationManager::BuildNotebook(TArray<FHDIDNotebookEntry>& OutEntries) const
{
	OutEntries.Reset();
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>();
	if (!Story || !Catalog)
	{
		return;
	}
	auto Add = [&OutEntries](const FString& Category, const FString& Title, const FString& Body, const FString& Source)
	{
		FHDIDNotebookEntry Entry;
		Entry.Category = Category;
		Entry.Title = Title;
		Entry.Body = Body;
		Entry.SourceId = Source;
		OutEntries.Add(Entry);
	};

	TSet<FString> People;
	for (const FString& Speaker : Story->SpeakersHeard)
	{
		People.Add(Speaker);
	}
	for (const FString& ClueId : Story->CluesFound)
	{
		if (!IsClueActive(ClueId))
		{
			continue;
		}
		if (const FHDIDClueDef* Clue = Catalog->FindClue(ClueId))
		{
			for (const FString& Who : Clue->RelatedCharacters)
			{
				People.Add(Who);
			}
		}
	}
	for (const FString& PersonId : People)
	{
		if (const FHDIDCharacterDef* Character = Catalog->FindCharacter(PersonId))
		{
			FString Body = Character->NotebookIntro;
			for (const FString& ClueId : Story->CluesFound)
			{
				if (!IsClueActive(ClueId))
				{
					continue;
				}
				if (const FHDIDClueDef* Clue = Catalog->FindClue(ClueId))
				{
					if (Clue->RelatedCharacters.Contains(PersonId))
					{
						Body += TEXT("\nFiled beside: ") + Clue->Title;
					}
				}
			}
			Add(TEXT("People"), Character->DisplayName, Body, PersonId);
		}
	}

	for (const FString& LocationId : Story->LocationsEntered)
	{
		if (const FHDIDLocationDef* Location = Catalog->FindLocation(LocationId))
		{
			Add(TEXT("Locations"), Location->Name, Location->Description, LocationId);
		}
	}

	for (const FString& ClueId : Story->CluesFound)
	{
		if (!IsClueActive(ClueId))
		{
			continue;
		}
		const FHDIDClueDef* Clue = Catalog->FindClue(ClueId);
		if (!Clue)
		{
			continue;
		}
		const FString Body = Clue->Description + TEXT("\n") + Clue->DiscoveryMethod + TEXT(" · ") + Clue->Importance;
		Add(TEXT("Clues"), Clue->Title, Body, ClueId);
		if (Clue->Categories.Contains(TEXT("Objects")))
		{
			Add(TEXT("Objects"), Clue->Title, Clue->Description, ClueId);
		}
	}

	for (const FString& MemoryId : Story->MemoriesFound)
	{
		if (const FHDIDMemoryDef* Memory = Catalog->FindMemory(MemoryId))
		{
			Add(TEXT("Memories"), Memory->MemoryID, Memory->QuestionAnswered + TEXT("\n\n") + Memory->QuestionOpened, MemoryId);
		}
	}

	for (const FString& CardId : Story->CardsReached)
	{
		if (const FHDIDCardDef* Card = Catalog->FindCard(CardId))
		{
			FString Body = Card->Body;
			if (Cards)
			{
				Body = Cards->GetBody();
				if (Card->CardID != Story->CurrentCardId)
				{
					Body = Card->Body;
				}
			}
			Add(TEXT("Events"), Card->Time + TEXT("  ") + Card->Title, Body, CardId);
		}
	}

	if (const FHDIDChapterFile* Chapter = Catalog->FindChapter(Story->CurrentChapterId))
	{
		for (const FHDIDTimelineEvent& Event : Chapter->Timeline)
		{
			if (!Story->TimelineUnlocked.Contains(Event.EventID))
			{
				continue;
			}
			FString Body = Event.Title;
			if (Story->TimelineVisited.Contains(Event.EventID))
			{
				Body += TEXT("\nVisited.");
			}
			for (const FHDIDTimelineBranch& Branch : Event.Branches)
			{
				Body += TEXT("\n") + Branch.Label;
				if (const FString* Chosen = Story->ChoiceByBranch.Find(TEXT("BRANCH_MAYA")))
				{
					(void)Chosen;
				}
				if (const FHDIDChoiceDef* Choice = Catalog->FindChoice(Branch.ChoiceID))
				{
					if (!Choice->BranchGroup.IsEmpty())
					{
						if (const FString* Chosen = Story->ChoiceByBranch.Find(Choice->BranchGroup))
						{
							if (*Chosen == Branch.ChoiceID || Chosen->StartsWith(Branch.ChoiceID))
							{
								Body += TEXT("  — standing");
							}
						}
					}
				}
			}
			Add(TEXT("Timeline"), Event.Time, Body, Event.EventID);
			Add(TEXT("Events"), Event.Time + TEXT("  ") + Event.Title, Event.Title, Event.EventID);
		}
	}
	if (Story->BranchRevisionCount > 0)
	{
		Add(TEXT("Timeline"), TEXT("Revision"), TEXT("An earlier version of this hour was recorded. The room now matches the choice that stands."), TEXT("REVISION"));
	}

	for (const FString& ClueId : Story->CluesFound)
	{
		if (!IsClueActive(ClueId))
		{
			continue;
		}
		const FHDIDClueDef* Clue = Catalog->FindClue(ClueId);
		if (!Clue)
		{
			continue;
		}
		for (const FString& RelatedId : Clue->RelatedClues)
		{
			if (RelatedId <= ClueId || !IsClueActive(RelatedId))
			{
				continue;
			}
			const FHDIDClueDef* Related = Catalog->FindClue(RelatedId);
			if (!Related)
			{
				continue;
			}
			Add(TEXT("Connections"), TEXT("Filed together"), Clue->Title + TEXT("\n") + Related->Title, ClueId + TEXT("|") + RelatedId);
		}
	}
}

bool UHDIDChoiceManager::RequirementsMet(const FHDIDChoiceDef& Choice) const
{
	if (Choice.Requirements.Num() == 0)
	{
		return true;
	}
	if (UHDIDConsequenceManager* Consequence = GetGameInstance()->GetSubsystem<UHDIDConsequenceManager>())
	{
		return Consequence->ConditionsMet(Choice.Requirements);
	}
	return false;
}

bool UHDIDChoiceManager::Commit(const FString& ChoiceId, bool bAdvanceCard)
{
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	UHDIDSaveGame* Story = StoryOf(this);
	const FHDIDChoiceDef* Choice = Catalog ? Catalog->FindChoice(ChoiceId) : nullptr;
	if (!Choice || !Story || !RequirementsMet(*Choice))
	{
		return false;
	}
	if (!Choice->BranchGroup.IsEmpty())
	{
		const FString* Previous = Story->ChoiceByBranch.Find(Choice->BranchGroup);
		if (!Previous || *Previous != ChoiceId)
		{
			if (Previous && !Previous->IsEmpty())
			{
				++Story->BranchRevisionCount;
			}
			Story->ChoiceByBranch.Add(Choice->BranchGroup, ChoiceId);
		}
	}
	else
	{
		Story->OneShotChoices.AddUnique(ChoiceId);
	}
	if (UHDIDConsequenceManager* Consequence = GetGameInstance()->GetSubsystem<UHDIDConsequenceManager>())
	{
		Consequence->Rebuild();
	}
	if (UHDIDSoulEvaluationManager* Soul = GetGameInstance()->GetSubsystem<UHDIDSoulEvaluationManager>())
	{
		Soul->Recompute();
	}
	if (AHDIDGreyboxWorld* Greybox = GreyboxOf(this))
	{
		Greybox->RefreshInteractables();
	}
	if (bAdvanceCard)
	{
		if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
		{
			Cards->NotifyCommit();
		}
	}
	Autosave(this);
	return true;
}

void UHDIDDialogueManager::StartDialogue(const FString& DialogueId)
{
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDDialogueNode* Node = Catalog ? Catalog->FindDialogue(DialogueId) : nullptr;
	if (!Node)
	{
		return;
	}
	if (Node->Reveal || !Catalog->IsRevealImplemented())
	{
		const FString Lower = Node->Text.ToLower();
		if (Node->Reveal || Lower.Contains(TEXT("grim reaper")) || Lower.Contains(TEXT("the void")) || Lower.Contains(TEXT("soul 11")))
		{
			UE_LOG(LogHDID, Warning, TEXT("Locked reveal dialogue refused: %s"), *DialogueId);
			if (Node->Reveal || Lower.Contains(TEXT("grim reaper")) || Lower.Contains(TEXT("the void")))
			{
				return;
			}
		}
	}
	OpenNode(*Node);
}

void UHDIDDialogueManager::OpenNode(const FHDIDDialogueNode& Node)
{
	UHDIDSaveGame* Story = StoryOf(this);
	if (!Story)
	{
		return;
	}
	bOpen = true;
	CurrentDialogueId = Node.DialogueID;
	const bool bFirst = !Story->DialoguesCompleted.Contains(Node.DialogueID);
	Story->DialoguesCompleted.AddUnique(Node.DialogueID);
	if (!Node.Speaker.IsEmpty())
	{
		Story->SpeakersHeard.AddUnique(Node.Speaker);
	}
	if (bFirst)
	{
		if (UHDIDInvestigationManager* Investigation = GetGameInstance()->GetSubsystem<UHDIDInvestigationManager>())
		{
			for (const FString& ClueId : Node.ClueUnlocks)
			{
				Investigation->DiscoverClue(ClueId);
			}
		}
		if (UHDIDConsequenceManager* Consequence = GetGameInstance()->GetSubsystem<UHDIDConsequenceManager>())
		{
			Consequence->Rebuild();
		}
		if (UHDIDSoulEvaluationManager* Soul = GetGameInstance()->GetSubsystem<UHDIDSoulEvaluationManager>())
		{
			Soul->Recompute();
		}
	}
	VisibleChoices.Reset();
	if (UHDIDChoiceManager* Choices = GetGameInstance()->GetSubsystem<UHDIDChoiceManager>())
	{
		for (const FHDIDChoiceDef& Choice : Node.Choices)
		{
			if (Choices->RequirementsMet(Choice))
			{
				VisibleChoices.Add(Choice);
			}
		}
	}
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ShowDialogue(Node, VisibleChoices);
		if (UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>())
		{
			const FHDIDCharacterDef* Speaker = Catalog->FindCharacter(Node.Speaker);
			UI->SetSubtitle(Speaker ? Speaker->DisplayName : Node.Speaker, Node.Text);
		}
	}
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		if (!Node.Camera.IsEmpty())
		{
			Character->SetCameraMode(Node.Camera);
		}
	}
	if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
	{
		Cards->NotifyDialogue(Node.DialogueID);
	}
}

void UHDIDDialogueManager::Choose(int32 Index)
{
	if (!VisibleChoices.IsValidIndex(Index))
	{
		return;
	}
	const FHDIDChoiceDef Choice = VisibleChoices[Index];
	if (UHDIDChoiceManager* Choices = GetGameInstance()->GetSubsystem<UHDIDChoiceManager>())
	{
		Choices->Commit(Choice.ChoiceID, false);
	}
	if (!Choice.NextNode.IsEmpty())
	{
		StartDialogue(Choice.NextNode);
	}
	else
	{
		Close();
		if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
		{
			Cards->NotifyCommit();
		}
	}
}

void UHDIDDialogueManager::Advance()
{
	if (!bOpen)
	{
		return;
	}
	if (VisibleChoices.Num() > 0)
	{
		return;
	}
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDDialogueNode* Node = Catalog ? Catalog->FindDialogue(CurrentDialogueId) : nullptr;
	if (Node && !Node->NextNode.IsEmpty())
	{
		StartDialogue(Node->NextNode);
	}
	else
	{
		Close();
	}
}

void UHDIDDialogueManager::Close()
{
	bOpen = false;
	VisibleChoices.Reset();
	CurrentDialogueId.Reset();
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->CloseTop();
		UI->SetSubtitle(FString(), FString());
	}
	if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
	{
		if (AHDIDPlayerCharacter* Character = CharacterOf(this))
		{
			Character->SetCameraMode(Cards->GetCameraMode());
		}
	}
}

const FHDIDCardDef* UHDIDStoryCardManager::GetCurrentCard() const
{
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (!Story || !Catalog)
	{
		return nullptr;
	}
	return Catalog->FindCard(Story->CurrentCardId);
}

FString UHDIDStoryCardManager::GetIndexLabel() const
{
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (!Story || !Catalog)
	{
		return FString();
	}
	const int32 Index = Catalog->GetCardIndex(Story->CurrentCardId);
	const int32 Count = Catalog->GetCardCount(Story->CurrentChapterId);
	if (Index == INDEX_NONE)
	{
		return FString();
	}
	return FString::Printf(TEXT("CARD %02d / %02d"), Index + 1, Count);
}

FString UHDIDStoryCardManager::GetTitle() const
{
	const FHDIDCardDef* Card = GetCurrentCard();
	return Card ? Card->Title : FString();
}

FString UHDIDStoryCardManager::GetBody() const
{
	const FHDIDCardDef* Card = GetCurrentCard();
	if (!Card)
	{
		return FString();
	}
	if (UHDIDConsequenceManager* Consequence = GetGameInstance()->GetSubsystem<UHDIDConsequenceManager>())
	{
		for (const FHDIDCardVariant& Variant : Card->Variants)
		{
			if (Consequence->ConditionsMet(Variant.IfAll))
			{
				return Variant.Body;
			}
		}
	}
	return Card->Body;
}

FString UHDIDStoryCardManager::GetCameraMode() const
{
	const FHDIDCardDef* Card = GetCurrentCard();
	return Card ? Card->Camera : TEXT("OverShoulder");
}

bool UHDIDStoryCardManager::IsCardReached(const FString& CardId) const
{
	if (CardId.IsEmpty())
	{
		return true;
	}
	UHDIDSaveGame* Story = StoryOf(this);
	return Story && Story->CardsReached.Contains(CardId);
}

bool UHDIDStoryCardManager::IsObjectiveMet(const FString& Objective) const
{
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDInvestigationManager* Investigation = GetGameInstance()->GetSubsystem<UHDIDInvestigationManager>();
	if (!Story)
	{
		return false;
	}
	TArray<FString> Parts;
	Objective.ParseIntoArray(Parts, TEXT("|"), true);
	for (const FString& Part : Parts)
	{
		FString Key;
		FString Value;
		if (!Part.Split(TEXT(":"), &Key, &Value))
		{
			continue;
		}
		if (Key == TEXT("discover") && Investigation && Investigation->IsClueActive(Value))
		{
			return true;
		}
		if (Key == TEXT("inspect") && Story->InspectedIds.Contains(Value))
		{
			return true;
		}
		if (Key == TEXT("dialogue") && Story->DialoguesCompleted.Contains(Value))
		{
			return true;
		}
		if (Key == TEXT("commit") && Story->ChoiceByBranch.Contains(Value))
		{
			return true;
		}
		if (Key == TEXT("flashback") && Story->MemoriesFound.Contains(Value))
		{
			return true;
		}
		if (Key == TEXT("open") && Value == TEXT("TIMELINE") && Story->bTimelineOpened)
		{
			return true;
		}
		if (Key == TEXT("enter") && Story->LocationsEntered.Contains(Value))
		{
			return true;
		}
	}
	return false;
}

bool UHDIDStoryCardManager::AreObjectivesMet(const FHDIDCardDef& Card) const
{
	for (const FString& Objective : Card.Objectives)
	{
		if (!IsObjectiveMet(Objective))
		{
			return false;
		}
	}
	return true;
}

void UHDIDStoryCardManager::Publish() const
{
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ShowCard(GetIndexLabel(), GetTitle(), GetBody());
	}
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		Character->SetCameraMode(GetCameraMode());
	}
	if (UHDIDAudioManager* Audio = GetGameInstance()->GetSubsystem<UHDIDAudioManager>())
	{
		Audio->ApplyCard(GetCurrentCard());
	}
}

void UHDIDStoryCardManager::CompleteChapter()
{
	if (bCompletionAnnounced)
	{
		return;
	}
	bCompletionAnnounced = true;
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->bChapterComplete = true;
		Story->UnlockedContent.AddUnique(Story->CurrentChapterId + TEXT("_COMPLETE"));
		if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
		{
			UI->ShowChapterComplete(GetTitle());
		}
	}
	Autosave(this);
}

void UHDIDStoryCardManager::TryAdvance()
{
	for (int32 Guard = 0; Guard < 16; ++Guard)
	{
		const FHDIDCardDef* Card = GetCurrentCard();
		UHDIDSaveGame* Story = StoryOf(this);
		if (!Card || !Story)
		{
			return;
		}
		if (!AreObjectivesMet(*Card))
		{
			Publish();
			return;
		}
		if (Card->NextCards.Num() == 0)
		{
			Publish();
			CompleteChapter();
			return;
		}
		Story->CurrentCardId = Card->NextCards[0];
		Story->CardsReached.AddUnique(Story->CurrentCardId);
	}
	Publish();
}

void UHDIDStoryCardManager::BeginSession()
{
	bCompletionAnnounced = false;
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->CardsReached.AddUnique(Story->CurrentCardId);
	}
	TryAdvance();
	Autosave(this);
}

void UHDIDStoryCardManager::NotifyInspect(const FString& InteractableId)
{
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->InspectedIds.AddUnique(InteractableId);
	}
	TryAdvance();
}

void UHDIDStoryCardManager::NotifyDiscover(const FString& ClueId)
{
	(void)ClueId;
	TryAdvance();
}

void UHDIDStoryCardManager::NotifyDialogue(const FString& DialogueId)
{
	(void)DialogueId;
	TryAdvance();
}

void UHDIDStoryCardManager::NotifyCommit()
{
	TryAdvance();
}

void UHDIDStoryCardManager::NotifyFlashback(const FString& MemoryId)
{
	(void)MemoryId;
	TryAdvance();
}

void UHDIDStoryCardManager::NotifyTimelineOpened()
{
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->bTimelineOpened = true;
	}
	TryAdvance();
}

void UHDIDStoryCardManager::NotifyEnterLocation(const FString& LocationId)
{
	(void)LocationId;
	TryAdvance();
}

void UHDIDTimelineManager::SyncFromSave()
{
}

void UHDIDTimelineManager::UnlockEvent(const FString& EventId)
{
	if (EventId.IsEmpty())
	{
		return;
	}
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->TimelineUnlocked.AddUnique(EventId);
	}
}

void UHDIDTimelineManager::UnlockFromClue(const FString& ClueId)
{
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (!Story || !Catalog)
	{
		return;
	}
	if (const FHDIDChapterFile* Chapter = Catalog->FindChapter(Story->CurrentChapterId))
	{
		for (const FHDIDTimelineEvent& Event : Chapter->Timeline)
		{
			if (Event.UnlockOnClue == ClueId)
			{
				UnlockEvent(Event.EventID);
			}
		}
	}
}

bool UHDIDTimelineManager::IsUnlocked(const FString& EventId) const
{
	UHDIDSaveGame* Story = StoryOf(this);
	return Story && Story->TimelineUnlocked.Contains(EventId);
}

bool UHDIDTimelineManager::TravelTo(const FString& EventId)
{
	if (!IsUnlocked(EventId))
	{
		if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
		{
			UI->ShowNotice(TEXT("This hour is not open."));
		}
		return false;
	}
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDTimelineEvent* Event = Catalog ? Catalog->FindEvent(EventId) : nullptr;
	UHDIDSaveGame* Story = StoryOf(this);
	if (!Event || !Story)
	{
		return false;
	}
	Story->TimelineVisited.AddUnique(EventId);
	if (UHDIDFlashbackManager* Flashback = GetGameInstance()->GetSubsystem<UHDIDFlashbackManager>())
	{
		if (!Flashback->IsActive())
		{
			if (AHDIDPlayerCharacter* Character = CharacterOf(this))
			{
				Character->SetActorLocation(Event->Position.ToUE());
			}
		}
	}
	Autosave(this);
	return true;
}

FString UHDIDTimelineManager::GetPrimaryBranchEventId() const
{
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (!Story || !Catalog)
	{
		return FString();
	}
	if (const FHDIDChapterFile* Chapter = Catalog->FindChapter(Story->CurrentChapterId))
	{
		for (const FHDIDTimelineEvent& Event : Chapter->Timeline)
		{
			if (Event.Branches.Num() > 0 && Story->TimelineUnlocked.Contains(Event.EventID))
			{
				return Event.EventID;
			}
		}
	}
	return FString();
}

void UHDIDFlashbackManager::BeginMemory(const FString& MemoryId, AActor* ZoomTarget)
{
	if (bActive || bBusy || MemoryId.IsEmpty())
	{
		return;
	}
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDMemoryDef* Memory = Catalog ? Catalog->FindMemory(MemoryId) : nullptr;
	if (!Memory)
	{
		return;
	}
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		SavedTransform = Character->GetActorTransform();
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			SavedControl = PC->GetControlRotation();
		}
	}
	bBusy = true;
	if (UHDIDCinematicManager* Cinematic = GetGameInstance()->GetSubsystem<UHDIDCinematicManager>())
	{
		FHDIDSimpleEvent Done;
		Done.BindLambda([this, MemoryId]()
		{
			FinishEnter(MemoryId);
		});
		Cinematic->EnterMemory(ZoomTarget, MoveTemp(Done));
	}
	else
	{
		FinishEnter(MemoryId);
	}
}

void UHDIDFlashbackManager::FinishEnter(const FString& MemoryId)
{
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDMemoryDef* Memory = Catalog ? Catalog->FindMemory(MemoryId) : nullptr;
	UHDIDSaveGame* Story = StoryOf(this);
	bBusy = false;
	if (!Memory || !Story)
	{
		return;
	}
	bActive = true;
	ActiveMemoryId = MemoryId;
	Story->ActiveMemoryId = MemoryId;
	Story->MemoriesFound.AddUnique(MemoryId);
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		Character->SetActorLocation(Memory->EntryPosition.ToUE());
		Character->SetSaturation(FMath::Max(0.22f, Character->GetActorLocation().Z * 0.f + 0.22f));
	}
	if (UHDIDTimelineManager* Timeline = GetGameInstance()->GetSubsystem<UHDIDTimelineManager>())
	{
		Timeline->UnlockEvent(Memory->TimelineEventID);
		Timeline->TravelTo(Memory->TimelineEventID);
	}
	if (AHDIDGreyboxWorld* Greybox = GreyboxOf(this))
	{
		Greybox->RefreshInteractables();
	}
	if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
	{
		Cards->NotifyFlashback(MemoryId);
	}
	if (UHDIDSoulEvaluationManager* Soul = GetGameInstance()->GetSubsystem<UHDIDSoulEvaluationManager>())
	{
		Soul->Recompute();
	}
	Autosave(this);
}

void UHDIDFlashbackManager::TryReturn()
{
	if (!bActive || bBusy)
	{
		return;
	}
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDMemoryDef* Memory = Catalog ? Catalog->FindMemory(ActiveMemoryId) : nullptr;
	UHDIDInvestigationManager* Investigation = GetGameInstance()->GetSubsystem<UHDIDInvestigationManager>();
	if (Memory && !Memory->RequiredClue.IsEmpty() && Investigation && !Investigation->IsClueDiscovered(Memory->RequiredClue))
	{
		if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
		{
			UI->ShowNotice(Memory->HoldLine);
		}
		return;
	}
	bBusy = true;
	if (UHDIDCinematicManager* Cinematic = GetGameInstance()->GetSubsystem<UHDIDCinematicManager>())
	{
		FHDIDSimpleEvent Done;
		Done.BindLambda([this]()
		{
			FinishExit();
		});
		Cinematic->ExitMemory(MoveTemp(Done));
	}
	else
	{
		FinishExit();
	}
}

void UHDIDFlashbackManager::FinishExit()
{
	bBusy = false;
	bActive = false;
	ActiveMemoryId.Reset();
	if (UHDIDSaveGame* Story = StoryOf(this))
	{
		Story->ActiveMemoryId.Reset();
	}
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		Character->SetActorTransform(SavedTransform);
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->SetControlRotation(SavedControl);
		}
		Character->ApplyChapterLook();
	}
	if (AHDIDGreyboxWorld* Greybox = GreyboxOf(this))
	{
		Greybox->RefreshInteractables();
	}
	Autosave(this);
}

void UHDIDFlashbackManager::RestoreFromSave()
{
	UHDIDSaveGame* Story = StoryOf(this);
	if (!Story || Story->ActiveMemoryId.IsEmpty())
	{
		bActive = false;
		ActiveMemoryId.Reset();
		return;
	}
	bActive = true;
	ActiveMemoryId = Story->ActiveMemoryId;
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		Character->SetSaturation(0.22f);
	}
}

void UHDIDAudioManager::ApplySettings()
{
	const UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
	if (!Save || !Save->GetSettings() || !GetWorld())
	{
		return;
	}
	if (FAudioDevice* Device = GetWorld()->GetAudioDeviceRaw())
	{
		Device->SetTransientPrimaryVolume(FMath::Clamp(Save->GetSettings()->MasterVolume, 0.f, 1.f));
	}
}

void UHDIDAudioManager::ApplyCard(const FHDIDCardDef* Card)
{
	ActiveLayers.Reset();
	if (!Card)
	{
		return;
	}
	ActiveLayers = Card->Audio;
	UHDIDSaveGame* Story = StoryOf(this);
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (Story && Catalog)
	{
		if (const FHDIDChapterFile* Chapter = Catalog->FindChapter(Story->CurrentChapterId))
		{
			for (const FString& Layer : Chapter->MusicLayers)
			{
				ActiveLayers.AddUnique(Layer);
			}
		}
	}
}

void UHDIDAudioManager::Pulse(const FString& Layer)
{
	(void)LayerGain(Layer);
}

void UHDIDAudioManager::PlayReverseMoment()
{
	ReverseAlpha = 0.15f;
	FTimerHandle Handle;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			ReverseAlpha = 1.f;
		}), 0.45f, false);
	}
}

float UHDIDAudioManager::LayerGain(const FString& Layer) const
{
	const UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
	const float Master = Save && Save->GetSettings() ? Save->GetSettings()->MasterVolume : 1.f;
	float Category = 1.f;
	if (Save && Save->GetSettings())
	{
		if (Layer == TEXT("piano") || Layer == TEXT("jazz") || Layer == TEXT("strings"))
		{
			Category = Save->GetSettings()->MusicVolume;
		}
		else if (Layer == TEXT("whispers"))
		{
			Category = Save->GetSettings()->DialogueVolume;
		}
		else
		{
			Category = Save->GetSettings()->EffectsVolume;
		}
	}
	const bool bActive = ActiveLayers.Contains(Layer) || Layer == TEXT("footsteps");
	return bActive ? Master * Category * ReverseAlpha : 0.f;
}

void UHDIDCinematicManager::EnterMemory(AActor* InZoomTarget, FHDIDSimpleEvent Finished)
{
	bBusy = true;
	bEntering = true;
	Stage = 0;
	Pending = MoveTemp(Finished);
	ZoomTarget = InZoomTarget;
	if (UHDIDAudioManager* Audio = GetGameInstance()->GetSubsystem<UHDIDAudioManager>())
	{
		Audio->PlayReverseMoment();
	}
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		Character->SetGameplayBlocked(true);
		Character->SetSaturation(0.f);
	}
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->SetLetterbox(1.f);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(StepTimer, this, &UHDIDCinematicManager::Step, 0.35f, false);
	}
}

void UHDIDCinematicManager::ExitMemory(FHDIDSimpleEvent Finished)
{
	bBusy = true;
	bEntering = false;
	Stage = 0;
	Pending = MoveTemp(Finished);
	if (UHDIDAudioManager* Audio = GetGameInstance()->GetSubsystem<UHDIDAudioManager>())
	{
		Audio->PlayReverseMoment();
	}
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		Character->SetGameplayBlocked(true);
		Character->SetSaturation(0.f);
	}
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->SetLetterbox(1.f);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(StepTimer, this, &UHDIDCinematicManager::Step, 0.3f, false);
	}
}

void UHDIDCinematicManager::Step()
{
	if (bEntering && Stage == 0)
	{
		if (AHDIDPlayerCharacter* Character = CharacterOf(this))
		{
			Character->SetArmLength(90.f);
			if (ZoomTarget.IsValid())
			{
				const FVector Look = ZoomTarget->GetActorLocation();
				if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
				{
					PC->SetControlRotation((Look - Character->GetActorLocation()).Rotation());
				}
			}
		}
		++Stage;
		if (UWorld* World = GetWorld())
		{
			const UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
			const float Delay = (Save && Save->GetSettings() && Save->GetSettings()->bMotionReduction) ? 0.05f : 0.4f;
			World->GetTimerManager().SetTimer(StepTimer, this, &UHDIDCinematicManager::Step, Delay, false);
		}
		return;
	}
	Finish();
}

void UHDIDCinematicManager::Finish()
{
	bBusy = false;
	if (AHDIDPlayerCharacter* Character = CharacterOf(this))
	{
		Character->SetGameplayBlocked(false);
	}
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->SetLetterbox(0.f);
	}
	FHDIDSimpleEvent Done = MoveTemp(Pending);
	Done.ExecuteIfBound();
}
