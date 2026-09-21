#include "Data/HDIDNarrativeValidator.h"

#include "Core/HDIDUtil.h"
#include "Data/HDIDTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/IConsoleManager.h"
#include "HowDidIDie.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Save/HDIDSaveGame.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <initializer_list>

namespace
{
	struct FProtagonist
	{
		const TCHAR* Id;
		const TCHAR* Name;
		int32 Age;
		const TCHAR* Job;
		const TCHAR* Theme;
		const TCHAR* Chapter;
	};

	const FProtagonist Protagonists[] = {
		{ TEXT("CH_ADRIAN"), TEXT("Adrian Vale"), 42, TEXT("CEO"), TEXT("Ambition"), TEXT("CH01") },
		{ TEXT("CH_MAYA"), TEXT("Maya"), 21, TEXT("Student"), TEXT("Trust"), TEXT("CH02") },
		{ TEXT("CH_DANIEL"), TEXT("Daniel Cross"), 51, TEXT("Detective"), TEXT("Truth"), TEXT("CH03") },
		{ TEXT("CH_ELENA"), TEXT("Elena Moretti"), 39, TEXT("Doctor"), TEXT("Responsibility"), TEXT("CH04") },
		{ TEXT("CH_LUCAS"), TEXT("Lucas"), 34, TEXT("Photographer / Traveler"), TEXT("Escape"), TEXT("CH05") },
		{ TEXT("CH_MARCO"), TEXT("Marco"), 47, TEXT("Chef"), TEXT("Ambition / Sacrifice"), TEXT("CH06") },
		{ TEXT("CH_SOFIA"), TEXT("Sofia"), 27, TEXT("Athlete"), TEXT("Sacrifice"), TEXT("CH07") },
		{ TEXT("CH_NINA"), TEXT("Nina"), 29, TEXT("Influencer"), TEXT("Identity"), TEXT("CH08") },
		{ TEXT("CH_ELIAS"), TEXT("Elias Hart"), 56, TEXT("Scientist"), TEXT("Control"), TEXT("CH09") },
		{ TEXT("CH_CLARA"), TEXT("Clara"), 63, TEXT("Artist"), TEXT("Memory"), TEXT("CH10") },
	};

	const TCHAR* CoinBeats[] = {
		TEXT("Adrian discovers the coin."),
		TEXT("Maya possesses the coin."),
		TEXT("Daniel identifies the coin."),
		TEXT("Elena sees the coin in an old photograph."),
		TEXT("Lucas photographs the coin."),
		TEXT("Marco receives the coin."),
		TEXT("Sofia finds the coin."),
		TEXT("Nina sees the coin in an old recording."),
		TEXT("Elias investigates the coin."),
		TEXT("Clara paints the coin."),
	};

	const TCHAR* HourglassBeats[] = {
		TEXT("An hourglass in the boardroom is nearly empty, and still only atmospheric."),
		TEXT("A cheap hourglass timer sits on Maya's desk, borrowed from a lab class."),
		TEXT("The hourglass appears in a crime-scene photograph Daniel cannot date."),
		TEXT("A patient's hourglass was left in Elena's office after visiting hours."),
		TEXT("The station clock is shaped like an hourglass; Lucas frames it and walks away."),
		TEXT("Marco's kitchen timer is an hourglass he never turns."),
		TEXT("The stadium clock is painted as an hourglass over the final lap."),
		TEXT("Nina's stream overlay is an hourglass she does not remember adding."),
		TEXT("Elias draws the hourglass as a function and the labels fall off the axes."),
		TEXT("Clara paints the hourglass behind the coin, sand in both bulbs at once."),
	};

	const TCHAR* StrangerStages[] = {
		TEXT("Silhouette"), TEXT("Hands"), TEXT("Voice"), TEXT("PartialFace"), TEXT("Photograph"),
		TEXT("HistoricalReference"), TEXT("DirectInteraction"), TEXT("Ageless"), TEXT("IdentityNearlyConfirmed"), TEXT("FullRevealLocked"),
	};

	const float Saturations[] = { 0.10f, 0.14f, 0.18f, 0.22f, 0.26f, 0.30f, 0.34f, 0.38f, 0.44f, 0.50f };

	bool IsOneOf(const FString& Value, std::initializer_list<const TCHAR*> Options)
	{
		for (const TCHAR* Option : Options)
		{
			if (Value == Option)
			{
				return true;
			}
		}
		return false;
	}

	FString Field(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
	{
		FString Value;
		if (Object.IsValid())
		{
			Object->TryGetStringField(Key, Value);
		}
		return Value;
	}

	bool Has(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
	{
		return Object.IsValid() && Object->Values.Contains(Key);
	}

	TSharedPtr<FJsonObject> LoadObject(const FString& Path, TArray<FString>& Errors)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			Errors.Add(FString::Printf(TEXT("Missing file %s"), *Path));
			return nullptr;
		}
		TSharedPtr<FJsonObject> Object;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Object) || !Object.IsValid())
		{
			Errors.Add(FString::Printf(TEXT("Invalid JSON %s"), *Path));
			return nullptr;
		}
		return Object;
	}

	FString FlagSignature(const TSharedPtr<FJsonObject>& Flags)
	{
		TArray<FString> Parts;
		if (Flags.IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Flags->Values)
			{
				Parts.Add(Pair.Key + TEXT("=") + Pair.Value->AsString());
			}
		}
		Parts.Sort();
		return FString::Join(Parts, TEXT("|"));
	}

	FString SoulSignature(const TSharedPtr<FJsonObject>& Soul)
	{
		TArray<FString> Parts;
		if (Soul.IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Soul->Values)
			{
				Parts.Add(Pair.Key + TEXT("=") + FString::FromInt(FMath::RoundToInt(Pair.Value->AsNumber())));
			}
		}
		Parts.Sort();
		return FString::Join(Parts, TEXT("|"));
	}

	FString ListSignature(const TArray<TSharedPtr<FJsonValue>>* Values)
	{
		TArray<FString> Parts;
		if (Values)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				Parts.Add(Value->AsString());
			}
		}
		Parts.Sort();
		return FString::Join(Parts, TEXT("|"));
	}

	bool FlagsMentioned(const TArray<TSharedPtr<FJsonValue>>* Conditions, const TSet<FString>& Flags)
	{
		if (!Conditions)
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Conditions)
		{
			const TSharedPtr<FJsonObject> Condition = Value->AsObject();
			if (Condition.IsValid() && Flags.Contains(Field(Condition, TEXT("Flag"))))
			{
				return true;
			}
		}
		return false;
	}

	void RequireProperty(UStruct* Struct, const TCHAR* Name, TArray<FString>& Errors)
	{
		if (!Struct || !Struct->FindPropertyByName(Name))
		{
			Errors.Add(FString::Printf(TEXT("Save field missing: %s"), Name));
		}
	}
}

FHDIDValidationReport FHDIDNarrativeValidator::ValidateProject()
{
	FHDIDValidationReport Report;
	TArray<FString>& Errors = Report.Errors;
	const FString DataDir = FPaths::ProjectContentDir() / TEXT("Data");

	const TSharedPtr<FJsonObject> CharactersFile = LoadObject(DataDir / TEXT("Characters.json"), Errors);
	TMap<FString, TSharedPtr<FJsonObject>> Characters;
	if (CharactersFile.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (!CharactersFile->TryGetArrayField(TEXT("Characters"), List) || !List)
		{
			Errors.Add(TEXT("Characters.json missing Characters."));
		}
		else
		{
			for (const TSharedPtr<FJsonValue>& Value : *List)
			{
				const TSharedPtr<FJsonObject> Character = Value->AsObject();
				const FString Id = Field(Character, TEXT("Id"));
				if (Characters.Contains(Id))
				{
					Errors.Add(TEXT("Duplicate character id."));
				}
				Characters.Add(Id, Character);
			}
		}
	}
	for (const FProtagonist& Protagonist : Protagonists)
	{
		const TSharedPtr<FJsonObject>* Found = Characters.Find(Protagonist.Id);
		if (!Found || !Found->IsValid())
		{
			Errors.Add(FString::Printf(TEXT("Missing protagonist %s"), Protagonist.Id));
			continue;
		}
		const TSharedPtr<FJsonObject>& Character = *Found;
		double Age = 0.0;
		Character->TryGetNumberField(TEXT("Age"), Age);
		if (Field(Character, TEXT("DisplayName")) != Protagonist.Name || FMath::RoundToInt(Age) != Protagonist.Age
			|| Field(Character, TEXT("Profession")) != Protagonist.Job || Field(Character, TEXT("Theme")) != Protagonist.Theme)
		{
			Errors.Add(FString::Printf(TEXT("Character drift: %s"), Protagonist.Id));
		}
		if (Field(Character, TEXT("NotebookIntro")).IsEmpty() || Field(Character, TEXT("VoiceNote")).IsEmpty())
		{
			Errors.Add(FString::Printf(TEXT("Character %s missing notebook intro or voice."), Protagonist.Id));
		}
	}
	if (const TSharedPtr<FJsonObject>* Stranger = Characters.Find(TEXT("CH_STRANGER")))
	{
		if (Field(*Stranger, TEXT("DisplayName")) != TEXT("Stranger"))
		{
			Errors.Add(TEXT("The stranger's public name must stay Stranger."));
		}
		if (Field(*Stranger, TEXT("DisplayName")).ToLower().Contains(TEXT("grim reaper")))
		{
			Errors.Add(TEXT("Stranger display name reveals the identity."));
		}
	}
	else
	{
		Errors.Add(TEXT("The stranger's public name must stay Stranger."));
	}

	const TSharedPtr<FJsonObject> Late = LoadObject(DataDir / TEXT("Locked/late_game.json"), Errors);
	if (Late.IsValid())
	{
		bool bPlayable = false;
		Late->TryGetBoolField(TEXT("Playable"), bPlayable);
		if (bPlayable)
		{
			Errors.Add(TEXT("Late game must not be playable in this slice."));
		}
		bool bVillain = false;
		Late->TryGetBoolField(TEXT("GrimReaperIsVillain"), bVillain);
		if (bVillain || !Has(Late, TEXT("GrimReaperIsVillain")))
		{
			if (bVillain)
			{
				Errors.Add(TEXT("The Grim Reaper is not the villain."));
			}
		}
		if (Field(Late, TEXT("GrimReaperIdentity")) != TEXT("The Grim Reaper"))
		{
			Errors.Add(TEXT("Locked identity missing."));
		}
		const TCHAR* Lines[] = {
			TEXT("You thought you were solving their deaths."),
			TEXT("You were learning who they were."),
			TEXT("And I was learning who you are."),
			TEXT("I don't decide who deserves another life."),
			TEXT("You showed me."),
			TEXT("There is one soul left."),
		};
		const TArray<TSharedPtr<FJsonValue>>* StoredLines = nullptr;
		Late->TryGetArrayField(TEXT("GrimReaperLines"), StoredLines);
		bool bLines = StoredLines && StoredLines->Num() == 6;
		if (bLines)
		{
			for (int32 Index = 0; Index < 6; ++Index)
			{
				bLines = bLines && (*StoredLines)[Index]->AsString() == Lines[Index];
			}
		}
		if (!bLines)
		{
			Errors.Add(TEXT("Final Grim Reaper lines drifted."));
		}
		if (Field(Late, TEXT("AwakeningLine")) != TEXT("Now we begin."))
		{
			Errors.Add(TEXT("Awakening line drifted."));
		}
		double Chairs = 0.0;
		Late->TryGetNumberField(TEXT("Soul11Chairs"), Chairs);
		if (Field(Late, TEXT("Soul11Title")) != TEXT("SOUL 11") || Field(Late, TEXT("Soul11Then")) != TEXT("YOU") || FMath::RoundToInt(Chairs) != 11)
		{
			Errors.Add(TEXT("Soul 11 lock drifted."));
		}
		const TArray<TSharedPtr<FJsonValue>>* FinalMessage = nullptr;
		Late->TryGetArrayField(TEXT("FinalMessage"), FinalMessage);
		if (!FinalMessage || FinalMessage->Num() != 2
			|| (*FinalMessage)[0]->AsString() != TEXT("You cannot change what happened.")
			|| (*FinalMessage)[1]->AsString() != TEXT("But you can change what it means."))
		{
			Errors.Add(TEXT("Final message drifted."));
		}
		if (Field(Late, TEXT("VoidName")) != TEXT("The Void"))
		{
			Errors.Add(TEXT("Void name missing from the lock file."));
		}
		const TCHAR* EndingTitles[] = { TEXT("THE RETURN"), TEXT("THE RELEASE"), TEXT("THE LOST"), TEXT("THE SACRIFICE"), TEXT("THE AWAKENING") };
		const TArray<TSharedPtr<FJsonValue>>* Endings = nullptr;
		Late->TryGetArrayField(TEXT("Endings"), Endings);
		bool bEndings = Endings && Endings->Num() == 5;
		if (bEndings)
		{
			for (int32 Index = 0; Index < 5; ++Index)
			{
				const TSharedPtr<FJsonObject> Ending = (*Endings)[Index]->AsObject();
				bEndings = bEndings && Field(Ending, TEXT("Title")) == EndingTitles[Index];
			}
		}
		if (!bEndings)
		{
			Errors.Add(TEXT("Ending list drifted."));
		}
		const TArray<TSharedPtr<FJsonValue>>* Steps = nullptr;
		Late->TryGetArrayField(TEXT("ImplementSteps"), Steps);
		for (int32 Step : { 16, 17, 18, 19 })
		{
			bool bFound = false;
			if (Steps)
			{
				for (const TSharedPtr<FJsonValue>& Value : *Steps)
				{
					bFound = bFound || FMath::RoundToInt(Value->AsNumber()) == Step;
				}
			}
			if (!bFound)
			{
				Errors.Add(FString::Printf(TEXT("Implement step %d not locked."), Step));
			}
		}
	}

	const TSharedPtr<FJsonObject> Music = LoadObject(DataDir / TEXT("Music.json"), Errors);
	if (Music.IsValid())
	{
		if (Field(Music, TEXT("StrangerMotifFull")) != TEXT("LOCKED_STEP_16"))
		{
			Errors.Add(TEXT("Full stranger motif must stay locked."));
		}
		const TSharedPtr<FJsonObject>* Identities = nullptr;
		Music->TryGetObjectField(TEXT("Identities"), Identities);
		for (const FProtagonist& Protagonist : Protagonists)
		{
			if (!Identities || !(*Identities)->Values.Contains(Protagonist.Id))
			{
				Errors.Add(FString::Printf(TEXT("Music identity missing for %s"), Protagonist.Id));
			}
		}
		if (!Identities || !(*Identities)->Values.Contains(TEXT("CH_STRANGER")))
		{
			Errors.Add(TEXT("Stranger motif fragment missing."));
		}
	}

	const TCHAR* SaveProps[] = {
		TEXT("CurrentChapterId"), TEXT("CurrentCardId"), TEXT("ProtagonistId"), TEXT("TimelineVisited"), TEXT("TimelineUnlocked"),
		TEXT("ChoiceByBranch"), TEXT("CluesFound"), TEXT("MemoriesFound"), TEXT("Relationships"), TEXT("CrossChapterDiscoveries"),
		TEXT("Soul"), TEXT("Dedication"), TEXT("EndingsUnlocked"), TEXT("UnlockedContent"),
	};
	for (const TCHAR* Name : SaveProps)
	{
		RequireProperty(UHDIDSaveGame::StaticClass(), Name, Errors);
		if (!UHDIDSaveManager::RequiredStoryProperties().Contains(Name))
		{
			Errors.Add(FString::Printf(TEXT("SaveGame missing %s"), Name));
		}
	}
	const TCHAR* SoulFields[] = {
		TEXT("Truth"), TEXT("Empathy"), TEXT("Courage"), TEXT("Sacrifice"), TEXT("Curiosity"),
		TEXT("Honesty"), TEXT("Compassion"), TEXT("Understanding"), TEXT("Dedication"),
	};
	for (const TCHAR* Name : SoulFields)
	{
		RequireProperty(FHDIDSoulValues::StaticStruct(), Name, Errors);
	}
	const TCHAR* DedicationFields[] = {
		TEXT("CluesFound"), TEXT("OptionalCluesFound"), TEXT("FlashbacksDiscovered"), TEXT("AlternateTimelines"),
		TEXT("NPCsHelped"), TEXT("OptionalDialogue"), TEXT("HiddenObjects"), TEXT("CrossChapterConnections"),
	};
	for (const TCHAR* Name : DedicationFields)
	{
		RequireProperty(FHDIDDedicationValues::StaticStruct(), Name, Errors);
	}

	TArray<FString> FileNames;
	IFileManager::Get().FindFiles(FileNames, *(DataDir / TEXT("Chapters/*.json")), true, false);
	FileNames.Sort();
	TArray<TSharedPtr<FJsonObject>> Chapters;
	for (const FString& FileName : FileNames)
	{
		if (const TSharedPtr<FJsonObject> Chapter = LoadObject(DataDir / TEXT("Chapters") / FileName, Errors))
		{
			Chapters.Add(Chapter);
		}
	}
	bool bOrder = Chapters.Num() == 10;
	if (bOrder)
	{
		for (int32 Index = 0; Index < 10; ++Index)
		{
			bOrder = bOrder && Field(Chapters[Index], TEXT("ChapterID")) == FString::Printf(TEXT("CH%02d"), Index + 1);
		}
	}
	if (!bOrder)
	{
		Errors.Add(TEXT("Expected chapters CH01–CH10."));
	}

	TSet<FString> ClueIds;
	TSet<FString> DialogueIds;
	TSet<FString> ChoiceIds;
	TSet<FString> CardIds;
	TSet<FString> EventIds;
	TSet<FString> MemoryIds;
	TSet<FString> KnownChapters;
	for (const TSharedPtr<FJsonObject>& Chapter : Chapters)
	{
		KnownChapters.Add(Field(Chapter, TEXT("ChapterID")));
	}
	KnownChapters.Add(TEXT("ALL"));
	KnownChapters.Add(TEXT("LATE"));

	const TCHAR* CardKeys[] = {
		TEXT("CardID"), TEXT("ChapterID"), TEXT("Scene"), TEXT("Location"), TEXT("Time"), TEXT("Characters"), TEXT("Objectives"),
		TEXT("Clues"), TEXT("Dialogue"), TEXT("Choices"), TEXT("Flashbacks"), TEXT("Consequences"), TEXT("Audio"), TEXT("Camera"), TEXT("NextCards"),
	};
	const TCHAR* ClueKeys[] = {
		TEXT("ClueID"), TEXT("ChapterID"), TEXT("CardID"), TEXT("Description"), TEXT("Location"), TEXT("DiscoveryMethod"),
		TEXT("RelatedCharacters"), TEXT("RelatedEvents"), TEXT("RelatedClues"), TEXT("MemoryReference"), TEXT("Importance"), TEXT("Optional"),
	};
	const TCHAR* DialogueKeys[] = {
		TEXT("DialogueID"), TEXT("Speaker"), TEXT("Text"), TEXT("Choices"), TEXT("Requirements"), TEXT("Consequences"),
		TEXT("ClueUnlocks"), TEXT("RelationshipChanges"), TEXT("SoulChanges"), TEXT("NextNode"),
	};

	for (int32 ChapterIndex = 0; ChapterIndex < Chapters.Num(); ++ChapterIndex)
	{
		const TSharedPtr<FJsonObject>& Chapter = Chapters[ChapterIndex];
		const FString ChapterId = Field(Chapter, TEXT("ChapterID"));
		const bool bIndexed = ChapterIndex >= 0 && ChapterIndex < 10;
		if (bIndexed)
		{
			if (Field(Chapter, TEXT("ProtagonistId")) != Protagonists[ChapterIndex].Id)
			{
				Errors.Add(FString::Printf(TEXT("%s protagonist mismatch."), *ChapterId));
			}
			if (Field(Chapter, TEXT("Theme")) != Protagonists[ChapterIndex].Theme)
			{
				Errors.Add(FString::Printf(TEXT("%s theme mismatch."), *ChapterId));
			}
			if (Field(Chapter, TEXT("CoinBeat")) != CoinBeats[ChapterIndex])
			{
				Errors.Add(FString::Printf(TEXT("%s coin beat drifted."), *ChapterId));
			}
			if (Field(Chapter, TEXT("HourglassBeat")) != HourglassBeats[ChapterIndex] || !Field(Chapter, TEXT("HourglassBeat")).ToLower().Contains(TEXT("hourglass")))
			{
				Errors.Add(FString::Printf(TEXT("%s hourglass beat drifted."), *ChapterId));
			}
			if (Field(Chapter, TEXT("StrangerStage")) != StrangerStages[ChapterIndex])
			{
				Errors.Add(FString::Printf(TEXT("%s stranger stage drifted."), *ChapterId));
			}
			double Saturation = 0.0;
			Chapter->TryGetNumberField(TEXT("ColourSaturation"), Saturation);
			if (FMath::Abs(static_cast<float>(Saturation) - Saturations[ChapterIndex]) > 0.001f)
			{
				Errors.Add(FString::Printf(TEXT("%s colour saturation drifted."), *ChapterId));
			}
		}
		bool bPlayable = false;
		Chapter->TryGetBoolField(TEXT("Playable"), bPlayable);
		if (ChapterId != TEXT("CH01") && bPlayable)
		{
			Errors.Add(FString::Printf(TEXT("%s must not be playable in this slice."), *ChapterId));
		}
		if (ChapterId == TEXT("CH01") && !bPlayable)
		{
			Errors.Add(TEXT("Chapter 01 must be playable."));
		}

		const TArray<TSharedPtr<FJsonValue>>* Cards = nullptr;
		Chapter->TryGetArrayField(TEXT("Cards"), Cards);
		if (!Cards || Cards->Num() != 12)
		{
			Errors.Add(FString::Printf(TEXT("%s must have exactly 12 cards."), *ChapterId));
		}
		const TArray<TSharedPtr<FJsonValue>>* Refs = nullptr;
		Chapter->TryGetArrayField(TEXT("CrossChapterRefs"), Refs);
		if (!Refs || Refs->Num() < 3)
		{
			Errors.Add(FString::Printf(TEXT("%s needs at least three cross-chapter references."), *ChapterId));
		}
		bool bCoin = false;
		bool bGlass = false;
		if (Refs)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Refs)
			{
				const TSharedPtr<FJsonObject> Ref = Value->AsObject();
				const FString Kind = Field(Ref, TEXT("Kind"));
				bCoin = bCoin || Kind == TEXT("coin");
				bGlass = bGlass || Kind == TEXT("hourglass");
				if (!IsOneOf(Kind, { TEXT("coin"), TEXT("hourglass"), TEXT("phrase"), TEXT("figure"), TEXT("photograph"), TEXT("newspaper"), TEXT("painting"), TEXT("location"), TEXT("motif"), TEXT("symbol") }))
				{
					Errors.Add(FString::Printf(TEXT("%s bad cross kind %s"), *ChapterId, *Kind));
				}
				if (!KnownChapters.Contains(Field(Ref, TEXT("TargetChapter"))))
				{
					Errors.Add(FString::Printf(TEXT("%s cross target %s missing."), *ChapterId, *Field(Ref, TEXT("TargetChapter"))));
				}
				if (Field(Ref, TEXT("Note")).IsEmpty())
				{
					Errors.Add(FString::Printf(TEXT("%s cross ref %s missing note."), *ChapterId, *Field(Ref, TEXT("Id"))));
				}
			}
		}
		if (Refs && (!bCoin || !bGlass))
		{
			Errors.Add(FString::Printf(TEXT("%s cross refs must include coin and hourglass."), *ChapterId));
		}

		TMap<FString, TSharedPtr<FJsonObject>> Consequences;
		const TArray<TSharedPtr<FJsonValue>>* ConsequenceValues = nullptr;
		Chapter->TryGetArrayField(TEXT("Consequences"), ConsequenceValues);
		if (ConsequenceValues)
		{
			for (const TSharedPtr<FJsonValue>& Value : *ConsequenceValues)
			{
				const TSharedPtr<FJsonObject> Consequence = Value->AsObject();
				const FString Id = Field(Consequence, TEXT("Id"));
				if (Consequences.Contains(Id))
				{
					Errors.Add(FString::Printf(TEXT("Duplicate consequence %s"), *Id));
				}
				Consequences.Add(Id, Consequence);
			}
		}

		TMap<FString, TSharedPtr<FJsonObject>> LocalCards;
		if (Cards)
		{
			for (int32 CardIndex = 0; CardIndex < Cards->Num(); ++CardIndex)
			{
				const TSharedPtr<FJsonObject> Card = (*Cards)[CardIndex]->AsObject();
				for (const TCHAR* Key : CardKeys)
				{
					if (!Has(Card, Key))
					{
						Errors.Add(FString::Printf(TEXT("%s card missing %s"), *ChapterId, Key));
					}
				}
				const FString CardId = Field(Card, TEXT("CardID"));
				if (CardIds.Contains(CardId))
				{
					Errors.Add(FString::Printf(TEXT("Duplicate card %s"), *CardId));
				}
				CardIds.Add(CardId);
				LocalCards.Add(CardId, Card);
				if (Field(Card, TEXT("ChapterID")) != ChapterId)
				{
					Errors.Add(FString::Printf(TEXT("Card %s chapter mismatch."), *CardId));
				}
				int32 Minutes = 0;
				if (!HDIDParseClock(Field(Card, TEXT("Time")), Minutes))
				{
					Errors.Add(FString::Printf(TEXT("Card %s bad time."), *CardId));
				}
				const TArray<TSharedPtr<FJsonValue>>* Next = nullptr;
				Card->TryGetArrayField(TEXT("NextCards"), Next);
				if (CardIndex + 1 < Cards->Num())
				{
					const FString Expected = Field((*Cards)[CardIndex + 1]->AsObject(), TEXT("CardID"));
					if (!Next || Next->Num() != 1 || (*Next)[0]->AsString() != Expected)
					{
						Errors.Add(FString::Printf(TEXT("%s must unlock the next card."), *CardId));
					}
				}
				else if (Next && Next->Num() > 0)
				{
					Errors.Add(FString::Printf(TEXT("%s is last and must not point onward."), *CardId));
				}
			}
		}

		TSet<FString> LocationIds;
		const TArray<TSharedPtr<FJsonValue>>* Locations = nullptr;
		Chapter->TryGetArrayField(TEXT("Locations"), Locations);
		if (Locations)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Locations)
			{
				LocationIds.Add(Field(Value->AsObject(), TEXT("LocationId")));
			}
		}

		TArray<int32> Times;
		TMap<FString, TSharedPtr<FJsonObject>> LocalEvents;
		const TArray<TSharedPtr<FJsonValue>>* Timeline = nullptr;
		Chapter->TryGetArrayField(TEXT("Timeline"), Timeline);
		if (Timeline)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Timeline)
			{
				const TSharedPtr<FJsonObject> Event = Value->AsObject();
				const FString EventId = Field(Event, TEXT("EventID"));
				if (EventIds.Contains(EventId))
				{
					Errors.Add(FString::Printf(TEXT("Duplicate event %s"), *EventId));
				}
				EventIds.Add(EventId);
				LocalEvents.Add(EventId, Event);
				int32 Minutes = 0;
				if (!HDIDParseClock(Field(Event, TEXT("Time")), Minutes))
				{
					Errors.Add(FString::Printf(TEXT("Event %s bad time."), *EventId));
				}
				else
				{
					Times.Add(Minutes);
				}
				if (!LocationIds.Contains(Field(Event, TEXT("LocationId"))))
				{
					Errors.Add(FString::Printf(TEXT("Event %s missing location."), *EventId));
				}
			}
		}
		TArray<int32> SortedTimes = Times;
		SortedTimes.Sort();
		TSet<int32> UniqueTimes;
		for (int32 Minute : Times)
		{
			UniqueTimes.Add(Minute);
		}
		if (Times != SortedTimes || UniqueTimes.Num() != Times.Num())
		{
			Errors.Add(FString::Printf(TEXT("%s timeline is not strictly chronological."), *ChapterId));
		}

		TMap<FString, TSharedPtr<FJsonObject>> LocalClues;
		const TArray<TSharedPtr<FJsonValue>>* Clues = nullptr;
		Chapter->TryGetArrayField(TEXT("Clues"), Clues);
		const TArray<TSharedPtr<FJsonValue>>* Memories = nullptr;
		Chapter->TryGetArrayField(TEXT("Memories"), Memories);
		auto MemoryExists = [&Memories](const FString& MemoryId)
		{
			if (!Memories)
			{
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Value : *Memories)
			{
				if (Field(Value->AsObject(), TEXT("MemoryID")) == MemoryId)
				{
					return true;
				}
			}
			return false;
		};
		if (Clues)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Clues)
			{
				const TSharedPtr<FJsonObject> Clue = Value->AsObject();
				for (const TCHAR* Key : ClueKeys)
				{
					if (!Has(Clue, Key))
					{
						Errors.Add(FString::Printf(TEXT("Clue missing %s"), Key));
					}
				}
				const FString ClueId = Field(Clue, TEXT("ClueID"));
				if (ClueIds.Contains(ClueId))
				{
					Errors.Add(FString::Printf(TEXT("Duplicate clue %s"), *ClueId));
				}
				ClueIds.Add(ClueId);
				LocalClues.Add(ClueId, Clue);
				if (Field(Clue, TEXT("ChapterID")) != ChapterId)
				{
					Errors.Add(FString::Printf(TEXT("Clue %s chapter mismatch."), *ClueId));
				}
				if (!LocalCards.Contains(Field(Clue, TEXT("CardID"))))
				{
					Errors.Add(FString::Printf(TEXT("Clue %s card missing."), *ClueId));
				}
				if (Field(Clue, TEXT("Description")).IsEmpty() || Field(Clue, TEXT("Location")).IsEmpty() || Field(Clue, TEXT("DiscoveryMethod")).IsEmpty())
				{
					Errors.Add(FString::Printf(TEXT("Clue %s empty required text."), *ClueId));
				}
				if (!IsOneOf(Field(Clue, TEXT("DiscoveryMethod")), { TEXT("physical"), TEXT("visual"), TEXT("audio"), TEXT("temporal"), TEXT("dialogue"), TEXT("documentary"), TEXT("photographic"), TEXT("environmental") }))
				{
					Errors.Add(FString::Printf(TEXT("Clue %s bad discovery method."), *ClueId));
				}
				if (!IsOneOf(Field(Clue, TEXT("Importance")), { TEXT("critical"), TEXT("major"), TEXT("minor") }))
				{
					Errors.Add(FString::Printf(TEXT("Clue %s bad importance."), *ClueId));
				}
				const FString MemoryReference = Field(Clue, TEXT("MemoryReference"));
				if (!MemoryReference.IsEmpty() && !MemoryExists(MemoryReference))
				{
					Errors.Add(FString::Printf(TEXT("Clue %s memory reference missing."), *ClueId));
				}
			}
			for (const TSharedPtr<FJsonValue>& Value : *Clues)
			{
				const TSharedPtr<FJsonObject> Clue = Value->AsObject();
				const FString ClueId = Field(Clue, TEXT("ClueID"));
				const TArray<TSharedPtr<FJsonValue>>* RelatedClues = nullptr;
				Clue->TryGetArrayField(TEXT("RelatedClues"), RelatedClues);
				if (RelatedClues)
				{
					for (const TSharedPtr<FJsonValue>& Related : *RelatedClues)
					{
						const FString RelatedId = Related->AsString();
						if (!LocalClues.Contains(RelatedId) && !ClueIds.Contains(RelatedId))
						{
							Errors.Add(FString::Printf(TEXT("Clue %s related clue %s missing."), *ClueId, *RelatedId));
						}
					}
				}
				const TArray<TSharedPtr<FJsonValue>>* RelatedCharacters = nullptr;
				Clue->TryGetArrayField(TEXT("RelatedCharacters"), RelatedCharacters);
				if (RelatedCharacters)
				{
					for (const TSharedPtr<FJsonValue>& Who : *RelatedCharacters)
					{
						if (!Characters.Contains(Who->AsString()))
						{
							Errors.Add(FString::Printf(TEXT("Clue %s unknown character %s."), *ClueId, *Who->AsString()));
						}
					}
				}
				const TArray<TSharedPtr<FJsonValue>>* RelatedEvents = nullptr;
				Clue->TryGetArrayField(TEXT("RelatedEvents"), RelatedEvents);
				if (RelatedEvents)
				{
					for (const TSharedPtr<FJsonValue>& EventId : *RelatedEvents)
					{
						if (!LocalEvents.Contains(EventId->AsString()))
						{
							Errors.Add(FString::Printf(TEXT("Clue %s unknown event %s."), *ClueId, *EventId->AsString()));
						}
					}
				}
			}
		}

		TMap<FString, TSharedPtr<FJsonObject>> LocalDialogues;
		TMap<FString, TSharedPtr<FJsonObject>> LocalChoices;
		const TArray<TSharedPtr<FJsonValue>>* Dialogue = nullptr;
		Chapter->TryGetArrayField(TEXT("Dialogue"), Dialogue);
		if (Dialogue)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Dialogue)
			{
				const TSharedPtr<FJsonObject> Node = Value->AsObject();
				for (const TCHAR* Key : DialogueKeys)
				{
					if (!Has(Node, Key))
					{
						Errors.Add(FString::Printf(TEXT("Dialogue missing %s"), Key));
					}
				}
				const FString DialogueId = Field(Node, TEXT("DialogueID"));
				if (DialogueIds.Contains(DialogueId))
				{
					Errors.Add(FString::Printf(TEXT("Duplicate dialogue %s"), *DialogueId));
				}
				DialogueIds.Add(DialogueId);
				LocalDialogues.Add(DialogueId, Node);
				if (!Characters.Contains(Field(Node, TEXT("Speaker"))))
				{
					Errors.Add(FString::Printf(TEXT("Dialogue %s speaker missing."), *DialogueId));
				}
				bool bReveal = false;
				Node->TryGetBoolField(TEXT("Reveal"), bReveal);
				if (bReveal)
				{
					Errors.Add(FString::Printf(TEXT("Reveal dialogue %s is locked."), *DialogueId));
				}
				const TArray<TSharedPtr<FJsonValue>>* NodeChoices = nullptr;
				Node->TryGetArrayField(TEXT("Choices"), NodeChoices);
				TArray<FString> Signatures;
				if (NodeChoices)
				{
					for (const TSharedPtr<FJsonValue>& ChoiceValue : *NodeChoices)
					{
						const TSharedPtr<FJsonObject> Choice = ChoiceValue->AsObject();
						const FString ChoiceId = Field(Choice, TEXT("ChoiceID"));
						if (ChoiceIds.Contains(ChoiceId))
						{
							Errors.Add(FString::Printf(TEXT("Duplicate choice %s"), *ChoiceId));
						}
						ChoiceIds.Add(ChoiceId);
						LocalChoices.Add(ChoiceId, Choice);
						const TSharedPtr<FJsonObject>* Consequence = Consequences.Find(Field(Choice, TEXT("ConsequenceID")));
						if (!Consequence || !Consequence->IsValid())
						{
							Errors.Add(FString::Printf(TEXT("Choice %s consequence missing."), *ChoiceId));
							continue;
						}
						const TSharedPtr<FJsonObject>* Flags = nullptr;
						const TArray<TSharedPtr<FJsonValue>>* Reveals = nullptr;
						const TSharedPtr<FJsonObject>* Soul = nullptr;
						(*Consequence)->TryGetObjectField(TEXT("SetFlags"), Flags);
						(*Consequence)->TryGetArrayField(TEXT("RevealClues"), Reveals);
						(*Consequence)->TryGetObjectField(TEXT("Soul"), Soul);
						Signatures.Add(Field(Choice, TEXT("ConsequenceID")) + TEXT("#") + Field(Choice, TEXT("NextNode")) + TEXT("#")
							+ FlagSignature(Flags ? *Flags : nullptr) + TEXT("#") + ListSignature(Reveals) + TEXT("#") + SoulSignature(Soul ? *Soul : nullptr));
					}
				}
				if (NodeChoices && NodeChoices->Num() >= 2)
				{
					TSet<FString> Unique;
					for (const FString& Signature : Signatures)
					{
						Unique.Add(Signature);
					}
					if (Unique.Num() < Signatures.Num())
					{
						Errors.Add(FString::Printf(TEXT("Dialogue %s has identical choice results."), *DialogueId));
					}
				}
			}
			for (const TPair<FString, TSharedPtr<FJsonObject>>& Pair : LocalDialogues)
			{
				const FString Next = Field(Pair.Value, TEXT("NextNode"));
				if (!Next.IsEmpty() && !LocalDialogues.Contains(Next))
				{
					Errors.Add(FString::Printf(TEXT("Dialogue %s next node missing."), *Pair.Key));
				}
				const TArray<TSharedPtr<FJsonValue>>* NodeChoices = nullptr;
				Pair.Value->TryGetArrayField(TEXT("Choices"), NodeChoices);
				if (!NodeChoices)
				{
					continue;
				}
				for (const TSharedPtr<FJsonValue>& ChoiceValue : *NodeChoices)
				{
					const FString NextChoice = Field(ChoiceValue->AsObject(), TEXT("NextNode"));
					if (!NextChoice.IsEmpty() && !LocalDialogues.Contains(NextChoice))
					{
						Errors.Add(FString::Printf(TEXT("Choice %s next node missing."), *Field(ChoiceValue->AsObject(), TEXT("ChoiceID"))));
					}
				}
			}
		}

		TMap<FString, TArray<TSharedPtr<FJsonObject>>> Groups;
		for (const TPair<FString, TSharedPtr<FJsonObject>>& Pair : LocalChoices)
		{
			const FString Group = Field(Pair.Value, TEXT("BranchGroup"));
			if (!Group.IsEmpty())
			{
				Groups.FindOrAdd(Group).Add(Pair.Value);
			}
		}
		for (const TPair<FString, TArray<TSharedPtr<FJsonObject>>>& Group : Groups)
		{
			TSet<FString> FlagSets;
			TSet<FString> Flags;
			bool bComplete = true;
			for (const TSharedPtr<FJsonObject>& Choice : Group.Value)
			{
				const TSharedPtr<FJsonObject>* Consequence = Consequences.Find(Field(Choice, TEXT("ConsequenceID")));
				if (!Consequence || !Consequence->IsValid())
				{
					bComplete = false;
					continue;
				}
				const TSharedPtr<FJsonObject>* SetFlags = nullptr;
				(*Consequence)->TryGetObjectField(TEXT("SetFlags"), SetFlags);
				FlagSets.Add(FlagSignature(SetFlags ? *SetFlags : nullptr));
				if (SetFlags)
				{
					for (const TPair<FString, TSharedPtr<FJsonValue>>& Flag : (*SetFlags)->Values)
					{
						Flags.Add(Flag.Key);
					}
				}
			}
			if (bComplete && FlagSets.Num() < 2)
			{
				Errors.Add(FString::Printf(TEXT("%s branch %s does not change flags."), *ChapterId, *Group.Key));
			}
			bool bUsed = false;
			if (Cards)
			{
				for (const TSharedPtr<FJsonValue>& CardValue : *Cards)
				{
					const TArray<TSharedPtr<FJsonValue>>* Variants = nullptr;
					CardValue->AsObject()->TryGetArrayField(TEXT("Variants"), Variants);
					if (!Variants)
					{
						continue;
					}
					for (const TSharedPtr<FJsonValue>& Variant : *Variants)
					{
						const TArray<TSharedPtr<FJsonValue>>* IfAll = nullptr;
						Variant->AsObject()->TryGetArrayField(TEXT("IfAll"), IfAll);
						bUsed = bUsed || FlagsMentioned(IfAll, Flags);
					}
				}
			}
			const TArray<TSharedPtr<FJsonValue>>* Interactables = nullptr;
			Chapter->TryGetArrayField(TEXT("Interactables"), Interactables);
			if (Interactables)
			{
				for (const TSharedPtr<FJsonValue>& Item : *Interactables)
				{
					const TArray<TSharedPtr<FJsonValue>>* Present = nullptr;
					Item->AsObject()->TryGetArrayField(TEXT("PresentIf"), Present);
					bUsed = bUsed || FlagsMentioned(Present, Flags);
				}
			}
			if (Clues)
			{
				for (const TSharedPtr<FJsonValue>& Item : *Clues)
				{
					const TArray<TSharedPtr<FJsonValue>>* Active = nullptr;
					Item->AsObject()->TryGetArrayField(TEXT("ActiveIf"), Active);
					bUsed = bUsed || FlagsMentioned(Active, Flags);
				}
			}
			if (!bUsed)
			{
				Errors.Add(FString::Printf(TEXT("%s branch %s does not change later narrative state."), *ChapterId, *Group.Key));
			}
		}

		if (Cards)
		{
			for (const TSharedPtr<FJsonValue>& CardValue : *Cards)
			{
				const TSharedPtr<FJsonObject> Card = CardValue->AsObject();
				const FString CardId = Field(Card, TEXT("CardID"));
				auto RequireIds = [&](const TCHAR* Key, const TMap<FString, TSharedPtr<FJsonObject>>& Known, const TCHAR* Label)
				{
					const TArray<TSharedPtr<FJsonValue>>* Ids = nullptr;
					Card->TryGetArrayField(Key, Ids);
					if (!Ids)
					{
						return;
					}
					for (const TSharedPtr<FJsonValue>& Id : *Ids)
					{
						if (!Known.Contains(Id->AsString()))
						{
							Errors.Add(FString::Printf(TEXT("Card %s unknown %s %s."), *CardId, Label, *Id->AsString()));
						}
					}
				};
				RequireIds(TEXT("NextCards"), LocalCards, TEXT("next card"));
				RequireIds(TEXT("Clues"), LocalClues, TEXT("clue"));
				RequireIds(TEXT("Dialogue"), LocalDialogues, TEXT("dialogue"));
				RequireIds(TEXT("Choices"), LocalChoices, TEXT("choice"));
				const TArray<TSharedPtr<FJsonValue>>* Flashbacks = nullptr;
				Card->TryGetArrayField(TEXT("Flashbacks"), Flashbacks);
				if (Flashbacks)
				{
					for (const TSharedPtr<FJsonValue>& MemoryId : *Flashbacks)
					{
						if (!MemoryExists(MemoryId->AsString()))
						{
							Errors.Add(FString::Printf(TEXT("Card %s unknown flashback %s."), *CardId, *MemoryId->AsString()));
						}
					}
				}
			}
		}

		if (Timeline)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Timeline)
			{
				const TSharedPtr<FJsonObject> Event = Value->AsObject();
				const FString Unlock = Field(Event, TEXT("UnlockOnClue"));
				if (!Unlock.IsEmpty() && !LocalClues.Contains(Unlock))
				{
					Errors.Add(FString::Printf(TEXT("Event %s unlock clue missing."), *Field(Event, TEXT("EventID"))));
				}
				const TArray<TSharedPtr<FJsonValue>>* Branches = nullptr;
				Event->TryGetArrayField(TEXT("Branches"), Branches);
				if (!Branches)
				{
					continue;
				}
				for (const TSharedPtr<FJsonValue>& Branch : *Branches)
				{
					if (!LocalChoices.Contains(Field(Branch->AsObject(), TEXT("ChoiceID"))))
					{
						Errors.Add(FString::Printf(TEXT("Event %s branch choice missing."), *Field(Event, TEXT("EventID"))));
					}
				}
			}
		}

		int32 PlayableMemories = 0;
		if (Memories)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Memories)
			{
				const TSharedPtr<FJsonObject> Memory = Value->AsObject();
				const FString MemoryId = Field(Memory, TEXT("MemoryID"));
				if (MemoryIds.Contains(MemoryId))
				{
					Errors.Add(FString::Printf(TEXT("Duplicate memory %s"), *MemoryId));
				}
				MemoryIds.Add(MemoryId);
				if (Field(Memory, TEXT("QuestionAnswered")).IsEmpty() || Field(Memory, TEXT("QuestionOpened")).IsEmpty())
				{
					Errors.Add(FString::Printf(TEXT("Memory %s does not reveal and open a question."), *MemoryId));
				}
				if (!LocalClues.Contains(Field(Memory, TEXT("RequiredClue"))))
				{
					Errors.Add(FString::Printf(TEXT("Memory %s required clue missing."), *MemoryId));
				}
				if (!LocalEvents.Contains(Field(Memory, TEXT("TimelineEventID"))))
				{
					Errors.Add(FString::Printf(TEXT("Memory %s timeline event missing."), *MemoryId));
				}
				bool bMemoryPlayable = false;
				Memory->TryGetBoolField(TEXT("Playable"), bMemoryPlayable);
				if (bMemoryPlayable)
				{
					++PlayableMemories;
				}
			}
		}

		if (ChapterId == TEXT("CH01"))
		{
			FString Raw;
			FFileHelper::LoadFileToString(Raw, *(DataDir / TEXT("Chapters/CH01.json")));
			const FString Lower = Raw.ToLower();
			const TCHAR* Forbidden[] = { TEXT("grim reaper"), TEXT("the void"), TEXT("soul 11"), TEXT("you died of"), TEXT("heart attack"), TEXT("cardiac") };
			for (const TCHAR* Phrase : Forbidden)
			{
				if (Lower.Contains(Phrase))
				{
					Errors.Add(FString::Printf(TEXT("Chapter 01 contains forbidden phrase: %s"), Phrase));
				}
			}
			if (PlayableMemories < 1)
			{
				Errors.Add(TEXT("Chapter 01 needs a playable flashback."));
			}
			if (!Raw.Contains(TEXT("You don't have much time.")) || !Raw.Contains(TEXT("You shouldn't open that door.")))
			{
				Errors.Add(TEXT("Chapter 01 is missing the stranger's double-edged lines."));
			}
			bool bStranger = false;
			bool bCoinObject = false;
			bool bBranched = false;
			const TArray<TSharedPtr<FJsonValue>>* Interactables = nullptr;
			Chapter->TryGetArrayField(TEXT("Interactables"), Interactables);
			if (Interactables)
			{
				for (const TSharedPtr<FJsonValue>& Item : *Interactables)
				{
					bStranger = bStranger || Field(Item->AsObject(), TEXT("Kind")) == TEXT("GrimReaper");
					bCoinObject = bCoinObject || Field(Item->AsObject(), TEXT("ClueID")) == TEXT("CLUE_COIN");
				}
			}
			if (Timeline)
			{
				for (const TSharedPtr<FJsonValue>& Value : *Timeline)
				{
					const TArray<TSharedPtr<FJsonValue>>* Branches = nullptr;
					Value->AsObject()->TryGetArrayField(TEXT("Branches"), Branches);
					bBranched = bBranched || (Branches && Branches->Num() >= 2);
				}
			}
			if (!bStranger)
			{
				Errors.Add(TEXT("Chapter 01 stranger interactable missing."));
			}
			if (!bCoinObject)
			{
				Errors.Add(TEXT("Chapter 01 coin interactable missing."));
			}
			if (!bBranched)
			{
				Errors.Add(TEXT("Chapter 01 timeline branch missing."));
			}
		}
	}

	return Report;
}

void FHDIDNarrativeValidator::LogReport(const FHDIDValidationReport& Report)
{
	if (Report.IsValid())
	{
		UE_LOG(LogHDID, Display, TEXT("Narrative data valid: 10 chapters, Chapter 01 playable, late game locked."));
		return;
	}
	UE_LOG(LogHDID, Error, TEXT("NARRATIVE VALIDATION FAILED (%d)"), Report.Errors.Num());
	for (const FString& Error : Report.Errors)
	{
		UE_LOG(LogHDID, Error, TEXT(" - %s"), *Error);
	}
}

static void HDIDRunValidateNarrative()
{
	FHDIDNarrativeValidator::LogReport(FHDIDNarrativeValidator::ValidateProject());
}

static FAutoConsoleCommand GHDIDValidateNarrative(
	TEXT("HDID.ValidateNarrative"),
	TEXT("Validate Content/Data against the narrative rules."),
	FConsoleCommandDelegate::CreateStatic(&HDIDRunValidateNarrative));
