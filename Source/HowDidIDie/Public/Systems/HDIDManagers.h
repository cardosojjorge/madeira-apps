#pragma once

#include "CoreMinimal.h"
#include "Data/HDIDTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HDIDManagers.generated.h"

class AActor;
class AHDIDPlayerCharacter;
class UHDIDUIManager;

DECLARE_DELEGATE(FHDIDSimpleEvent);

/** Intended Blueprint wrapper: BP_ConsequenceManager. Rebuilds flags from the current choices. */
UCLASS()
class HOWDIDIDIE_API UHDIDConsequenceManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Rebuild();
	TMap<FString, FString> GetFlags() const;
	bool ConditionsMet(const TArray<FHDIDCondition>& Conditions) const;
	bool AnyConditionMet(const TArray<FHDIDCondition>& Conditions) const;
	void GetActiveConsequences(TArray<const FHDIDConsequenceDef*>& Out) const;

private:
	TMap<FString, FString> DerivedFlags;
	void ApplyConsequence(const FHDIDConsequenceDef& Consequence, TMap<FString, int32>& Relationships);
};

/** Intended Blueprint wrapper: BP_SoulEvaluationManager. Values stay off the notebook. */
UCLASS()
class HOWDIDIDIE_API UHDIDSoulEvaluationManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Recompute();
};

/** Intended Blueprint wrapper: BP_InvestigationManager. Stores evidence. Does not state conclusions. */
UCLASS()
class HOWDIDIDIE_API UHDIDInvestigationManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void DiscoverClue(const FString& ClueId);
	bool IsClueDiscovered(const FString& ClueId) const;
	bool IsClueActive(const FString& ClueId) const;
	void NotifySpeaker(const FString& SpeakerId);
	void NotifyLocation(const FString& LocationId);
	void BuildNotebook(TArray<FHDIDNotebookEntry>& OutEntries) const;
};

/** Intended Blueprint wrapper: BP_ChoiceManager. */
UCLASS()
class HOWDIDIDIE_API UHDIDChoiceManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool RequirementsMet(const FHDIDChoiceDef& Choice) const;
	bool Commit(const FString& ChoiceId, bool bAdvanceCard);
};

/** Intended Blueprint wrapper: BP_DialogueManager. */
UCLASS()
class HOWDIDIDIE_API UHDIDDialogueManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void StartDialogue(const FString& DialogueId);
	void Choose(int32 Index);
	void Advance();
	void Close();
	bool IsOpen() const { return bOpen; }
	const TArray<FHDIDChoiceDef>& GetVisibleChoices() const { return VisibleChoices; }

private:
	void OpenNode(const FHDIDDialogueNode& Node);

	bool bOpen = false;
	FString CurrentDialogueId;
	TArray<FHDIDChoiceDef> VisibleChoices;
};

/** Intended Blueprint wrapper: BP_StoryCardManager. */
UCLASS()
class HOWDIDIDIE_API UHDIDStoryCardManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void BeginSession();
	void NotifyInspect(const FString& InteractableId);
	void NotifyDiscover(const FString& ClueId);
	void NotifyDialogue(const FString& DialogueId);
	void NotifyCommit();
	void NotifyFlashback(const FString& MemoryId);
	void NotifyTimelineOpened();
	void NotifyEnterLocation(const FString& LocationId);

	const FHDIDCardDef* GetCurrentCard() const;
	FString GetIndexLabel() const;
	FString GetTitle() const;
	FString GetBody() const;
	FString GetCameraMode() const;
	bool IsCardReached(const FString& CardId) const;

private:
	void TryAdvance();
	bool IsObjectiveMet(const FString& Objective) const;
	bool AreObjectivesMet(const FHDIDCardDef& Card) const;
	void Publish() const;
	void CompleteChapter();

	bool bCompletionAnnounced = false;
};

/** Intended Blueprint wrapper: BP_TimelineManager. */
UCLASS()
class HOWDIDIDIE_API UHDIDTimelineManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SyncFromSave();
	void UnlockEvent(const FString& EventId);
	void UnlockFromClue(const FString& ClueId);
	bool IsUnlocked(const FString& EventId) const;
	bool TravelTo(const FString& EventId);
	FString GetPrimaryBranchEventId() const;
};

/** Intended Blueprint wrapper: BP_FlashbackManager. */
UCLASS()
class HOWDIDIDIE_API UHDIDFlashbackManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool IsActive() const { return bActive; }
	bool IsBusy() const { return bBusy; }
	void BeginMemory(const FString& MemoryId, AActor* ZoomTarget);
	void TryReturn();
	void RestoreFromSave();
	const FString& GetActiveMemoryId() const { return ActiveMemoryId; }

private:
	void FinishEnter(const FString& MemoryId);
	void FinishExit();

	bool bActive = false;
	bool bBusy = false;
	FString ActiveMemoryId;
	FTransform SavedTransform = FTransform::Identity;
	FRotator SavedControl = FRotator::ZeroRotator;
};

/** Intended Blueprint wrapper: BP_AudioManager. MetaSound beds are named in data; no wave assets are authored. */
UCLASS()
class HOWDIDIDIE_API UHDIDAudioManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void ApplySettings();
	void ApplyCard(const FHDIDCardDef* Card);
	void Pulse(const FString& Layer);
	void PlayReverseMoment();

	float LayerGain(const FString& Layer) const;

private:
	TArray<FString> ActiveLayers;
	float ReverseAlpha = 1.f;
};

/** Intended Blueprint wrapper: BP_CinematicManager. Sequencer is the later wrapper; this is the greybox transition. */
UCLASS()
class HOWDIDIDIE_API UHDIDCinematicManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void EnterMemory(AActor* ZoomTarget, FHDIDSimpleEvent Finished);
	void ExitMemory(FHDIDSimpleEvent Finished);
	bool IsBusy() const { return bBusy; }

private:
	void Step();
	void Finish();

	bool bBusy = false;
	bool bEntering = false;
	int32 Stage = 0;
	FHDIDSimpleEvent Pending;
	TWeakObjectPtr<AActor> ZoomTarget;
	FTimerHandle StepTimer;
};
