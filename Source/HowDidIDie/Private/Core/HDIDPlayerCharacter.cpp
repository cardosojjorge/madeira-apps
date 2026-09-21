#include "Core/HDIDPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HDIDPlayerController.h"
#include "Data/HDIDCatalog.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/HDIDInteractable.h"
#include "Save/HDIDSaveGame.h"
#include "Systems/HDIDManagers.h"
#include "UI/HDIDUIManager.h"
#include "World/HDIDGreyboxWorld.h"
#include "Core/HDIDGameMode.h"
#include "Engine/StaticMesh.h"

AHDIDPlayerCharacter::AHDIDPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 480.f, 0.f);
	Move->JumpZVelocity = 0.f;
	Move->AirControl = 0.15f;
	Move->MaxWalkSpeed = 190.f;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->MaxAcceleration = 720.f;
	Move->BrakingDecelerationWalking = 480.f;
	Move->GroundFriction = 6.f;
	Move->BrakingFrictionFactor = 0.4f;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 280.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bDoCollisionTest = true;
	SpringArm->ProbeSize = 12.f;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.f;
	SpringArm->SocketOffset = FVector(0.f, 48.f, 62.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->PostProcessBlendWeight = 1.f;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	HeadMesh->SetupAttachment(RootComponent);
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHDIDPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		BodyMesh->SetStaticMesh(Cube);
		BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -10.f));
		BodyMesh->SetRelativeScale3D(FVector(0.42f, 0.28f, 0.85f));
	}
	if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		HeadMesh->SetStaticMesh(Sphere);
		HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 62.f));
		HeadMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.22f));
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->PlayerCameraManager->ViewPitchMin = -50.f;
		PC->PlayerCameraManager->ViewPitchMax = 35.f;
	}
	ApplyChapterLook();
}

void AHDIDPlayerCharacter::ApplyChapterLook()
{
	float Saturation = 0.10f;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UHDIDSaveManager* Save = GI->GetSubsystem<UHDIDSaveManager>())
		{
			if (UHDIDCatalogSubsystem* Catalog = GI->GetSubsystem<UHDIDCatalogSubsystem>())
			{
				if (Save->GetStory())
				{
					if (const FHDIDChapterFile* Chapter = Catalog->FindChapter(Save->GetStory()->CurrentChapterId))
					{
						Saturation = Chapter->ColourSaturation;
					}
				}
			}
		}
	}
	TargetSaturation = Saturation;
	CurrentSaturation = Saturation;
	UpdateColour(0.f);
}

void AHDIDPlayerCharacter::AddMoveAxis(const FVector2D& Axis)
{
	RawMove = Axis.GetClampedToMaxSize(1.f);
	bMoveFresh = true;
}

void AHDIDPlayerCharacter::AddLookMouse(const FVector2D& Axis)
{
	if (bGameplayBlocked || !Controller)
	{
		return;
	}
	float Sens = 1.f;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UHDIDSaveManager* Save = GI->GetSubsystem<UHDIDSaveManager>())
		{
			if (Save->GetSettings())
			{
				Sens = Save->GetSettings()->MouseSensitivity;
			}
		}
	}
	AddControllerYawInput(Axis.X * Sens);
	AddControllerPitchInput(Axis.Y * Sens);
}

void AHDIDPlayerCharacter::AddLookGamepad(const FVector2D& Axis)
{
	if (bGameplayBlocked || !Controller)
	{
		return;
	}
	float Sens = 1.f;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UHDIDSaveManager* Save = GI->GetSubsystem<UHDIDSaveManager>())
		{
			if (Save->GetSettings())
			{
				Sens = Save->GetSettings()->ControllerSensitivity;
			}
		}
	}
	const float Scale = 75.f * Sens * GetWorld()->GetDeltaSeconds();
	AddControllerYawInput(Axis.X * Scale);
	AddControllerPitchInput(Axis.Y * Scale);
}

void AHDIDPlayerCharacter::SetSprinting(bool bInSprinting)
{
	bSprinting = bInSprinting;
}

void AHDIDPlayerCharacter::SetGameplayBlocked(bool bBlocked)
{
	bGameplayBlocked = bBlocked;
	if (bBlocked)
	{
		RawMove = FVector2D::ZeroVector;
		SmoothedMove = FVector2D::ZeroVector;
	}
}

void AHDIDPlayerCharacter::SetSaturation(float Saturation)
{
	TargetSaturation = Saturation;
}

void AHDIDPlayerCharacter::SetCameraMode(const FString& Mode)
{
	if (Mode == TEXT("Close"))
	{
		TargetArm = 160.f;
		SpringArm->SocketOffset = FVector(0.f, 28.f, 58.f);
	}
	else if (Mode == TEXT("Low"))
	{
		TargetArm = 240.f;
		SpringArm->SocketOffset = FVector(0.f, 36.f, 18.f);
	}
	else if (Mode == TEXT("Conversation"))
	{
		TargetArm = 190.f;
		SpringArm->SocketOffset = FVector(0.f, 62.f, 52.f);
	}
	else if (Mode == TEXT("Corridor"))
	{
		TargetArm = 200.f;
		SpringArm->SocketOffset = FVector(0.f, 22.f, 58.f);
	}
	else if (Mode == TEXT("Flashback"))
	{
		TargetArm = 220.f;
		SpringArm->SocketOffset = FVector(0.f, 36.f, 50.f);
	}
	else
	{
		TargetArm = 280.f;
		SpringArm->SocketOffset = FVector(0.f, 48.f, 62.f);
	}
}

void AHDIDPlayerCharacter::SetArmLength(float Length)
{
	TargetArm = Length;
}

void AHDIDPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bMoveFresh)
	{
		RawMove = FVector2D::ZeroVector;
	}
	bMoveFresh = false;

	bool bMotionReduction = false;
	bool bBlockedByUI = bGameplayBlocked;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UHDIDSaveManager* Save = GI->GetSubsystem<UHDIDSaveManager>())
		{
			if (Save->GetSettings())
			{
				bMotionReduction = Save->GetSettings()->bMotionReduction;
			}
		}
		if (UHDIDUIManager* UI = GI->GetSubsystem<UHDIDUIManager>())
		{
			bBlockedByUI = bBlockedByUI || UI->ConsumesGameplay();
		}
		if (UHDIDFlashbackManager* Flashback = GI->GetSubsystem<UHDIDFlashbackManager>())
		{
			bBlockedByUI = bBlockedByUI || Flashback->IsBusy();
		}
	}

	if (bBlockedByUI)
	{
		RawMove = FVector2D::ZeroVector;
	}

	const float InterpSpeed = RawMove.IsNearlyZero() ? 4.8f : 3.1f;
	SmoothedMove = FMath::Vector2DInterpTo(SmoothedMove, RawMove, DeltaSeconds, InterpSpeed);
	GetCharacterMovement()->MaxWalkSpeed = bSprinting && !bBlockedByUI ? 520.f : 190.f;

	if (Controller && !SmoothedMove.IsNearlyZero())
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), SmoothedMove.Y);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), SmoothedMove.X);
	}

	UpdateCamera(DeltaSeconds);
	UpdateColour(DeltaSeconds);
	UpdateFocus();

	const float Speed = GetVelocity().Size2D();
	if (Speed > 40.f && !bBlockedByUI)
	{
		FootstepTimer -= DeltaSeconds;
		if (FootstepTimer <= 0.f)
		{
			FootstepTimer = bSprinting ? 0.32f : 0.48f;
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UHDIDAudioManager* Audio = GI->GetSubsystem<UHDIDAudioManager>())
				{
					Audio->Pulse(TEXT("footsteps"));
				}
			}
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
		{
			if (AHDIDGreyboxWorld* WorldBuilder = Mode->GetGreybox())
			{
				const FString LocationId = WorldBuilder->GetLocationIdAt(GetActorLocation());
				if (!LocationId.IsEmpty() && LocationId != CurrentLocationId)
				{
					CurrentLocationId = LocationId;
					if (UHDIDInvestigationManager* Investigation = GI->GetSubsystem<UHDIDInvestigationManager>())
					{
						Investigation->NotifyLocation(LocationId);
					}
					if (UHDIDStoryCardManager* Cards = GI->GetSubsystem<UHDIDStoryCardManager>())
					{
						Cards->NotifyEnterLocation(LocationId);
					}
				}
			}
		}
	}

	if (GetActorLocation().Z < -400.f)
	{
		if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
		{
			if (AHDIDGreyboxWorld* WorldBuilder = Mode->GetGreybox())
			{
				SetActorTransform(WorldBuilder->GetSpawnTransform());
			}
		}
	}

	if (UGameInstance* PromptInstance = GetGameInstance())
	{
		if (UHDIDUIManager* UI = PromptInstance->GetSubsystem<UHDIDUIManager>())
		{
			if (Focused.IsValid())
			{
				UI->SetPrompt(Focused->GetPromptText(), Focused->GetActorLocation() + FVector(0.f, 0.f, 40.f), true);
			}
			else
			{
				UI->SetPrompt(FString(), FVector::ZeroVector, false);
			}
		}
	}

	SpringArm->bEnableCameraLag = !bMotionReduction;
	SpringArm->CameraLagSpeed = bMotionReduction ? 30.f : 8.f;
}

void AHDIDPlayerCharacter::UpdateCamera(float DeltaSeconds)
{
	bool bCorridor = false;
	if (AHDIDGameMode* Mode = GetWorld()->GetAuthGameMode<AHDIDGameMode>())
	{
		if (AHDIDGreyboxWorld* WorldBuilder = Mode->GetGreybox())
		{
			bCorridor = WorldBuilder->IsCorridorAt(GetActorLocation());
		}
	}
	float Desired = TargetArm;
	if (bCorridor)
	{
		Desired = FMath::Min(Desired, 210.f);
	}
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, Desired, DeltaSeconds, 4.f);
}

void AHDIDPlayerCharacter::UpdateColour(float DeltaSeconds)
{
	if (DeltaSeconds > 0.f)
	{
		CurrentSaturation = FMath::FInterpTo(CurrentSaturation, TargetSaturation, DeltaSeconds, 2.2f);
	}
	FPostProcessSettings& Settings = Camera->PostProcessSettings;
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(CurrentSaturation, CurrentSaturation, CurrentSaturation, 1.f);
	Settings.bOverride_ColorContrast = true;
	Settings.ColorContrast = FVector4(1.12f, 1.12f, 1.16f, 1.f);
	Settings.bOverride_FilmGrainIntensity = true;
	Settings.FilmGrainIntensity = 0.32f;
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.55f;
	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = 0.35f;
	Settings.bOverride_MotionBlurAmount = true;
	Settings.MotionBlurAmount = 0.f;
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = 0.15f;
}

void AHDIDPlayerCharacter::UpdateFocus()
{
	Focused = nullptr;
	if (!Camera || bGameplayBlocked)
	{
		return;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HDIDFocus), false, this);
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * 380.f;
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (AHDIDInteractable* Actor = Cast<AHDIDInteractable>(Hit.GetActor()))
		{
			if (!Actor->UsesCloseRange() || FVector::Dist(Actor->GetActorLocation(), GetActorLocation()) < 130.f)
			{
				Focused = Actor;
			}
		}
	}
}
