#include "Interaction/HDIDInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "Core/HDIDPlayerController.h"
#include "Core/HDIDUtil.h"
#include "Data/HDIDCatalog.h"
#include "Data/HDIDTypes.h"
#include "Engine/StaticMesh.h"
#include "HowDidIDie.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Systems/HDIDManagers.h"

namespace
{
	UStaticMesh* LoadBasicMesh(const FString& Name)
	{
		if (Name == TEXT("Cylinder"))
		{
			return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		}
		if (Name == TEXT("Sphere"))
		{
			return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		}
		if (Name == TEXT("Plane"))
		{
			return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		}
		return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	}

	bool TextIsLockedReveal(const FString& Text)
	{
		const FString Lower = Text.ToLower();
		return Lower.Contains(TEXT("grim reaper")) || Lower.Contains(TEXT("the void")) || Lower.Contains(TEXT("soul 11")) || Lower.Contains(TEXT("you died of"));
	}
}

AHDIDInteractable::AHDIDInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AHDIDInteractable::ApplyDef(const FHDIDInteractableDef& Def)
{
	DefId = Def.Id;
	Prompt = Def.Prompt.IsEmpty() ? TEXT("E \u2014 INSPECT") : Def.Prompt;
	Title = Def.Title;
	InspectText = Def.InspectText;
	ClueId = Def.ClueID;
	DialogueId = Def.DialogueID;
	MemoryId = Def.MemoryID;
	Action = Def.Action;
	bCloseRange = Def.Hidden;
	SetActorLocation(Def.Position.ToUE());
	BuildMesh(Def);
}

void AHDIDInteractable::BuildMesh(const FHDIDInteractableDef& Def)
{
	if (!Mesh)
	{
		return;
	}
	if (UStaticMesh* Shape = LoadBasicMesh(Def.Mesh))
	{
		Mesh->SetStaticMesh(Shape);
	}
	Mesh->SetWorldScale3D(HDIDScaleOrOne(Def.Scale));
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this))
		{
			const FLinearColor Color = HDIDHexColor(Def.Color);
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			Mesh->SetMaterial(0, MID);
		}
	}
	if (Def.BlocksPlayer)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
}

void AHDIDInteractable::Interact(AHDIDPlayerController* PC)
{
	OnInteract(PC);
}

void AHDIDInteractable::FocusInspect(AHDIDPlayerController* PC, bool bHeld)
{
	if (!PC)
	{
		return;
	}
	PC->ShowExamine(Title, InspectText, bHeld);
}

void AHDIDInteractable::OnInteract(AHDIDPlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}
	if (UHDIDStoryCardManager* Cards = GameInstance->GetSubsystem<UHDIDStoryCardManager>())
	{
		Cards->NotifyInspect(DefId);
	}
	if (!ClueId.IsEmpty())
	{
		if (UHDIDInvestigationManager* Investigation = GameInstance->GetSubsystem<UHDIDInvestigationManager>())
		{
			Investigation->DiscoverClue(ClueId);
		}
	}
	if (Action == TEXT("ReturnMemory"))
	{
		if (UHDIDFlashbackManager* Flashback = GameInstance->GetSubsystem<UHDIDFlashbackManager>())
		{
			Flashback->TryReturn();
		}
		return;
	}
	if (!DialogueId.IsEmpty())
	{
		if (UHDIDDialogueManager* Dialogue = GameInstance->GetSubsystem<UHDIDDialogueManager>())
		{
			Dialogue->StartDialogue(DialogueId);
		}
	}
	if (!MemoryId.IsEmpty())
	{
		if (UHDIDFlashbackManager* Flashback = GameInstance->GetSubsystem<UHDIDFlashbackManager>())
		{
			Flashback->BeginMemory(MemoryId, this);
		}
	}
	PC->PulseHaptics();
}

AHDIDGrimReaper::AHDIDGrimReaper()
{
	// No face, no eyes, no name. Chapter 01 only ever sees this silhouette.
}

void AHDIDGrimReaper::OnInteract(AHDIDPlayerController* PC)
{
	UGameInstance* GameInstance = GetGameInstance();
	UHDIDCatalogSubsystem* Catalog = GameInstance ? GameInstance->GetSubsystem<UHDIDCatalogSubsystem>() : nullptr;
	if (Catalog && !DialogueId.IsEmpty())
	{
		if (const FHDIDDialogueNode* Node = Catalog->FindDialogue(DialogueId))
		{
			if (Node->Reveal || !Catalog->IsRevealImplemented() && TextIsLockedReveal(Node->Text))
			{
				UE_LOG(LogHDID, Warning, TEXT("Stranger refused a locked reveal line."));
				return;
			}
			if (TextIsLockedReveal(Node->Text))
			{
				UE_LOG(LogHDID, Warning, TEXT("Stranger refused a line that names the locked identity."));
				return;
			}
		}
	}
	if (Catalog && !Catalog->IsRevealImplemented() && CanDeliverFinalReveal())
	{
		return;
	}
	Super::OnInteract(PC);
}

AHDIDVoidTrace::AHDIDVoidTrace()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AHDIDVoidTrace::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Mesh)
	{
		return;
	}
	if (BaseScale.IsNearlyZero())
	{
		BaseScale = Mesh->GetComponentScale();
	}
	Phase += DeltaSeconds;
	const float Flicker = 1.f + 0.04f * FMath::Sin(Phase * 9.f);
	Mesh->SetWorldScale3D(BaseScale * Flicker);
}
