#include "Core/HDIDGameMode.h"

#include "Core/HDIDPlayerCharacter.h"
#include "Core/HDIDPlayerController.h"
#include "Data/HDIDCatalog.h"
#include "HowDidIDie.h"
#include "Save/HDIDSaveGame.h"
#include "Systems/HDIDManagers.h"
#include "UI/HDIDUIManager.h"
#include "World/HDIDGreyboxWorld.h"

AHDIDGameMode::AHDIDGameMode()
{
	DefaultPawnClass = AHDIDPlayerCharacter::StaticClass();
	PlayerControllerClass = AHDIDPlayerController::StaticClass();
	bStartPlayersAsSpectators = true;
}

void AHDIDGameMode::StartFirstPlayable()
{
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FString ChapterId = Catalog ? Catalog->GetFirstPlayableChapterId() : FString();
	StartChapter(ChapterId);
}

void AHDIDGameMode::StartChapter(const FString& ChapterId)
{
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>();
	if (!Catalog || !Catalog->IsPlayable(ChapterId))
	{
		if (UI)
		{
			UI->ShowNotice(TEXT("This chapter is recorded. It is not open."));
		}
		UE_LOG(LogHDID, Warning, TEXT("Refused to start chapter '%s'."), *ChapterId);
		return;
	}
	if (UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>())
	{
		Save->NewGame(ChapterId);
	}
	EnterSession(false);
}

void AHDIDGameMode::ContinueGame()
{
	UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
	UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>();
	if (!Save || !Save->LoadStory() || !Save->GetStory())
	{
		if (UI)
		{
			UI->ShowNotice(TEXT("No record to resume."));
		}
		return;
	}
	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	if (!Catalog || !Catalog->IsPlayable(Save->GetStory()->CurrentChapterId))
	{
		if (UI)
		{
			UI->ShowNotice(TEXT("The saved chapter is not open."));
		}
		return;
	}
	EnterSession(true);
}

void AHDIDGameMode::ReturnToMenu()
{
	if (UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>())
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				Save->RememberTransform(Pawn->GetActorLocation(), PC->GetControlRotation());
			}
		}
		if (bInSession)
		{
			Save->WriteStory();
		}
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			PC->UnPossess();
			Pawn->Destroy();
		}
	}
	if (Greybox)
	{
		Greybox->ClearWorld();
		Greybox->Destroy();
		Greybox = nullptr;
	}
	bInSession = false;
	if (AHDIDPlayerController* PC = Cast<AHDIDPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->EnterMenu();
	}
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ShowMainMenu();
	}
}

void AHDIDGameMode::EnterSession(bool bFromSave)
{
	UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
	if (!Save || !Save->GetStory())
	{
		return;
	}
	if (!Greybox)
	{
		FActorSpawnParameters Params;
		Greybox = GetWorld()->SpawnActor<AHDIDGreyboxWorld>(AHDIDGreyboxWorld::StaticClass(), FTransform::Identity, Params);
	}
	Greybox->BuildChapter(Save->GetStory()->CurrentChapterId);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}
	if (APawn* Existing = PC->GetPawn())
	{
		PC->UnPossess();
		Existing->Destroy();
	}
	const FTransform Spawn = (bFromSave && Save->GetStory()->bHasSavedTransform)
		? FTransform(Save->GetStory()->SavedRotation, Save->GetStory()->SavedLocation)
		: Greybox->GetSpawnTransform();
	RestartPlayerAtTransform(PC, Spawn);
	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->SetActorTransform(Spawn);
	}
	PC->SetControlRotation(Spawn.Rotator());

	if (UHDIDFlashbackManager* Flashback = GetGameInstance()->GetSubsystem<UHDIDFlashbackManager>())
	{
		Flashback->RestoreFromSave();
	}
	if (UHDIDConsequenceManager* Consequence = GetGameInstance()->GetSubsystem<UHDIDConsequenceManager>())
	{
		Consequence->Rebuild();
	}
	if (UHDIDSoulEvaluationManager* Soul = GetGameInstance()->GetSubsystem<UHDIDSoulEvaluationManager>())
	{
		Soul->Recompute();
	}
	Greybox->RefreshInteractables();
	bInSession = true;
	if (AHDIDPlayerController* HPC = Cast<AHDIDPlayerController>(PC))
	{
		HPC->EnterGameplay();
	}
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(PC->GetPawn()))
	{
		Character->ApplyChapterLook();
	}
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ShowGameplay();
	}
	if (UHDIDAudioManager* Audio = GetGameInstance()->GetSubsystem<UHDIDAudioManager>())
	{
		Audio->ApplySettings();
	}
	if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
	{
		Cards->BeginSession();
	}
}
