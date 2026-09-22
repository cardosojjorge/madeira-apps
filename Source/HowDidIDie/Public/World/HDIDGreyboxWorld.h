#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HDIDGreyboxWorld.generated.h"

class AHDIDInteractable;

/** Runtime greybox. Rooms, props, and interactables are spawned from chapter JSON. No .umap. */
UCLASS()
class HOWDIDIDIE_API AHDIDGreyboxWorld : public AActor
{
	GENERATED_BODY()

public:
	AHDIDGreyboxWorld();

	void BuildChapter(const FString& ChapterId);
	void RefreshInteractables();
	void ClearWorld();
	FTransform GetSpawnTransform() const { return SpawnTransform; }
	bool IsCorridorAt(const FVector& Point) const;
	FString GetLocationIdAt(const FVector& Point) const;
	AActor* FindSpawnedInteractable(const FString& Id) const;

private:
	void SpawnAtmosphere();
	void SpawnLocation(const struct FHDIDLocationDef& Location);
	void SpawnProp(const struct FHDIDPropDef& Prop);
	void SpawnWall(const FVector& Center, const FVector& Scale);
	UStaticMesh* BasicMesh(const FString& Name) const;
	void Paint(class UStaticMeshComponent* Mesh, const FString& Hex, bool bBlockPawn, bool bBlockCamera);

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> SpawnedMeshes;

	TMap<FString, TWeakObjectPtr<AHDIDInteractable>> InteractableById;
	FString BuiltChapterId;
	FTransform SpawnTransform = FTransform::Identity;
	bool bAtmosphere = false;
};
