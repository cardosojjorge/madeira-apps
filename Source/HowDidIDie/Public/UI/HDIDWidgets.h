#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/HDIDTypes.h"
#include "HDIDWidgets.generated.h"

class UHDIDUIManager;
class UTextBlock;
class UBorder;
class UScrollBox;
class UButton;

USTRUCT()
struct FHDIDListItem
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Label;

	UPROPERTY()
	FString Detail;

	UPROPERTY()
	bool bEnabled = true;
};

UCLASS()
class HOWDIDIDIE_API UHDIDClickRelay : public UObject
{
	GENERATED_BODY()

public:
	int32 Index = 0;

	UFUNCTION()
	void Fire();

	TWeakObjectPtr<UUserWidget> Widget;
};

UCLASS()
class HOWDIDIDIE_API UHDIDListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UHDIDUIManager* InOwner, const FString& Heading, const FString& Footer, const TArray<FHDIDListItem>& InItems, bool bRain);
	void Navigate(int32 Delta);
	void SetFooter(const FString& InFooter);
	FString GetSelectedId() const;
	int32 GetSelectedIndex() const { return Selected; }

	UFUNCTION()
	void HandleClick(int32 Index);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Rebuild();

	UPROPERTY()
	TObjectPtr<UHDIDUIManager> Owner;

	UPROPERTY()
	TArray<TObjectPtr<UHDIDClickRelay>> Relays;

	TArray<FHDIDListItem> Items;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> Plates;

	FString Heading;
	FString Footer;

	UPROPERTY()
	TObjectPtr<UTextBlock> FooterBlock;
	int32 Selected = 0;
	bool bShowRain = false;
	float RainPhase = 0.f;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> RainBars;
};

UCLASS()
class HOWDIDIDIE_API UHDIDDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UHDIDUIManager* InOwner, const FHDIDDialogueNode& Node, const TArray<FHDIDChoiceDef>& Choices);
	void Navigate(int32 Delta);
	int32 GetSelectedIndex() const { return Selected; }
	bool HasChoices() const { return Choices.Num() > 0; }

	UFUNCTION()
	void HandleClick(int32 Index);

private:
	void Rebuild();

	UPROPERTY()
	TObjectPtr<UHDIDUIManager> Owner;

	FHDIDDialogueNode Node;
	TArray<FHDIDChoiceDef> Choices;
	int32 Selected = 0;

	UPROPERTY()
	TArray<TObjectPtr<UHDIDClickRelay>> Relays;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> Plates;
};

UCLASS()
class HOWDIDIDIE_API UHDIDTimelineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UHDIDUIManager* InOwner, const FString& FocusEventId);
	void Navigate(int32 Delta);
	void NavigateHorizontal(int32 Delta);
	void Confirm();

private:
	void Rebuild();

	UPROPERTY()
	TObjectPtr<UHDIDUIManager> Owner;

	TArray<FString> EventIds;
	int32 Selected = 0;
	int32 Branch = 0;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> RowTexts;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> RowPlates;
};

UCLASS()
class HOWDIDIDIE_API UHDIDSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(UHDIDUIManager* InOwner);
	void Navigate(int32 Delta);
	void NavigateHorizontal(int32 Delta);
	void Confirm();
	bool IsWaiting() const { return bWaiting; }
	FName GetWaitingSlot() const { return WaitingSlot; }
	void FinishRebind();

private:
	void Rebuild();
	void ChangeValue(int32 Direction);

	UPROPERTY()
	TObjectPtr<UHDIDUIManager> Owner;

	int32 Selected = 0;
	bool bWaiting = false;
	FName WaitingSlot;
	TArray<FName> SlotIds;
	int32 SettingRows = 0;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> ValueTexts;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> RowPlates;
};

UCLASS()
class HOWDIDIDIE_API UHDIDHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure();
	void SetPrompt(const FString& Text, const FVector2D& ScreenPosition, bool bVisible);
	void ShowCard(const FString& IndexLabel, const FString& Title, const FString& Body);
	void ShowNotice(const FString& Text);
	void ShowExamine(const FString& Title, const FString& Body, bool bHeld);
	void SetLetterbox(float Alpha);
	void SetSubtitle(const FString& Speaker, const FString& Line);
	void ToggleLocation();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY()
	TObjectPtr<UBorder> PromptPlate;

	UPROPERTY()
	TObjectPtr<UTextBlock> CardText;

	UPROPERTY()
	TObjectPtr<UTextBlock> NoticeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ExamineText;

	UPROPERTY()
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> LocationText;

	UPROPERTY()
	TObjectPtr<UBorder> LetterTop;

	UPROPERTY()
	TObjectPtr<UBorder> LetterBottom;

	float NoticeLeft = 0.f;
	bool bLocation = false;
	bool bExamine = false;
};
