#pragma once

#include "CoreMinimal.h"
#include "Data/HDIDTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HDIDCatalog.generated.h"

/** Loads Content/Data at startup. A new chapter is a new JSON file in Content/Data/Chapters. */
UCLASS()
class HOWDIDIDIE_API UHDIDCatalogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	bool IsReady() const { return bReady; }
	const TArray<FString>& GetLoadErrors() const { return LoadErrors; }

	const FHDIDGameText& GetGameText() const { return GameText; }
	const FHDIDMusicFile& GetMusic() const { return Music; }
	const FHDIDLateGameFile& GetLateGame() const { return LateGame; }

	/** Steps 16–19 are not implemented. Data may describe them; nothing may start them. */
	bool IsRevealImplemented() const { return false; }
	bool IsLateGamePlayable() const { return false; }

	TArray<FString> GetChapterIds() const;
	const FHDIDChapterFile* FindChapter(const FString& ChapterId) const;
	bool IsPlayable(const FString& ChapterId) const;
	FString GetFirstPlayableChapterId() const;

	const FHDIDCharacterDef* FindCharacter(const FString& CharacterId) const;
	const FHDIDCardDef* FindCard(const FString& CardId) const;
	const FHDIDClueDef* FindClue(const FString& ClueId) const;
	const FHDIDDialogueNode* FindDialogue(const FString& DialogueId) const;
	const FHDIDChoiceDef* FindChoice(const FString& ChoiceId) const;
	const FHDIDConsequenceDef* FindConsequence(const FString& ConsequenceId) const;
	const FHDIDMemoryDef* FindMemory(const FString& MemoryId) const;
	const FHDIDTimelineEvent* FindEvent(const FString& EventId) const;
	const FHDIDLocationDef* FindLocation(const FString& LocationId) const;
	int32 GetCardIndex(const FString& CardId) const;
	int32 GetCardCount(const FString& ChapterId) const;

	FString GetChapterForCard(const FString& CardId) const;

private:
	UPROPERTY()
	FHDIDGameText GameText;

	UPROPERTY()
	FHDIDMusicFile Music;

	UPROPERTY()
	FHDIDLateGameFile LateGame;

	UPROPERTY()
	TMap<FString, FHDIDCharacterDef> Characters;

	UPROPERTY()
	TMap<FString, FHDIDChapterFile> Chapters;

	UPROPERTY()
	TArray<FString> ChapterOrder;

	TMap<FString, FHDIDChoiceDef> Choices;

	bool bReady = false;
	TArray<FString> LoadErrors;
};
