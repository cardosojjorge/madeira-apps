#pragma once

#include "CoreMinimal.h"
#include "HDIDTypes.generated.h"

/** World-space centimetres. JSON object {"X":0,"Y":0,"Z":0}. */
USTRUCT(BlueprintType)
struct FHDIDVec3
{
	GENERATED_BODY()

	UPROPERTY()
	float X = 0.f;

	UPROPERTY()
	float Y = 0.f;

	UPROPERTY()
	float Z = 0.f;

	FVector ToUE() const { return FVector(X, Y, Z); }
};

/** All listed conditions must match. Empty means "no requirement". */
USTRUCT(BlueprintType)
struct FHDIDCondition
{
	GENERATED_BODY()

	UPROPERTY()
	FString Flag;

	UPROPERTY()
	FString Equals;
};

/** Hidden soul values. The same shape is a delta and a total. */
USTRUCT(BlueprintType)
struct FHDIDSoulValues
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Truth = 0;

	UPROPERTY()
	int32 Empathy = 0;

	UPROPERTY()
	int32 Courage = 0;

	UPROPERTY()
	int32 Sacrifice = 0;

	UPROPERTY()
	int32 Curiosity = 0;

	UPROPERTY()
	int32 Honesty = 0;

	UPROPERTY()
	int32 Compassion = 0;

	UPROPERTY()
	int32 Understanding = 0;

	UPROPERTY()
	int32 Dedication = 0;

	void Add(const FHDIDSoulValues& Other)
	{
		Truth += Other.Truth;
		Empathy += Other.Empathy;
		Courage += Other.Courage;
		Sacrifice += Other.Sacrifice;
		Curiosity += Other.Curiosity;
		Honesty += Other.Honesty;
		Compassion += Other.Compassion;
		Understanding += Other.Understanding;
		Dedication += Other.Dedication;
	}
};

USTRUCT(BlueprintType)
struct FHDIDDedicationValues
{
	GENERATED_BODY()

	UPROPERTY()
	int32 CluesFound = 0;

	UPROPERTY()
	int32 OptionalCluesFound = 0;

	UPROPERTY()
	int32 FlashbacksDiscovered = 0;

	UPROPERTY()
	int32 AlternateTimelines = 0;

	UPROPERTY()
	int32 NPCsHelped = 0;

	UPROPERTY()
	int32 OptionalDialogue = 0;

	UPROPERTY()
	int32 HiddenObjects = 0;

	UPROPERTY()
	int32 CrossChapterConnections = 0;
};

USTRUCT(BlueprintType)
struct FHDIDCrossRef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString TargetChapter;

	UPROPERTY()
	FString Kind;

	UPROPERTY()
	FString Note;
};

USTRUCT(BlueprintType)
struct FHDIDCharacterDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	int32 Age = 0;

	UPROPERTY()
	FString Profession;

	UPROPERTY()
	FString Theme;

	/** Safe to show in the notebook. Never a solved conclusion. */
	UPROPERTY()
	FString NotebookIntro;

	UPROPERTY()
	FString VoiceNote;
};

USTRUCT(BlueprintType)
struct FHDIDPropDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Mesh = TEXT("Cube");

	UPROPERTY()
	FHDIDVec3 Position;

	UPROPERTY()
	FHDIDVec3 Scale;

	UPROPERTY()
	FString Color = TEXT("14161C");

	UPROPERTY()
	bool BlocksPlayer = true;
};

USTRUCT(BlueprintType)
struct FHDIDLightDef
{
	GENERATED_BODY()

	UPROPERTY()
	FHDIDVec3 Position;

	UPROPERTY()
	FString Color = TEXT("9BB4C8");

	UPROPERTY()
	float Intensity = 1200.f;

	UPROPERTY()
	float Radius = 600.f;
};

USTRUCT(BlueprintType)
struct FHDIDLocationDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString LocationId;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	FHDIDVec3 Origin;

	UPROPERTY()
	float SizeX = 1000.f;

	UPROPERTY()
	float SizeY = 1000.f;

	UPROPERTY()
	float Height = 400.f;

	UPROPERTY()
	bool Corridor = false;

	UPROPERTY()
	bool OpenEast = false;

	UPROPERTY()
	bool OpenWest = false;

	UPROPERTY()
	bool OpenNorth = false;

	UPROPERTY()
	bool OpenSouth = false;

	UPROPERTY()
	FHDIDVec3 Spawn;

	UPROPERTY()
	float SpawnYaw = 0.f;

	UPROPERTY()
	TArray<FHDIDPropDef> Props;

	UPROPERTY()
	TArray<FHDIDLightDef> Lights;
};

USTRUCT(BlueprintType)
struct FHDIDCardVariant
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	TArray<FHDIDCondition> IfAll;

	UPROPERTY()
	FString Body;
};

USTRUCT(BlueprintType)
struct FHDIDCardDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString CardID;

	UPROPERTY()
	FString ChapterID;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Scene;

	UPROPERTY()
	FString Location;

	UPROPERTY()
	FString Time;

	UPROPERTY()
	TArray<FString> Characters;

	UPROPERTY()
	TArray<FString> Objectives;

	UPROPERTY()
	TArray<FString> Clues;

	UPROPERTY()
	TArray<FString> Dialogue;

	UPROPERTY()
	TArray<FString> Choices;

	UPROPERTY()
	TArray<FString> Flashbacks;

	UPROPERTY()
	TArray<FString> Consequences;

	UPROPERTY()
	TArray<FString> Audio;

	UPROPERTY()
	FString Camera;

	UPROPERTY()
	TArray<FString> NextCards;

	UPROPERTY()
	FString Body;

	UPROPERTY()
	TArray<FHDIDCardVariant> Variants;
};

USTRUCT(BlueprintType)
struct FHDIDChoiceDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString ChoiceID;

	UPROPERTY()
	FString Text;

	UPROPERTY()
	FString BranchGroup;

	UPROPERTY()
	FString ConsequenceID;

	UPROPERTY()
	FString NextNode;

	UPROPERTY()
	TArray<FHDIDCondition> Requirements;
};

USTRUCT(BlueprintType)
struct FHDIDDialogueNode
{
	GENERATED_BODY()

	UPROPERTY()
	FString DialogueID;

	UPROPERTY()
	FString Speaker;

	UPROPERTY()
	FString Text;

	UPROPERTY()
	TArray<FHDIDChoiceDef> Choices;

	UPROPERTY()
	TArray<FHDIDCondition> Requirements;

	UPROPERTY()
	TArray<FString> Consequences;

	UPROPERTY()
	TArray<FString> ClueUnlocks;

	UPROPERTY()
	TMap<FString, int32> RelationshipChanges;

	UPROPERTY()
	FHDIDSoulValues SoulChanges;

	UPROPERTY()
	FString NextNode;

	UPROPERTY()
	FString Camera;

	/** True only for unimplemented reveal lines. Chapter 01 must keep this false. */
	UPROPERTY()
	bool Reveal = false;
};

USTRUCT(BlueprintType)
struct FHDIDConsequenceDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	TMap<FString, FString> SetFlags;

	UPROPERTY()
	FHDIDSoulValues Soul;

	UPROPERTY()
	TArray<FString> RevealClues;

	UPROPERTY()
	TArray<FString> UnlockTimeline;

	UPROPERTY()
	TMap<FString, int32> Relationship;

	UPROPERTY()
	bool HelpedNpc = false;

	UPROPERTY()
	bool OptionalDialogue = false;

	UPROPERTY()
	FString NotebookFact;
};

USTRUCT(BlueprintType)
struct FHDIDClueDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString ClueID;

	UPROPERTY()
	FString ChapterID;

	UPROPERTY()
	FString CardID;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	FString Location;

	UPROPERTY()
	FString DiscoveryMethod;

	UPROPERTY()
	TArray<FString> RelatedCharacters;

	UPROPERTY()
	TArray<FString> RelatedEvents;

	UPROPERTY()
	TArray<FString> RelatedClues;

	UPROPERTY()
	FString MemoryReference;

	UPROPERTY()
	FString Importance;

	UPROPERTY()
	bool Optional = false;

	UPROPERTY()
	bool Hidden = false;

	UPROPERTY()
	bool BranchExclusive = false;

	UPROPERTY()
	bool CrossChapter = false;

	UPROPERTY()
	FString CrossTarget;

	UPROPERTY()
	TArray<FHDIDCondition> ActiveIf;

	UPROPERTY()
	FString GrantFlag;

	UPROPERTY()
	FString GrantValue;

	UPROPERTY()
	TArray<FString> Categories;

	UPROPERTY()
	FHDIDSoulValues SoulOnDiscover;
};

USTRUCT(BlueprintType)
struct FHDIDMemoryDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString MemoryID;

	UPROPERTY()
	FString ChapterID;

	UPROPERTY()
	FString CardID;

	UPROPERTY()
	FString QuestionAnswered;

	UPROPERTY()
	FString QuestionOpened;

	UPROPERTY()
	FString RequiredClue;

	UPROPERTY()
	FString TimelineEventID;

	UPROPERTY()
	FString EntryLocationId;

	UPROPERTY()
	FHDIDVec3 EntryPosition;

	UPROPERTY()
	FString HoldLine;

	UPROPERTY()
	bool Playable = false;
};

USTRUCT(BlueprintType)
struct FHDIDTimelineBranch
{
	GENERATED_BODY()

	UPROPERTY()
	FString ChoiceID;

	UPROPERTY()
	FString Label;
};

USTRUCT(BlueprintType)
struct FHDIDTimelineEvent
{
	GENERATED_BODY()

	UPROPERTY()
	FString EventID;

	UPROPERTY()
	FString ChapterID;

	UPROPERTY()
	FString Time;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString LocationId;

	UPROPERTY()
	FHDIDVec3 Position;

	UPROPERTY()
	bool StartsLocked = true;

	UPROPERTY()
	TArray<FHDIDTimelineBranch> Branches;

	UPROPERTY()
	FString UnlockOnClue;
};

USTRUCT(BlueprintType)
struct FHDIDInteractableDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString LocationId;

	UPROPERTY()
	FString Kind;

	UPROPERTY()
	FString Prompt;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString InspectText;

	UPROPERTY()
	FString ClueID;

	UPROPERTY()
	FString DialogueID;

	UPROPERTY()
	FString MemoryID;

	UPROPERTY()
	FString TimelineEventID;

	UPROPERTY()
	FHDIDVec3 Position;

	UPROPERTY()
	FHDIDVec3 Scale;

	UPROPERTY()
	FString Mesh = TEXT("Cube");

	UPROPERTY()
	FString Color = TEXT("1A1C22");

	UPROPERTY()
	TArray<FHDIDCondition> PresentIf;

	UPROPERTY()
	TArray<FHDIDCondition> AbsentIf;

	UPROPERTY()
	FString RequiredCard;

	UPROPERTY()
	bool Hidden = false;

	UPROPERTY()
	bool BlocksPlayer = false;

	UPROPERTY()
	bool OnlyInFlashback = false;

	UPROPERTY()
	FString FlashbackMemoryID;

	UPROPERTY()
	FString TraceStyle;

	/** Empty, or ReturnMemory. Generic verb, not a chapter id. */
	UPROPERTY()
	FString Action;
};

USTRUCT(BlueprintType)
struct FHDIDChapterFile
{
	GENERATED_BODY()

	UPROPERTY()
	FString ChapterID;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Logline;

	UPROPERTY()
	bool Playable = false;

	UPROPERTY()
	FString ProtagonistId;

	UPROPERTY()
	FString Theme;

	UPROPERTY()
	float ColourSaturation = 0.1f;

	UPROPERTY()
	FString Clock;

	UPROPERTY()
	FString CoinBeat;

	UPROPERTY()
	FString HourglassBeat;

	UPROPERTY()
	FString StrangerStage;

	UPROPERTY()
	TArray<FHDIDCrossRef> CrossChapterRefs;

	UPROPERTY()
	TArray<FHDIDLocationDef> Locations;

	UPROPERTY()
	TArray<FHDIDCardDef> Cards;

	UPROPERTY()
	TArray<FHDIDClueDef> Clues;

	UPROPERTY()
	TArray<FHDIDDialogueNode> Dialogue;

	UPROPERTY()
	TArray<FHDIDConsequenceDef> Consequences;

	UPROPERTY()
	TArray<FHDIDMemoryDef> Memories;

	UPROPERTY()
	TArray<FHDIDTimelineEvent> Timeline;

	UPROPERTY()
	TArray<FHDIDInteractableDef> Interactables;

	UPROPERTY()
	FString MusicIdentity;

	UPROPERTY()
	TArray<FString> MusicLayers;
};

USTRUCT(BlueprintType)
struct FHDIDGameText
{
	GENERATED_BODY()

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Tagline;

	UPROPERTY()
	FString MenuClockFallback = TEXT("23:47");
};

USTRUCT(BlueprintType)
struct FHDIDMusicFile
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> Layers;

	UPROPERTY()
	TMap<FString, FString> Identities;

	UPROPERTY()
	FString StrangerMotifFull;
};

USTRUCT(BlueprintType)
struct FHDIDKeyRebind
{
	GENERATED_BODY()

	UPROPERTY()
	FString SlotId;

	UPROPERTY()
	FString KeyName;
};

USTRUCT(BlueprintType)
struct FHDIDNotebookEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FString Category;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Body;

	UPROPERTY()
	FString SourceId;
};

USTRUCT()
struct FHDIDCharacterList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FHDIDCharacterDef> Characters;
};

USTRUCT()
struct FHDIDEndingDef
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Meaning;
};

/** Late-game canon stub. Playable stays false until steps 16–19. */
USTRUCT()
struct FHDIDLateGameFile
{
	GENERATED_BODY()

	UPROPERTY()
	bool Playable = false;

	UPROPERTY()
	TArray<int32> ImplementSteps;

	UPROPERTY()
	FString GrimReaperIdentity;

	UPROPERTY()
	bool GrimReaperIsVillain = false;

	UPROPERTY()
	TArray<FString> GrimReaperLines;

	UPROPERTY()
	FString VoidName;

	UPROPERTY()
	FString VoidPolicy;

	UPROPERTY()
	TArray<FHDIDEndingDef> Endings;

	UPROPERTY()
	FString Soul11Title;

	UPROPERTY()
	FString Soul11Then;

	UPROPERTY()
	int32 Soul11Chairs = 0;

	UPROPERTY()
	FString Soul11CoinPlacedBy;

	UPROPERTY()
	TArray<FString> FinalMessage;

	UPROPERTY()
	FString AwakeningLine;
};
