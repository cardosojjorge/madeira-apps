#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Data/HDIDTypes.h"
#include "HDIDSaveGame.generated.h"

/**
 * Story slot. Spec section 30.
 * Intended Blueprint wrapper: BP_SaveManager (UHDIDSaveManager).
 *
 * CurrentChapterId, CurrentCardId, ProtagonistId,
 * TimelineVisited, TimelineUnlocked, ChoiceByBranch, OneShotChoices,
 * CluesFound, MemoriesFound, Relationships, CrossChapterDiscoveries,
 * Soul, Dedication, EndingsUnlocked, UnlockedContent,
 * plus the transform and progress sets needed to resume the same hour.
 */
UCLASS()
class HOWDIDIDIE_API UHDIDSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString CurrentChapterId;

	UPROPERTY()
	FString CurrentCardId;

	UPROPERTY()
	FString ProtagonistId;

	UPROPERTY()
	TArray<FString> TimelineVisited;

	UPROPERTY()
	TArray<FString> TimelineUnlocked;

	UPROPERTY()
	TMap<FString, FString> ChoiceByBranch;

	UPROPERTY()
	TArray<FString> OneShotChoices;

	UPROPERTY()
	TArray<FString> CluesFound;

	UPROPERTY()
	TArray<FString> MemoriesFound;

	UPROPERTY()
	TMap<FString, int32> Relationships;

	UPROPERTY()
	TArray<FString> CrossChapterDiscoveries;

	UPROPERTY()
	FHDIDSoulValues Soul;

	UPROPERTY()
	FHDIDDedicationValues Dedication;

	UPROPERTY()
	TArray<FString> EndingsUnlocked;

	UPROPERTY()
	TArray<FString> UnlockedContent;

	UPROPERTY()
	FVector SavedLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator SavedRotation = FRotator::ZeroRotator;

	UPROPERTY()
	bool bHasSavedTransform = false;

	UPROPERTY()
	FString ActiveMemoryId;

	UPROPERTY()
	TMap<FString, FString> BaseFlags;

	UPROPERTY()
	TArray<FString> CardsReached;

	UPROPERTY()
	TArray<FString> SpeakersHeard;

	UPROPERTY()
	TArray<FString> LocationsEntered;

	UPROPERTY()
	TArray<FString> InspectedIds;

	UPROPERTY()
	TArray<FString> DialoguesCompleted;

	UPROPERTY()
	bool bTimelineOpened = false;

	UPROPERTY()
	int32 BranchRevisionCount = 0;

	UPROPERTY()
	bool bChapterComplete = false;
};

/** Settings from spec sections 32–33. Separate from the story slot. */
UCLASS()
class HOWDIDIDIE_API UHDIDSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float MasterVolume = 0.8f;

	UPROPERTY()
	float MusicVolume = 0.7f;

	UPROPERTY()
	float EffectsVolume = 0.8f;

	UPROPERTY()
	float DialogueVolume = 1.f;

	UPROPERTY()
	bool bSubtitles = true;

	UPROPERTY()
	float SubtitleScale = 1.f;

	UPROPERTY()
	FString GraphicsPreset = TEXT("High");

	UPROPERTY()
	int32 ResolutionX = 1920;

	UPROPERTY()
	int32 ResolutionY = 1080;

	UPROPERTY()
	FString WindowMode = TEXT("Windowed");

	UPROPERTY()
	float MouseSensitivity = 1.f;

	UPROPERTY()
	float ControllerSensitivity = 1.f;

	UPROPERTY()
	bool bVibration = true;

	UPROPERTY()
	bool bMotionReduction = false;

	UPROPERTY()
	bool bHighContrast = false;

	UPROPERTY()
	TArray<FHDIDKeyRebind> Rebinds;
};

UCLASS()
class HOWDIDIDIE_API UHDIDSaveManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* StorySlotName() { return TEXT("HDID_Story"); }
	static const TCHAR* SettingsSlotName() { return TEXT("HDID_Settings"); }

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UHDIDSaveGame* GetStory() const { return Story; }
	UHDIDSettingsSave* GetSettings() const { return Settings; }

	bool HasStorySave() const;
	void NewGame(const FString& ChapterId);
	bool LoadStory();
	void WriteStory();
	void WriteSettings();

	void SetBaseFlag(const FString& Flag, const FString& Value);
	void RememberTransform(const FVector& Location, const FRotator& Rotation);

	static const TArray<FString>& RequiredStoryProperties();

private:
	UPROPERTY()
	TObjectPtr<UHDIDSaveGame> Story;

	UPROPERTY()
	TObjectPtr<UHDIDSettingsSave> Settings;
};
