#include "Data/HDIDCatalog.h"

#include "HowDidIDie.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
	template<typename T>
	bool LoadStructFile(TArray<FString>& Errors, const FString& Path, T& Out)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *Path))
		{
			Errors.Add(FString::Printf(TEXT("Missing file: %s"), *Path));
			return false;
		}
		if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Out, 0, 0))
		{
			Errors.Add(FString::Printf(TEXT("JSON did not match the game struct: %s"), *Path));
			return false;
		}
		return true;
	}
}

void UHDIDCatalogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadErrors.Reset();
	ChapterOrder.Reset();
	Chapters.Reset();
	Characters.Reset();
	Choices.Reset();

	const FString DataDir = FPaths::ProjectContentDir() / TEXT("Data");
	LoadStructFile(LoadErrors, DataDir / TEXT("Game.json"), GameText);
	LoadStructFile(LoadErrors, DataDir / TEXT("Music.json"), Music);
	LoadStructFile(LoadErrors, DataDir / TEXT("Locked/late_game.json"), LateGame);

	FHDIDCharacterList CharacterList;
	if (LoadStructFile(LoadErrors, DataDir / TEXT("Characters.json"), CharacterList))
	{
		for (const FHDIDCharacterDef& Character : CharacterList.Characters)
		{
			if (Characters.Contains(Character.Id))
			{
				LoadErrors.Add(FString::Printf(TEXT("Duplicate character id %s"), *Character.Id));
			}
			Characters.Add(Character.Id, Character);
		}
	}

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(DataDir / TEXT("Chapters/*.json")), true, false);
	Files.Sort();
	for (const FString& FileName : Files)
	{
		FHDIDChapterFile Chapter;
		const FString Path = DataDir / TEXT("Chapters") / FileName;
		if (!LoadStructFile(LoadErrors, Path, Chapter))
		{
			continue;
		}
		if (Chapters.Contains(Chapter.ChapterID))
		{
			LoadErrors.Add(FString::Printf(TEXT("Duplicate chapter id %s"), *Chapter.ChapterID));
			continue;
		}
		for (const FHDIDDialogueNode& Node : Chapter.Dialogue)
		{
			for (const FHDIDChoiceDef& Choice : Node.Choices)
			{
				if (Choices.Contains(Choice.ChoiceID))
				{
					LoadErrors.Add(FString::Printf(TEXT("Duplicate choice id %s"), *Choice.ChoiceID));
				}
				Choices.Add(Choice.ChoiceID, Choice);
			}
		}
		ChapterOrder.Add(Chapter.ChapterID);
		Chapters.Add(Chapter.ChapterID, Chapter);
	}

	bReady = LoadErrors.Num() == 0 && Chapters.Num() > 0;
	if (!bReady)
	{
		UE_LOG(LogHDID, Error, TEXT("Narrative catalog failed to load (%d errors)."), LoadErrors.Num());
		for (const FString& Error : LoadErrors)
		{
			UE_LOG(LogHDID, Error, TEXT("%s"), *Error);
		}
	}
	else
	{
		UE_LOG(LogHDID, Log, TEXT("Narrative catalog loaded %d chapters."), Chapters.Num());
	}
}

TArray<FString> UHDIDCatalogSubsystem::GetChapterIds() const
{
	return ChapterOrder;
}

const FHDIDChapterFile* UHDIDCatalogSubsystem::FindChapter(const FString& ChapterId) const
{
	return Chapters.Find(ChapterId);
}

bool UHDIDCatalogSubsystem::IsPlayable(const FString& ChapterId) const
{
	const FHDIDChapterFile* Chapter = FindChapter(ChapterId);
	return Chapter && Chapter->Playable;
}

FString UHDIDCatalogSubsystem::GetFirstPlayableChapterId() const
{
	for (const FString& Id : ChapterOrder)
	{
		if (IsPlayable(Id))
		{
			return Id;
		}
	}
	return FString();
}

const FHDIDCharacterDef* UHDIDCatalogSubsystem::FindCharacter(const FString& CharacterId) const
{
	return Characters.Find(CharacterId);
}

const FHDIDCardDef* UHDIDCatalogSubsystem::FindCard(const FString& CardId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDCardDef& Card : Pair.Value.Cards)
		{
			if (Card.CardID == CardId)
			{
				return &Card;
			}
		}
	}
	return nullptr;
}

const FHDIDClueDef* UHDIDCatalogSubsystem::FindClue(const FString& ClueId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDClueDef& Clue : Pair.Value.Clues)
		{
			if (Clue.ClueID == ClueId)
			{
				return &Clue;
			}
		}
	}
	return nullptr;
}

const FHDIDDialogueNode* UHDIDCatalogSubsystem::FindDialogue(const FString& DialogueId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDDialogueNode& Node : Pair.Value.Dialogue)
		{
			if (Node.DialogueID == DialogueId)
			{
				return &Node;
			}
		}
	}
	return nullptr;
}

const FHDIDChoiceDef* UHDIDCatalogSubsystem::FindChoice(const FString& ChoiceId) const
{
	return Choices.Find(ChoiceId);
}

const FHDIDConsequenceDef* UHDIDCatalogSubsystem::FindConsequence(const FString& ConsequenceId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDConsequenceDef& Consequence : Pair.Value.Consequences)
		{
			if (Consequence.Id == ConsequenceId)
			{
				return &Consequence;
			}
		}
	}
	return nullptr;
}

const FHDIDMemoryDef* UHDIDCatalogSubsystem::FindMemory(const FString& MemoryId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDMemoryDef& Memory : Pair.Value.Memories)
		{
			if (Memory.MemoryID == MemoryId)
			{
				return &Memory;
			}
		}
	}
	return nullptr;
}

const FHDIDTimelineEvent* UHDIDCatalogSubsystem::FindEvent(const FString& EventId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDTimelineEvent& Event : Pair.Value.Timeline)
		{
			if (Event.EventID == EventId)
			{
				return &Event;
			}
		}
	}
	return nullptr;
}

const FHDIDLocationDef* UHDIDCatalogSubsystem::FindLocation(const FString& LocationId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDLocationDef& Location : Pair.Value.Locations)
		{
			if (Location.LocationId == LocationId)
			{
				return &Location;
			}
		}
	}
	return nullptr;
}

int32 UHDIDCatalogSubsystem::GetCardIndex(const FString& CardId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (int32 Index = 0; Index < Pair.Value.Cards.Num(); ++Index)
		{
			if (Pair.Value.Cards[Index].CardID == CardId)
			{
				return Index;
			}
		}
	}
	return INDEX_NONE;
}

int32 UHDIDCatalogSubsystem::GetCardCount(const FString& ChapterId) const
{
	const FHDIDChapterFile* Chapter = FindChapter(ChapterId);
	return Chapter ? Chapter->Cards.Num() : 0;
}

FString UHDIDCatalogSubsystem::GetChapterForCard(const FString& CardId) const
{
	for (const TPair<FString, FHDIDChapterFile>& Pair : Chapters)
	{
		for (const FHDIDCardDef& Card : Pair.Value.Cards)
		{
			if (Card.CardID == CardId)
			{
				return Pair.Key;
			}
		}
	}
	return FString();
}
