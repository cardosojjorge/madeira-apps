#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "HDIDInputSetup.generated.h"

class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
struct FHDIDKeyRebind;

USTRUCT()
struct FHDIDBindSlotView
{
	GENERATED_BODY()

	UPROPERTY()
	FName SlotId;

	UPROPERTY()
	FName ActionId;

	UPROPERTY()
	FString Display;

	UPROPERTY()
	FString Category;

	UPROPERTY()
	FString KeyName;
};

/** Runtime Enhanced Input map. Every slot is player-mappable. No input assets are authored. */
UCLASS()
class HOWDIDIDIE_API UHDIDInputSetup : public UObject
{
	GENERATED_BODY()

public:
	void BuildDefaults();
	void ApplyRebinds(const TArray<FHDIDKeyRebind>& Rebinds);
	void Register(UEnhancedInputLocalPlayerSubsystem* Subsystem);
	UInputAction* GetAction(FName ActionId) const;
	UInputMappingContext* GetContext() const { return Context; }
	TArray<FHDIDBindSlotView> GetBindViews() const;
	void SetSlotKey(FName SlotId, const FKey& Key);

private:
	struct FSlot
	{
		FName SlotId;
		FName ActionId;
		FKey DefaultKey;
		FKey CurrentKey;
		bool bNegate = false;
		bool bSwizzleY = false;
		bool bMouseScale = false;
		FString Display;
		FString Category;
	};

	void RebuildContext();
	UInputAction* AddAction(FName Id, bool bAxis2D);
	void AddSlot(FName SlotId, FName ActionId, const FKey& Key, const FString& Display, const FString& Category, bool bNegate = false, bool bSwizzleY = false, bool bMouseScale = false);

	UPROPERTY()
	TObjectPtr<UInputMappingContext> Context;

	UPROPERTY()
	TMap<FName, TObjectPtr<UInputAction>> Actions;

	TArray<FSlot> Slots;
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> Subsystem;
};
