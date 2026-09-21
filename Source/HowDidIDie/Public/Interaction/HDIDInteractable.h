#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/HDIDTypes.h"
#include "HDIDInteractable.generated.h"

class UStaticMeshComponent;
class AHDIDPlayerController;

/** Intended Blueprint wrapper: BP_Interactable. Behaviour comes from data, not from the chapter id. */
UCLASS()
class HOWDIDIDIE_API AHDIDInteractable : public AActor
{
	GENERATED_BODY()

public:
	AHDIDInteractable();

	void ApplyDef(const FHDIDInteractableDef& Def);
	void Interact(AHDIDPlayerController* PC);
	void FocusInspect(AHDIDPlayerController* PC, bool bHeld);

	const FString& GetPromptText() const { return Prompt; }
	const FString& GetDefId() const { return DefId; }
	bool UsesCloseRange() const { return bCloseRange; }
	bool HasMemory() const { return !MemoryId.IsEmpty(); }
	const FString& GetMemoryId() const { return MemoryId; }
	const FString& GetTitleText() const { return Title; }

protected:
	virtual void OnInteract(AHDIDPlayerController* PC);
	void BuildMesh(const FHDIDInteractableDef& Def);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	FString DefId;
	FString Prompt;
	FString Title;
	FString InspectText;
	FString ClueId;
	FString DialogueId;
	FString MemoryId;
	FString Action;
	bool bCloseRange = false;
};

/** Intended Blueprint wrapper: BP_Clue. */
UCLASS()
class HOWDIDIDIE_API AHDIDClue : public AHDIDInteractable
{
	GENERATED_BODY()
};

/** Intended Blueprint wrapper: BP_MemoryTrigger. */
UCLASS()
class HOWDIDIDIE_API AHDIDMemoryTrigger : public AHDIDInteractable
{
	GENERATED_BODY()
};

UCLASS()
class HOWDIDIDIE_API AHDIDDialogueTrigger : public AHDIDInteractable
{
	GENERATED_BODY()
};

UCLASS()
class HOWDIDIDIE_API AHDIDStoryObject : public AHDIDInteractable
{
	GENERATED_BODY()
};

UCLASS()
class HOWDIDIDIE_API AHDIDTimelineTrigger : public AHDIDInteractable
{
	GENERATED_BODY()
};

UCLASS()
class HOWDIDIDIE_API AHDIDCharacterInteraction : public AHDIDInteractable
{
	GENERATED_BODY()
};

/** Intended Blueprint wrapper: BP_GrimReaper. Silhouette only. Reveal lines are refused. */
UCLASS()
class HOWDIDIDIE_API AHDIDGrimReaper : public AHDIDInteractable
{
	GENERATED_BODY()

public:
	AHDIDGrimReaper();
	virtual void OnInteract(AHDIDPlayerController* PC) override;
	bool CanDeliverFinalReveal() const { return false; }
};

/** Intended Blueprint wrapper: BP_Void. Early traces only. It never names itself. */
UCLASS()
class HOWDIDIDIE_API AHDIDVoidTrace : public AHDIDInteractable
{
	GENERATED_BODY()

public:
	AHDIDVoidTrace();
	virtual void Tick(float DeltaSeconds) override;

private:
	float Phase = 0.f;
	FVector BaseScale = FVector::OneVector;
};
