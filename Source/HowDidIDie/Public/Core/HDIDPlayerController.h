#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HDIDPlayerController.generated.h"

class UHDIDInputSetup;
class UInputAction;
struct FInputActionValue;
struct FInputKeyParams;

/** Input and UI routing only. No chapter-specific branches. */
UCLASS()
class HOWDIDIDIE_API AHDIDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHDIDPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyParams& Params) override;

	void EnterGameplay();
	void EnterMenu();
	void RequestNewGame();
	void RequestContinue();
	void RequestChapter(const FString& ChapterId);
	void RequestQuit();
	void ReturnToMenu();
	void ShowExamine(const FString& Title, const FString& Body, bool bHeld);
	void PulseHaptics();
	bool HandleRebindKey(const FKey& Key);
	void SetRebindCapture(bool bCapture, FName SlotId);
	UHDIDInputSetup* GetInputSetup() const { return InputSetup; }
	void ApplyGraphicsSettings();

	UFUNCTION(Exec)
	void HDIDValidate();

	UFUNCTION(Exec)
	void HDIDDumpSoul();

	UFUNCTION()
	void OnMove(const FInputActionValue& Value);
	UFUNCTION()
	void OnLookMouse(const FInputActionValue& Value);
	UFUNCTION()
	void OnLookGamepad(const FInputActionValue& Value);
	UFUNCTION()
	void OnSprintStarted(const FInputActionValue& Value);
	UFUNCTION()
	void OnSprintStopped(const FInputActionValue& Value);
	UFUNCTION()
	void OnInteract(const FInputActionValue& Value);
	UFUNCTION()
	void OnInspectStarted(const FInputActionValue& Value);
	UFUNCTION()
	void OnInspectStopped(const FInputActionValue& Value);
	UFUNCTION()
	void OnAdvance(const FInputActionValue& Value);
	UFUNCTION()
	void OnPause(const FInputActionValue& Value);
	UFUNCTION()
	void OnNotebook(const FInputActionValue& Value);
	UFUNCTION()
	void OnTimeline(const FInputActionValue& Value);
	UFUNCTION()
	void OnMemory(const FInputActionValue& Value);
	UFUNCTION()
	void OnRewind(const FInputActionValue& Value);
	UFUNCTION()
	void OnChoice1(const FInputActionValue& Value);
	UFUNCTION()
	void OnChoice2(const FInputActionValue& Value);
	UFUNCTION()
	void OnChoice3(const FInputActionValue& Value);
	UFUNCTION()
	void OnChoice4(const FInputActionValue& Value);
	UFUNCTION()
	void OnMap(const FInputActionValue& Value);
	UFUNCTION()
	void OnUIUp(const FInputActionValue& Value);
	UFUNCTION()
	void OnUIDown(const FInputActionValue& Value);
	UFUNCTION()
	void OnUILeft(const FInputActionValue& Value);
	UFUNCTION()
	void OnUIRight(const FInputActionValue& Value);
	UFUNCTION()
	void OnBack(const FInputActionValue& Value);

private:
	bool AllowGameplayMotion() const;
	void Choose(int32 Index);

	UPROPERTY()
	TObjectPtr<UHDIDInputSetup> InputSetup;

	bool bCapturingRebind = false;
	FName RebindSlot;
};
