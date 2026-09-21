#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HDIDPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class AHDIDInteractable;

/** Intended Blueprint wrapper: BP_PlayerCharacter. Greybox mesh stands in for the Animation Blueprint. */
UCLASS()
class HOWDIDIDIE_API AHDIDPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHDIDPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	void AddMoveAxis(const FVector2D& Axis);
	void AddLookMouse(const FVector2D& Axis);
	void AddLookGamepad(const FVector2D& Axis);
	void SetSprinting(bool bInSprinting);
	void SetGameplayBlocked(bool bBlocked);
	void SetSaturation(float Saturation);
	void SetCameraMode(const FString& Mode);
	void SetArmLength(float Length);
	void ApplyChapterLook();
	AHDIDInteractable* GetFocused() const { return Focused.Get(); }
	UCameraComponent* GetViewCamera() const { return Camera; }

private:
	void UpdateFocus();
	void UpdateCamera(float DeltaSeconds);
	void UpdateColour(float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	FVector2D RawMove = FVector2D::ZeroVector;
	FVector2D SmoothedMove = FVector2D::ZeroVector;
	bool bMoveFresh = false;
	bool bSprinting = false;
	bool bGameplayBlocked = false;
	float TargetArm = 280.f;
	float TargetSaturation = 0.1f;
	float CurrentSaturation = 0.1f;
	float FootstepTimer = 0.f;
	FString CurrentLocationId;
	TWeakObjectPtr<AHDIDInteractable> Focused;
};
