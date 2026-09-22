#include "World/HDIDGreyboxWorld.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HDIDUtil.h"
#include "Data/HDIDCatalog.h"
#include "Data/HDIDTypes.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "HowDidIDie.h"
#include "Interaction/HDIDInteractable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Save/HDIDSaveGame.h"
#include "Systems/HDIDManagers.h"

AHDIDGreyboxWorld::AHDIDGreyboxWorld()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AHDIDGreyboxWorld::ClearWorld()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	for (UStaticMeshComponent* Mesh : SpawnedMeshes)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}
	SpawnedActors.Reset();
	SpawnedMeshes.Reset();
	InteractableById.Reset();
	BuiltChapterId.Reset();
}

void AHDIDGreyboxWorld::BuildChapter(const FString& ChapterId)
{
	ClearWorld();
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDChapterFile* Chapter = Catalog ? Catalog->FindChapter(ChapterId) : nullptr;
	if (!Chapter)
	{
		UE_LOG(LogHDID, Error, TEXT("No chapter data for %s"), *ChapterId);
		return;
	}
	BuiltChapterId = ChapterId;
	SpawnAtmosphere();
	for (const FHDIDLocationDef& Location : Chapter->Locations)
	{
		SpawnLocation(Location);
	}
	if (Chapter->Cards.Num() > 0)
	{
		if (const FHDIDLocationDef* SpawnLoc = Catalog->FindLocation(Chapter->Cards[0].Location))
		{
			SpawnTransform = FTransform(FRotator(0.f, SpawnLoc->SpawnYaw, 0.f), SpawnLoc->Spawn.ToUE());
		}
	}
	RefreshInteractables();
}

void AHDIDGreyboxWorld::SpawnAtmosphere()
{
	UWorld* World = GetWorld();
	if (!World || bAtmosphere)
	{
		return;
	}
	bAtmosphere = true;
	if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 800.f), FRotator(-48.f, 28.f, 0.f)))
	{
		Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Sun->GetLightComponent()->SetIntensity(3.2f);
		Sun->GetLightComponent()->SetLightColor(FLinearColor(0.62f, 0.72f, 0.86f));
	}
	if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator))
	{
		Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
		Sky->GetLightComponent()->SetIntensity(0.45f);
		if (USkyLightComponent* SkyComp = Cast<USkyLightComponent>(Sky->GetLightComponent()))
		{
			SkyComp->RecaptureSky();
		}
	}
	if (AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator))
	{
		Fog->GetComponent()->SetFogDensity(0.018f);
		Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.015f, 0.02f, 0.03f));
	}
}

UStaticMesh* AHDIDGreyboxWorld::BasicMesh(const FString& Name) const
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

void AHDIDGreyboxWorld::Paint(UStaticMeshComponent* MeshComp, const FString& Hex, bool bBlockPawn, bool bBlockCamera)
{
	if (!MeshComp)
	{
		return;
	}
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, MeshComp))
		{
			MID->SetVectorParameterValue(TEXT("Color"), HDIDHexColor(Hex));
			MeshComp->SetMaterial(0, MID);
		}
	}
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	if (bBlockPawn)
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	}
	if (bBlockCamera && !bBlockPawn)
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	}
	if (!bBlockPawn && !bBlockCamera)
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AHDIDGreyboxWorld::SpawnWall(const FVector& Center, const FVector& Scale)
{
	UStaticMeshComponent* Wall = NewObject<UStaticMeshComponent>(this);
	Wall->SetStaticMesh(BasicMesh(TEXT("Cube")));
	Wall->SetupAttachment(GetRootComponent());
	Wall->RegisterComponent();
	Wall->SetWorldLocation(Center);
	Wall->SetWorldScale3D(Scale);
	Paint(Wall, TEXT("12141A"), true, true);
	SpawnedMeshes.Add(Wall);
}

void AHDIDGreyboxWorld::SpawnProp(const FHDIDPropDef& Prop)
{
	UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this);
	MeshComp->SetStaticMesh(BasicMesh(Prop.Mesh));
	MeshComp->SetupAttachment(GetRootComponent());
	MeshComp->RegisterComponent();
	MeshComp->SetWorldLocation(Prop.Position.ToUE());
	MeshComp->SetWorldScale3D(HDIDScaleOrOne(Prop.Scale));
	Paint(MeshComp, Prop.Color, Prop.BlocksPlayer, false);
	SpawnedMeshes.Add(MeshComp);
}

void AHDIDGreyboxWorld::SpawnLocation(const FHDIDLocationDef& Location)
{
	const FVector Origin = Location.Origin.ToUE();
	const float Height = Location.Height > 0.f ? Location.Height : 400.f;
	UStaticMeshComponent* Floor = NewObject<UStaticMeshComponent>(this);
	Floor->SetStaticMesh(BasicMesh(TEXT("Cube")));
	Floor->SetupAttachment(GetRootComponent());
	Floor->RegisterComponent();
	Floor->SetWorldLocation(Origin + FVector(Location.SizeX * 0.5f, Location.SizeY * 0.5f, -10.f));
	Floor->SetWorldScale3D(FVector(Location.SizeX / 100.f, Location.SizeY / 100.f, 0.2f));
	Paint(Floor, TEXT("0C0E12"), true, true);
	SpawnedMeshes.Add(Floor);

	UStaticMeshComponent* Ceiling = NewObject<UStaticMeshComponent>(this);
	Ceiling->SetStaticMesh(BasicMesh(TEXT("Cube")));
	Ceiling->SetupAttachment(GetRootComponent());
	Ceiling->RegisterComponent();
	Ceiling->SetWorldLocation(Origin + FVector(Location.SizeX * 0.5f, Location.SizeY * 0.5f, Height + 8.f));
	Ceiling->SetWorldScale3D(FVector(Location.SizeX / 100.f, Location.SizeY / 100.f, 0.08f));
	Paint(Ceiling, TEXT("08090C"), false, false);
	SpawnedMeshes.Add(Ceiling);

	auto WallRun = [&](bool bAlongX, bool bOpen, float Fixed, bool bPositive)
	{
		const float Length = bAlongX ? Location.SizeX : Location.SizeY;
		const float Thickness = 20.f;
		const float CenterFixed = Fixed;
		auto Place = [&](float AlongCenter, float AlongLength)
		{
			if (AlongLength < 8.f)
			{
				return;
			}
			FVector Center = Origin;
			FVector Scale(Thickness / 100.f, Thickness / 100.f, Height / 100.f);
			if (bAlongX)
			{
				Center.X += AlongCenter;
				Center.Y += CenterFixed;
				Scale.X = AlongLength / 100.f;
			}
			else
			{
				Center.Y += AlongCenter;
				Center.X += CenterFixed;
				Scale.Y = AlongLength / 100.f;
			}
			Center.Z = Height * 0.5f;
			SpawnWall(Center, Scale);
		};
		if (!bOpen)
		{
			Place(Length * 0.5f, Length);
			return;
		}
		const float Gap = 180.f;
		const float Side = (Length - Gap) * 0.5f;
		Place(Side * 0.5f, Side);
		Place(Length - Side * 0.5f, Side);
		(void)bPositive;
	};

	WallRun(true, Location.OpenSouth, 0.f, false);
	WallRun(true, Location.OpenNorth, Location.SizeY, true);
	WallRun(false, Location.OpenWest, 0.f, false);
	WallRun(false, Location.OpenEast, Location.SizeX, true);

	for (const FHDIDPropDef& Prop : Location.Props)
	{
		SpawnProp(Prop);
	}
	for (const FHDIDLightDef& LightDef : Location.Lights)
	{
		if (APointLight* Light = GetWorld()->SpawnActor<APointLight>(LightDef.Position.ToUE(), FRotator::ZeroRotator))
		{
			Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			Light->GetLightComponent()->SetIntensity(LightDef.Intensity);
			Light->GetLightComponent()->SetLightColor(HDIDHexColor(LightDef.Color));
			if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light->GetLightComponent()))
			{
				Point->SetAttenuationRadius(LightDef.Radius);
			}
			SpawnedActors.Add(Light);
		}
	}
}

void AHDIDGreyboxWorld::RefreshInteractables()
{
	for (TPair<FString, TWeakObjectPtr<AHDIDInteractable>>& Pair : InteractableById)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Destroy();
		}
	}
	InteractableById.Reset();

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}
	UHDIDCatalogSubsystem* Catalog = GI->GetSubsystem<UHDIDCatalogSubsystem>();
	UHDIDSaveManager* Save = GI->GetSubsystem<UHDIDSaveManager>();
	UHDIDConsequenceManager* Consequence = GI->GetSubsystem<UHDIDConsequenceManager>();
	UHDIDStoryCardManager* Cards = GI->GetSubsystem<UHDIDStoryCardManager>();
	UHDIDFlashbackManager* Flashback = GI->GetSubsystem<UHDIDFlashbackManager>();
	const FHDIDChapterFile* Chapter = Catalog ? Catalog->FindChapter(BuiltChapterId) : nullptr;
	if (!Chapter || !Save || !Save->GetStory() || !Consequence)
	{
		return;
	}
	const TMap<FString, FString> Flags = Consequence->GetFlags();
	const FString ActiveMemory = Flashback ? Flashback->GetActiveMemoryId() : Save->GetStory()->ActiveMemoryId;

	for (const FHDIDInteractableDef& Def : Chapter->Interactables)
	{
		if (!Def.RequiredCard.IsEmpty() && Cards && !Cards->IsCardReached(Def.RequiredCard))
		{
			continue;
		}
		if (Def.PresentIf.Num() > 0 && !HDIDConditionsMet(Def.PresentIf, Flags))
		{
			continue;
		}
		if (HDIDAnyConditionMet(Def.AbsentIf, Flags))
		{
			continue;
		}
		if (Def.OnlyInFlashback && Def.FlashbackMemoryID != ActiveMemory)
		{
			continue;
		}
		UClass* Class = AHDIDInteractable::StaticClass();
		if (Def.Kind == TEXT("Clue"))
		{
			Class = AHDIDClue::StaticClass();
		}
		else if (Def.Kind == TEXT("MemoryTrigger"))
		{
			Class = AHDIDMemoryTrigger::StaticClass();
		}
		else if (Def.Kind == TEXT("DialogueTrigger"))
		{
			Class = AHDIDDialogueTrigger::StaticClass();
		}
		else if (Def.Kind == TEXT("TimelineTrigger"))
		{
			Class = AHDIDTimelineTrigger::StaticClass();
		}
		else if (Def.Kind == TEXT("CharacterInteraction"))
		{
			Class = AHDIDCharacterInteraction::StaticClass();
		}
		else if (Def.Kind == TEXT("StoryObject"))
		{
			Class = AHDIDStoryObject::StaticClass();
		}
		else if (Def.Kind == TEXT("GrimReaper"))
		{
			Class = AHDIDGrimReaper::StaticClass();
		}
		else if (Def.Kind == TEXT("VoidTrace"))
		{
			Class = AHDIDVoidTrace::StaticClass();
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AHDIDInteractable* Actor = GetWorld()->SpawnActor<AHDIDInteractable>(Class, Def.Position.ToUE(), FRotator::ZeroRotator, Params))
		{
			Actor->ApplyDef(Def);
			InteractableById.Add(Def.Id, Actor);
			SpawnedActors.Add(Actor);
		}
	}
}

bool AHDIDGreyboxWorld::IsCorridorAt(const FVector& Point) const
{
	UHDIDCatalogSubsystem* Catalog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>() : nullptr;
	const FHDIDChapterFile* Chapter = Catalog ? Catalog->FindChapter(BuiltChapterId) : nullptr;
	if (!Chapter)
	{
		return false;
	}
	for (const FHDIDLocationDef& Location : Chapter->Locations)
	{
		const FVector Origin = Location.Origin.ToUE();
		const bool bInside = Point.X >= Origin.X && Point.X <= Origin.X + Location.SizeX && Point.Y >= Origin.Y && Point.Y <= Origin.Y + Location.SizeY;
		if (bInside && Location.Corridor)
		{
			return true;
		}
	}
	return false;
}

FString AHDIDGreyboxWorld::GetLocationIdAt(const FVector& Point) const
{
	UHDIDCatalogSubsystem* Catalog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>() : nullptr;
	const FHDIDChapterFile* Chapter = Catalog ? Catalog->FindChapter(BuiltChapterId) : nullptr;
	if (!Chapter)
	{
		return FString();
	}
	const FHDIDLocationDef* Best = nullptr;
	float BestArea = TNumericLimits<float>::Max();
	for (const FHDIDLocationDef& Location : Chapter->Locations)
	{
		const FVector Origin = Location.Origin.ToUE();
		const bool bInside = Point.X >= Origin.X && Point.X <= Origin.X + Location.SizeX && Point.Y >= Origin.Y && Point.Y <= Origin.Y + Location.SizeY;
		const float Area = Location.SizeX * Location.SizeY;
		if (bInside && Area < BestArea)
		{
			Best = &Location;
			BestArea = Area;
		}
	}
	return Best ? Best->LocationId : FString();
}

AActor* AHDIDGreyboxWorld::FindSpawnedInteractable(const FString& Id) const
{
	const TWeakObjectPtr<AHDIDInteractable>* Found = InteractableById.Find(Id);
	return Found && Found->IsValid() ? Found->Get() : nullptr;
}
