#include "Core/HDIDPlayerController.h"

#include "Core/HDIDGameMode.h"
#include "Core/HDIDInputSetup.h"
#include "Core/HDIDPlayerCharacter.h"
#include "Data/HDIDCatalog.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerInput.h"
#include "HowDidIDie.h"
#include "InputActionValue.h"
#include "Interaction/HDIDInteractable.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Save/HDIDSaveGame.h"
#include "Systems/HDIDManagers.h"
#include "UI/HDIDUIManager.h"

AHDIDPlayerController::AHDIDPlayerController()
{
	bShowMouseCursor = true;
}

void AHDIDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ApplyGraphicsSettings();
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (InputSetup)
			{
				InputSetup->Register(Subsystem);
			}
		}
	}
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ShowMainMenu();
	}
	EnterMenu();
}

void AHDIDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputSetup = NewObject<UHDIDInputSetup>(this);
	InputSetup->BuildDefaults();
	if (UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>())
	{
		if (Save->GetSettings())
		{
			InputSetup->ApplyRebinds(Save->GetSettings()->Rebinds);
		}
	}
	if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Move")), ETriggerEvent::Triggered, this, &AHDIDPlayerController::OnMove);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("LookMouse")), ETriggerEvent::Triggered, this, &AHDIDPlayerController::OnLookMouse);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("LookGamepad")), ETriggerEvent::Triggered, this, &AHDIDPlayerController::OnLookGamepad);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Sprint")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnSprintStarted);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Sprint")), ETriggerEvent::Completed, this, &AHDIDPlayerController::OnSprintStopped);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Interact")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnInteract);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Inspect")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnInspectStarted);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Inspect")), ETriggerEvent::Completed, this, &AHDIDPlayerController::OnInspectStopped);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Advance")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnAdvance);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Pause")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnPause);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Notebook")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnNotebook);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Timeline")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnTimeline);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Memory")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnMemory);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Rewind")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnRewind);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Choice1")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnChoice1);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Choice2")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnChoice2);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Choice3")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnChoice3);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Choice4")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnChoice4);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Map")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnMap);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("UIUp")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnUIUp);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("UIDown")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnUIDown);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("UILeft")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnUILeft);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("UIRight")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnUIRight);
		Enhanced->BindAction(InputSetup->GetAction(TEXT("Back")), ETriggerEvent::Started, this, &AHDIDPlayerController::OnBack);
	}
	else
	{
		UE_LOG(LogHDID, Error, TEXT("Enhanced Input component missing. Check DefaultInput.ini."));
	}
}

bool AHDIDPlayerController::InputKey(const FInputKeyParams& Params)
{
	if (bCapturingRebind && Params.Event == IE_Pressed)
	{
		if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
		{
			if (UI->HandleRebindKey(Params.Key))
			{
				return true;
			}
		}
	}
	return Super::InputKey(Params);
}

void AHDIDPlayerController::EnterGameplay()
{
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = false;
}

void AHDIDPlayerController::EnterMenu()
{
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

void AHDIDPlayerController::RequestNewGame()
{
	if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
	{
		Mode->StartFirstPlayable();
	}
}

void AHDIDPlayerController::RequestContinue()
{
	if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
	{
		Mode->ContinueGame();
	}
}

void AHDIDPlayerController::RequestChapter(const FString& ChapterId)
{
	if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
	{
		Mode->StartChapter(ChapterId);
	}
}

void AHDIDPlayerController::RequestQuit()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AHDIDPlayerController::ReturnToMenu()
{
	if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
	{
		Mode->ReturnToMenu();
	}
}

void AHDIDPlayerController::ShowExamine(const FString& Title, const FString& Body, bool bHeld)
{
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ShowExamine(Title, Body, bHeld);
	}
}

void AHDIDPlayerController::PulseHaptics()
{
	UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
	if (!Save || !Save->GetSettings() || !Save->GetSettings()->bVibration)
	{
		return;
	}
	SetHapticsByValue(0.35f, 0.45f, EControllerHand::Left);
	SetHapticsByValue(0.35f, 0.45f, EControllerHand::Right);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		SetHapticsByValue(0.f, 0.f, EControllerHand::Left);
		SetHapticsByValue(0.f, 0.f, EControllerHand::Right);
	}), 0.12f, false);
}

bool AHDIDPlayerController::HandleRebindKey(const FKey& Key)
{
	return bCapturingRebind && Key.IsValid();
}

void AHDIDPlayerController::SetRebindCapture(bool bCapture, FName SlotId)
{
	bCapturingRebind = bCapture;
	RebindSlot = SlotId;
}

void AHDIDPlayerController::ApplyGraphicsSettings()
{
	UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Save || !Save->GetSettings() || !Settings)
	{
		return;
	}
	UHDIDSettingsSave* Data = Save->GetSettings();
	Settings->SetScreenResolution(FIntPoint(Data->ResolutionX, Data->ResolutionY));
	EWindowMode::Type Mode = EWindowMode::Windowed;
	if (Data->WindowMode == TEXT("Borderless"))
	{
		Mode = EWindowMode::WindowedFullscreen;
	}
	else if (Data->WindowMode == TEXT("Fullscreen"))
	{
		Mode = EWindowMode::Fullscreen;
	}
	Settings->SetFullscreenMode(Mode);
	int32 Level = 2;
	if (Data->GraphicsPreset == TEXT("Low")) Level = 0;
	else if (Data->GraphicsPreset == TEXT("Medium")) Level = 1;
	else if (Data->GraphicsPreset == TEXT("High")) Level = 2;
	else if (Data->GraphicsPreset == TEXT("Ultra")) Level = 3;
	else if (Data->GraphicsPreset == TEXT("Cinematic")) Level = 4;
	Settings->SetOverallScalabilityLevel(Level);
	Settings->ApplySettings(false);
	if (UHDIDAudioManager* Audio = GetGameInstance()->GetSubsystem<UHDIDAudioManager>())
	{
		Audio->ApplySettings();
	}
}

void AHDIDPlayerController::HDIDValidate()
{
	// Implemented in the validator translation unit via the console command.
	ConsoleCommand(TEXT("HDID.ValidateNarrative"));
}

void AHDIDPlayerController::HDIDDumpSoul()
{
	UHDIDSaveManager* Save = GetGameInstance()->GetSubsystem<UHDIDSaveManager>();
	if (!Save || !Save->GetStory())
	{
		return;
	}
	const FHDIDSoulValues& Soul = Save->GetStory()->Soul;
	UE_LOG(LogHDID, Display, TEXT("Hidden soul T%d E%d C%d S%d Cu%d H%d Co%d U%d D%d"),
		Soul.Truth, Soul.Empathy, Soul.Courage, Soul.Sacrifice, Soul.Curiosity, Soul.Honesty, Soul.Compassion, Soul.Understanding, Soul.Dedication);
}

bool AHDIDPlayerController::AllowGameplayMotion() const
{
	const UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>();
	if (UI && UI->ConsumesGameplay())
	{
		return false;
	}
	const AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>();
	return Mode && Mode->IsInSession();
}

void AHDIDPlayerController::OnMove(const FInputActionValue& Value)
{
	if (!AllowGameplayMotion())
	{
		return;
	}
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		Character->AddMoveAxis(Value.Get<FVector2D>());
	}
}

void AHDIDPlayerController::OnLookMouse(const FInputActionValue& Value)
{
	if (!AllowGameplayMotion())
	{
		return;
	}
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		Character->AddLookMouse(Value.Get<FVector2D>());
	}
}

void AHDIDPlayerController::OnLookGamepad(const FInputActionValue& Value)
{
	if (!AllowGameplayMotion())
	{
		return;
	}
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		Character->AddLookGamepad(Value.Get<FVector2D>());
	}
}

void AHDIDPlayerController::OnSprintStarted(const FInputActionValue& Value)
{
	(void)Value;
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		Character->SetSprinting(true);
	}
}

void AHDIDPlayerController::OnSprintStopped(const FInputActionValue& Value)
{
	(void)Value;
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		Character->SetSprinting(false);
	}
}

void AHDIDPlayerController::OnInteract(const FInputActionValue& Value)
{
	(void)Value;
	UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>();
	if (UI && UI->IsOverlayOpen())
	{
		UI->Confirm();
		return;
	}
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		if (AHDIDInteractable* Focused = Character->GetFocused())
		{
			Focused->Interact(this);
		}
	}
}

void AHDIDPlayerController::OnInspectStarted(const FInputActionValue& Value)
{
	(void)Value;
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		if (AHDIDInteractable* Focused = Character->GetFocused())
		{
			Focused->FocusInspect(this, true);
		}
	}
}

void AHDIDPlayerController::OnInspectStopped(const FInputActionValue& Value)
{
	(void)Value;
	ShowExamine(FString(), FString(), false);
}

void AHDIDPlayerController::OnAdvance(const FInputActionValue& Value)
{
	(void)Value;
	UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>();
	if (UI && UI->IsOverlayOpen())
	{
		UI->Confirm();
	}
}

void AHDIDPlayerController::OnPause(const FInputActionValue& Value)
{
	(void)Value;
	UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>();
	if (!UI)
	{
		return;
	}
	if (UI->IsOverlayOpen())
	{
		UI->Back();
		return;
	}
	if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
	{
		if (Mode->IsInSession())
		{
			UI->PushPause();
		}
	}
}

void AHDIDPlayerController::OnNotebook(const FInputActionValue& Value)
{
	(void)Value;
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ToggleNotebook();
	}
}

void AHDIDPlayerController::OnTimeline(const FInputActionValue& Value)
{
	(void)Value;
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ToggleTimeline(FString());
	}
}

void AHDIDPlayerController::OnMemory(const FInputActionValue& Value)
{
	(void)Value;
	UHDIDFlashbackManager* Flashback = GetGameInstance()->GetSubsystem<UHDIDFlashbackManager>();
	if (Flashback && Flashback->IsActive())
	{
		Flashback->TryReturn();
		return;
	}
	if (AHDIDPlayerCharacter* Character = Cast<AHDIDPlayerCharacter>(GetPawn()))
	{
		if (AHDIDInteractable* Focused = Character->GetFocused())
		{
			if (Focused->HasMemory())
			{
				Focused->Interact(this);
				return;
			}
		}
	}
	if (UHDIDStoryCardManager* Cards = GetGameInstance()->GetSubsystem<UHDIDStoryCardManager>())
	{
		if (const FHDIDCardDef* Card = Cards->GetCurrentCard())
		{
			if (Card->Flashbacks.Num() > 0 && Flashback)
			{
				Flashback->BeginMemory(Card->Flashbacks[0], nullptr);
			}
		}
	}
}

void AHDIDPlayerController::OnRewind(const FInputActionValue& Value)
{
	(void)Value;
	UHDIDTimelineManager* Timeline = GetGameInstance()->GetSubsystem<UHDIDTimelineManager>();
	UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>();
	if (Timeline && UI)
	{
		UI->ToggleTimeline(Timeline->GetPrimaryBranchEventId());
	}
}

void AHDIDPlayerController::Choose(int32 Index)
{
	if (UHDIDDialogueManager* Dialogue = GetGameInstance()->GetSubsystem<UHDIDDialogueManager>())
	{
		if (Dialogue->IsOpen())
		{
			Dialogue->Choose(Index);
		}
	}
}

void AHDIDPlayerController::OnChoice1(const FInputActionValue& Value) { (void)Value; Choose(0); }
void AHDIDPlayerController::OnChoice2(const FInputActionValue& Value) { (void)Value; Choose(1); }
void AHDIDPlayerController::OnChoice3(const FInputActionValue& Value) { (void)Value; Choose(2); }
void AHDIDPlayerController::OnChoice4(const FInputActionValue& Value) { (void)Value; Choose(3); }

void AHDIDPlayerController::OnMap(const FInputActionValue& Value)
{
	(void)Value;
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		UI->ToggleLocationCard();
	}
}

void AHDIDPlayerController::OnUIUp(const FInputActionValue& Value) { (void)Value; if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>()) UI->Navigate(-1); }
void AHDIDPlayerController::OnUIDown(const FInputActionValue& Value) { (void)Value; if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>()) UI->Navigate(1); }
void AHDIDPlayerController::OnUILeft(const FInputActionValue& Value) { (void)Value; if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>()) UI->NavigateHorizontal(-1); }
void AHDIDPlayerController::OnUIRight(const FInputActionValue& Value) { (void)Value; if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>()) UI->NavigateHorizontal(1); }

void AHDIDPlayerController::OnBack(const FInputActionValue& Value)
{
	(void)Value;
	if (UHDIDUIManager* UI = GetGameInstance()->GetSubsystem<UHDIDUIManager>())
	{
		if (UI->IsOverlayOpen())
		{
			UI->Back();
		}
	}
}
