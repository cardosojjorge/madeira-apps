#include "UI/HDIDWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/HDIDGameMode.h"
#include "Core/HDIDPlayerCharacter.h"
#include "Core/HDIDPlayerController.h"
#include "Data/HDIDCatalog.h"
#include "Save/HDIDSaveGame.h"
#include "Styling/CoreStyle.h"
#include "Systems/HDIDManagers.h"
#include "UI/HDIDUIManager.h"
#include "World/HDIDGreyboxWorld.h"

namespace
{
	const int32 OverlayZ = 30;

	bool Contrast(const UUserWidget* Widget)
	{
		const UGameInstance* Instance = Widget ? Widget->GetGameInstance() : nullptr;
		const UHDIDSaveManager* Save = Instance ? Instance->GetSubsystem<UHDIDSaveManager>() : nullptr;
		return Save && Save->GetSettings() && Save->GetSettings()->bHighContrast;
	}

	FLinearColor Ink(const UUserWidget* Widget)
	{
		return Contrast(Widget) ? FLinearColor(0.96f, 0.96f, 0.93f) : FLinearColor(0.78f, 0.75f, 0.68f);
	}

	FLinearColor Dim(const UUserWidget* Widget)
	{
		return Contrast(Widget) ? FLinearColor(0.62f, 0.62f, 0.58f) : FLinearColor(0.42f, 0.41f, 0.38f);
	}

	FLinearColor Night()
	{
		return FLinearColor(0.012f, 0.013f, 0.016f, 0.94f);
	}

	FSlateFontInfo Font(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
	}

	UTextBlock* MakeText(UUserWidget* Owner, const FString& Text, int32 Size, const FLinearColor& Color, bool bWrap)
	{
		UTextBlock* Block = Owner->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		Block->SetFont(Font(Size));
		Block->SetColorAndOpacity(FSlateColor(Color));
		Block->SetAutoWrapText(bWrap);
		return Block;
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Child, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size, bool bAuto)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Child);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetAutoSize(bAuto);
		return Slot;
	}

	void ShowRoot(UUserWidget* Widget, UWidget* Root)
	{
		const bool bShown = Widget->IsInViewport();
		if (bShown)
		{
			Widget->RemoveFromParent();
		}
		Widget->WidgetTree->RootWidget = Root;
		if (bShown)
		{
			Widget->ReleaseSlateResources(true);
			Widget->AddToViewport(OverlayZ);
		}
	}

	FButtonStyle QuietButton()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		Style.Normal.TintColor = FSlateColor(FLinearColor::Transparent);
		Style.Hovered.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.05f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.08f));
		Style.NormalPadding = FMargin(10.f, 6.f);
		Style.PressedPadding = FMargin(10.f, 6.f);
		return Style;
	}

	void PaintPlate(UBorder* Plate, bool bSelected, bool bEnabled, const UUserWidget* Widget)
	{
		if (!Plate)
		{
			return;
		}
		if (bSelected)
		{
			Plate->SetBrushColor(Contrast(Widget) ? FLinearColor(0.92f, 0.92f, 0.88f, 1.f) : FLinearColor(0.20f, 0.18f, 0.15f, 1.f));
		}
		else
		{
			Plate->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.035f, bEnabled ? 0.55f : 0.25f));
		}
	}
}

void UHDIDClickRelay::Fire()
{
	if (UHDIDListWidget* List = Cast<UHDIDListWidget>(Widget.Get()))
	{
		List->HandleClick(Index);
	}
	else if (UHDIDDialogueWidget* Dialogue = Cast<UHDIDDialogueWidget>(Widget.Get()))
	{
		Dialogue->HandleClick(Index);
	}
}

void UHDIDListWidget::Setup(UHDIDUIManager* InOwner, const FString& InHeading, const FString& InFooter, const TArray<FHDIDListItem>& InItems, bool bRain)
{
	Owner = InOwner;
	Heading = InHeading;
	Footer = InFooter;
	Items = InItems;
	bShowRain = bRain;
	Selected = 0;
	Rebuild();
}

void UHDIDListWidget::SetFooter(const FString& InFooter)
{
	Footer = InFooter;
	if (FooterBlock)
	{
		FooterBlock->SetText(FText::FromString(Footer));
	}
}

void UHDIDListWidget::Navigate(int32 Delta)
{
	if (Items.Num() == 0)
	{
		return;
	}
	Selected = (Selected + Delta) % Items.Num();
	if (Selected < 0)
	{
		Selected += Items.Num();
	}
	for (int32 Index = 0; Index < Plates.Num(); ++Index)
	{
		const bool bEnabled = Items.IsValidIndex(Index) ? Items[Index].bEnabled : true;
		PaintPlate(Plates[Index], Index == Selected, bEnabled, this);
		if (UBorder* Plate = Plates[Index])
		{
			if (UButton* Button = Cast<UButton>(Plate->GetChildAt(0)))
			{
				if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0)))
				{
					const bool bHot = Index == Selected && Contrast(this);
					Label->SetColorAndOpacity(FSlateColor(bHot ? FLinearColor(0.04f, 0.04f, 0.04f) : (bEnabled ? Ink(this) : Dim(this))));
				}
			}
		}
	}
}

FString UHDIDListWidget::GetSelectedId() const
{
	return Items.IsValidIndex(Selected) ? Items[Selected].Id : FString();
}

void UHDIDListWidget::HandleClick(int32 Index)
{
	if (!Items.IsValidIndex(Index) || !Owner)
	{
		return;
	}
	Selected = Index;
	Navigate(0);
	Owner->Confirm();
}

void UHDIDListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bShowRain)
	{
		return;
	}
	RainPhase += InDeltaTime;
	for (int32 Index = 0; Index < RainBars.Num(); ++Index)
	{
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(RainBars[Index] ? RainBars[Index]->Slot : nullptr))
		{
			FVector2D Position = Slot->GetPosition();
			Position.Y += InDeltaTime * (220.f + (Index % 6) * 48.f);
			if (Position.Y > 1400.f)
			{
				Position.Y = -180.f;
			}
			Slot->SetPosition(Position);
		}
	}
}

void UHDIDListWidget::Rebuild()
{
	Relays.Reset();
	Plates.Reset();
	RainBars.Reset();

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetBrushColor(Contrast(this) ? FLinearColor(0.f, 0.f, 0.f, 0.96f) : Night());
	Place(Canvas, Background, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	if (bShowRain)
	{
		for (int32 Index = 0; Index < 22; ++Index)
		{
			UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Bar->SetBrushColor(FLinearColor(0.75f, 0.78f, 0.82f, 0.07f + (Index % 3) * 0.02f));
			Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
			const float X = 40.f + (Index * 97.f) % 1500.f;
			const float Y = -160.f - (Index * 53.f) % 700.f;
			const float Height = 70.f + (Index % 5) * 28.f;
			Place(Canvas, Bar, FAnchors(0.f, 0.f), FVector2D::ZeroVector, FVector2D(X, Y), FVector2D(2.f, Height), false);
			RainBars.Add(Bar);
		}

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Body->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.012f, 1.f));
		Body->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Body, FAnchors(0.78f, 0.22f), FVector2D(0.5f, 0.f), FVector2D::ZeroVector, FVector2D(78.f, 460.f), false);
		UBorder* Head = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Head->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.012f, 1.f));
		Head->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Head, FAnchors(0.78f, 0.22f), FVector2D(0.5f, 1.f), FVector2D(0.f, -8.f), FVector2D(52.f, 64.f), false);
	}

	FString Title = Heading;
	FString Clock;
	FString Remainder = Footer;
	if (bShowRain)
	{
		FString Left;
		if (Footer.Split(TEXT("\n"), &Left, &Clock, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			Remainder = Left;
		}
	}

	UTextBlock* TitleText = MakeText(this, Title, bShowRain ? 54 : 28, Ink(this), true);
	Place(Canvas, TitleText, FAnchors(0.06f, 0.07f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(760.f, 80.f), true);

	if (bShowRain && !Clock.IsEmpty())
	{
		UTextBlock* ClockText = MakeText(this, Clock, 40, Ink(this), false);
		Place(Canvas, ClockText, FAnchors(0.06f, 0.22f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(400.f, 56.f), true);
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Place(Canvas, Scroll, bShowRain ? FAnchors(0.06f, 0.36f, 0.52f, 0.84f) : FAnchors(0.06f, 0.22f, 0.72f, 0.84f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	const FButtonStyle Style = QuietButton();
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const FHDIDListItem& Item = Items[Index];
		UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		PaintPlate(Plate, Index == Selected, Item.bEnabled, this);
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetStyle(Style);
		FString Line = Item.Label;
		if (!Item.Detail.IsEmpty())
		{
			Line += TEXT("    ");
			Line += Item.Detail;
		}
		const bool bHot = Index == Selected && Contrast(this);
		UTextBlock* Label = MakeText(this, Line, 18, bHot ? FLinearColor(0.04f, 0.04f, 0.04f) : (Item.bEnabled ? Ink(this) : Dim(this)), false);
		Button->SetContent(Label);
		Plate->SetContent(Button);

		UHDIDClickRelay* Relay = NewObject<UHDIDClickRelay>(this);
		Relay->Index = Index;
		Relay->Widget = this;
		Button->OnClicked.AddDynamic(Relay, &UHDIDClickRelay::Fire);
		Relays.Add(Relay);
		Plates.Add(Plate);

		if (UVerticalBoxSlot* Row = Cast<UVerticalBoxSlot>(Scroll->AddChild(Plate)))
		{
			Row->SetPadding(FMargin(0.f, 4.f));
		}
	}

	FooterBlock = MakeText(this, Remainder, 16, Dim(this), true);
	Place(Canvas, FooterBlock, FAnchors(0.06f, 0.88f, 0.7f, 0.97f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	ShowRoot(this, Canvas);
}

void UHDIDDialogueWidget::Setup(UHDIDUIManager* InOwner, const FHDIDDialogueNode& InNode, const TArray<FHDIDChoiceDef>& InChoices)
{
	Owner = InOwner;
	Node = InNode;
	Choices = InChoices;
	Selected = 0;
	Rebuild();
}

void UHDIDDialogueWidget::Navigate(int32 Delta)
{
	if (Choices.Num() == 0)
	{
		return;
	}
	Selected = (Selected + Delta) % Choices.Num();
	if (Selected < 0)
	{
		Selected += Choices.Num();
	}
	for (int32 Index = 0; Index < Plates.Num(); ++Index)
	{
		PaintPlate(Plates[Index], Index == Selected, true, this);
	}
}

void UHDIDDialogueWidget::HandleClick(int32 Index)
{
	if (!Choices.IsValidIndex(Index) || !Owner)
	{
		return;
	}
	Selected = Index;
	Navigate(0);
	Owner->Confirm();
}

void UHDIDDialogueWidget::Rebuild()
{
	Plates.Reset();
	Relays.Reset();
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.72f));
	Place(Canvas, Background, FAnchors(0.f, 0.55f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	FString SpeakerName = Node.Speaker;
	if (const UGameInstance* Instance = GetGameInstance())
	{
		if (const UHDIDCatalogSubsystem* Catalog = Instance->GetSubsystem<UHDIDCatalogSubsystem>())
		{
			if (const FHDIDCharacterDef* Speaker = Catalog->FindCharacter(Node.Speaker))
			{
				SpeakerName = Speaker->DisplayName;
			}
		}
	}
	UTextBlock* SpeakerText = MakeText(this, SpeakerName.ToUpper(), 14, Dim(this), false);
	Place(Canvas, SpeakerText, FAnchors(0.08f, 0.60f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(800.f, 24.f), true);
	UTextBlock* Body = MakeText(this, Node.Text, 22, Ink(this), true);
	Place(Canvas, Body, FAnchors(0.08f, 0.66f, 0.86f, 0.78f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Place(Canvas, Column, FAnchors(0.08f, 0.80f, 0.7f, 0.98f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	const FButtonStyle Style = QuietButton();
	if (Choices.Num() == 0)
	{
		UTextBlock* Continue = MakeText(this, TEXT("SPACE  \u2014  CONTINUE"), 16, Dim(this), false);
		Column->AddChildToVerticalBox(Continue);
	}
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		PaintPlate(Plate, Index == Selected, true, this);
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetStyle(Style);
		UTextBlock* Label = MakeText(this, FString::Printf(TEXT("%d   %s"), Index + 1, *Choices[Index].Text), 18, Ink(this), false);
		Button->SetContent(Label);
		Plate->SetContent(Button);
		UHDIDClickRelay* Relay = NewObject<UHDIDClickRelay>(this);
		Relay->Index = Index;
		Relay->Widget = this;
		Button->OnClicked.AddDynamic(Relay, &UHDIDClickRelay::Fire);
		Relays.Add(Relay);
		Plates.Add(Plate);
		if (UVerticalBoxSlot* Row = Column->AddChildToVerticalBox(Plate))
		{
			Row->SetPadding(FMargin(0.f, 3.f));
		}
	}
	ShowRoot(this, Canvas);
}

void UHDIDTimelineWidget::Setup(UHDIDUIManager* InOwner, const FString& FocusEventId)
{
	Owner = InOwner;
	EventIds.Reset();
	Selected = 0;
	Branch = 0;
	if (const UGameInstance* Instance = GetGameInstance())
	{
		const UHDIDSaveManager* Save = Instance->GetSubsystem<UHDIDSaveManager>();
		const UHDIDCatalogSubsystem* Catalog = Instance->GetSubsystem<UHDIDCatalogSubsystem>();
		if (Save && Save->GetStory() && Catalog)
		{
			if (const FHDIDChapterFile* Chapter = Catalog->FindChapter(Save->GetStory()->CurrentChapterId))
			{
				for (int32 Index = 0; Index < Chapter->Timeline.Num(); ++Index)
				{
					EventIds.Add(Chapter->Timeline[Index].EventID);
					if (Chapter->Timeline[Index].EventID == FocusEventId)
					{
						Selected = Index;
					}
				}
			}
		}
	}
	Rebuild();
}

void UHDIDTimelineWidget::Navigate(int32 Delta)
{
	if (EventIds.Num() == 0)
	{
		return;
	}
	Selected = (Selected + Delta) % EventIds.Num();
	if (Selected < 0)
	{
		Selected += EventIds.Num();
	}
	Branch = 0;
	Rebuild();
}

void UHDIDTimelineWidget::NavigateHorizontal(int32 Delta)
{
	Branch += Delta;
	Rebuild();
}

void UHDIDTimelineWidget::Confirm()
{
	UGameInstance* Instance = GetGameInstance();
	if (!Instance || !EventIds.IsValidIndex(Selected))
	{
		return;
	}
	UHDIDCatalogSubsystem* Catalog = Instance->GetSubsystem<UHDIDCatalogSubsystem>();
	UHDIDTimelineManager* Timeline = Instance->GetSubsystem<UHDIDTimelineManager>();
	const FHDIDTimelineEvent* Event = Catalog ? Catalog->FindEvent(EventIds[Selected]) : nullptr;
	if (!Event || !Timeline)
	{
		return;
	}
	if (!Timeline->IsUnlocked(Event->EventID))
	{
		if (Owner)
		{
			Owner->ShowNotice(TEXT("This hour is not open."));
		}
		return;
	}
	if (Event->Branches.Num() > 0)
	{
		const int32 Index = FMath::Clamp(Branch, 0, Event->Branches.Num() - 1);
		if (UHDIDChoiceManager* Choices = Instance->GetSubsystem<UHDIDChoiceManager>())
		{
			if (!Choices->Commit(Event->Branches[Index].ChoiceID, true) && Owner)
			{
				Owner->ShowNotice(TEXT("That version of the hour will not hold."));
			}
		}
	}
	Timeline->TravelTo(Event->EventID);
	Rebuild();
}

void UHDIDTimelineWidget::Rebuild()
{
	RowTexts.Reset();
	RowPlates.Reset();
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetBrushColor(Night());
	Place(Canvas, Background, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	UTextBlock* Title = MakeText(this, TEXT("TIMELINE"), 28, Ink(this), false);
	Place(Canvas, Title, FAnchors(0.08f, 0.08f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(400.f, 40.f), true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Place(Canvas, Column, FAnchors(0.08f, 0.18f, 0.86f, 0.9f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	const UGameInstance* Instance = GetGameInstance();
	const UHDIDCatalogSubsystem* Catalog = Instance ? Instance->GetSubsystem<UHDIDCatalogSubsystem>() : nullptr;
	const UHDIDTimelineManager* Timeline = Instance ? Instance->GetSubsystem<UHDIDTimelineManager>() : nullptr;
	const UHDIDSaveGame* Story = nullptr;
	if (const UHDIDSaveManager* Save = Instance ? Instance->GetSubsystem<UHDIDSaveManager>() : nullptr)
	{
		Story = Save->GetStory();
	}

	for (int32 Index = 0; Index < EventIds.Num(); ++Index)
	{
		const FHDIDTimelineEvent* Event = Catalog ? Catalog->FindEvent(EventIds[Index]) : nullptr;
		if (!Event)
		{
			continue;
		}
		const bool bOpen = Timeline && Timeline->IsUnlocked(Event->EventID);
		FString Line = bOpen ? (Event->Time + TEXT("    ") + Event->Title) : (TEXT("\u2014\u2014    ") + Event->Title);
		if (Index == Selected && Event->Branches.Num() > 0)
		{
			Branch = FMath::Clamp(Branch, 0, Event->Branches.Num() - 1);
			Line += TEXT("\n");
			for (int32 BranchIndex = 0; BranchIndex < Event->Branches.Num(); ++BranchIndex)
			{
				const FHDIDTimelineBranch& Item = Event->Branches[BranchIndex];
				bool bStanding = false;
				if (Story)
				{
					if (const FHDIDChoiceDef* Choice = Catalog->FindChoice(Item.ChoiceID))
					{
						if (const FString* Saved = Story->ChoiceByBranch.Find(Choice->BranchGroup))
						{
							bStanding = Saved->StartsWith(Item.ChoiceID) || Item.ChoiceID.StartsWith(*Saved);
						}
					}
				}
				if (BranchIndex == Branch)
				{
					Line += TEXT("[ ");
				}
				if (bStanding)
				{
					Line += TEXT("* ");
				}
				Line += Item.Label;
				if (BranchIndex == Branch)
				{
					Line += TEXT(" ]");
				}
				if (BranchIndex + 1 < Event->Branches.Num())
				{
					Line += TEXT("      ");
				}
			}
		}
		UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		PaintPlate(Plate, Index == Selected, bOpen, this);
		UTextBlock* Row = MakeText(this, Line, 18, bOpen ? Ink(this) : Dim(this), true);
		Plate->SetContent(Row);
		Plate->SetPadding(FMargin(12.f, 8.f));
		RowTexts.Add(Row);
		RowPlates.Add(Plate);
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Plate))
		{
			Slot->SetPadding(FMargin(0.f, 4.f));
		}
	}
	UTextBlock* Hint = MakeText(this, TEXT("Left and right revise a branch. Confirm stands in that hour."), 14, Dim(this), false);
	Place(Canvas, Hint, FAnchors(0.08f, 0.92f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(900.f, 24.f), true);
	ShowRoot(this, Canvas);
}

void UHDIDSettingsWidget::Setup(UHDIDUIManager* InOwner)
{
	Owner = InOwner;
	Selected = 0;
	bWaiting = false;
	WaitingSlot = NAME_None;
	Rebuild();
}

void UHDIDSettingsWidget::Navigate(int32 Delta)
{
	const int32 Count = SettingRows + SlotIds.Num();
	if (Count == 0 || bWaiting)
	{
		return;
	}
	Selected = (Selected + Delta) % Count;
	if (Selected < 0)
	{
		Selected += Count;
	}
	for (int32 Index = 0; Index < RowPlates.Num(); ++Index)
	{
		PaintPlate(RowPlates[Index], Index == Selected, true, this);
	}
}

void UHDIDSettingsWidget::NavigateHorizontal(int32 Delta)
{
	if (!bWaiting)
	{
		ChangeValue(Delta);
	}
}

void UHDIDSettingsWidget::Confirm()
{
	if (bWaiting)
	{
		return;
	}
	if (Selected >= SettingRows)
	{
		const int32 SlotIndex = Selected - SettingRows;
		if (!SlotIds.IsValidIndex(SlotIndex))
		{
			return;
		}
		bWaiting = true;
		WaitingSlot = SlotIds[SlotIndex];
		if (AHDIDPlayerController* Controller = GetWorld() ? Cast<AHDIDPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr)
		{
			Controller->SetRebindCapture(true, WaitingSlot);
		}
		if (ValueTexts.IsValidIndex(Selected))
		{
			ValueTexts[Selected]->SetText(FText::FromString(TEXT("PRESS A KEY")));
		}
		return;
	}
	if (Selected == 4 || Selected == 11 || Selected == 12 || Selected == 13)
	{
		ChangeValue(1);
	}
}

void UHDIDSettingsWidget::FinishRebind()
{
	bWaiting = false;
	WaitingSlot = NAME_None;
	Rebuild();
}

void UHDIDSettingsWidget::ChangeValue(int32 Direction)
{
	UGameInstance* Instance = GetGameInstance();
	UHDIDSaveManager* Save = Instance ? Instance->GetSubsystem<UHDIDSaveManager>() : nullptr;
	UHDIDSettingsSave* Data = Save ? Save->GetSettings() : nullptr;
	if (!Data || Selected >= SettingRows)
	{
		return;
	}
	auto Step = [Direction](float& Value, float Min, float Max, float Amount)
	{
		Value = FMath::Clamp(Value + Direction * Amount, Min, Max);
	};
	const bool bWasContrast = Data->bHighContrast;
	switch (Selected)
	{
	case 0: Step(Data->MasterVolume, 0.f, 1.f, 0.05f); break;
	case 1: Step(Data->MusicVolume, 0.f, 1.f, 0.05f); break;
	case 2: Step(Data->EffectsVolume, 0.f, 1.f, 0.05f); break;
	case 3: Step(Data->DialogueVolume, 0.f, 1.f, 0.05f); break;
	case 4: Data->bSubtitles = !Data->bSubtitles; break;
	case 5:
	{
		const float Scales[] = { 0.75f, 1.f, 1.25f, 1.5f };
		int32 Index = 1;
		for (int32 ScaleIndex = 0; ScaleIndex < 4; ++ScaleIndex)
		{
			if (FMath::IsNearlyEqual(Data->SubtitleScale, Scales[ScaleIndex]))
			{
				Index = ScaleIndex;
			}
		}
		Index = (Index + (Direction >= 0 ? 1 : 3)) % 4;
		Data->SubtitleScale = Scales[Index];
		break;
	}
	case 6:
	{
		const TCHAR* Presets[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Ultra"), TEXT("Cinematic") };
		int32 Index = 2;
		for (int32 PresetIndex = 0; PresetIndex < 5; ++PresetIndex)
		{
			if (Data->GraphicsPreset == Presets[PresetIndex])
			{
				Index = PresetIndex;
			}
		}
		Index = (Index + (Direction >= 0 ? 1 : 4)) % 5;
		Data->GraphicsPreset = Presets[Index];
		break;
	}
	case 7:
	{
		const int32 Xs[] = { 1920, 2560, 3840 };
		const int32 Ys[] = { 1080, 1440, 2160 };
		int32 Index = 0;
		for (int32 ResIndex = 0; ResIndex < 3; ++ResIndex)
		{
			if (Data->ResolutionX == Xs[ResIndex])
			{
				Index = ResIndex;
			}
		}
		Index = (Index + (Direction >= 0 ? 1 : 2)) % 3;
		Data->ResolutionX = Xs[Index];
		Data->ResolutionY = Ys[Index];
		break;
	}
	case 8:
	{
		const TCHAR* Modes[] = { TEXT("Windowed"), TEXT("Borderless"), TEXT("Fullscreen") };
		int32 Index = 0;
		for (int32 ModeIndex = 0; ModeIndex < 3; ++ModeIndex)
		{
			if (Data->WindowMode == Modes[ModeIndex])
			{
				Index = ModeIndex;
			}
		}
		Index = (Index + (Direction >= 0 ? 1 : 2)) % 3;
		Data->WindowMode = Modes[Index];
		break;
	}
	case 9: Step(Data->MouseSensitivity, 0.25f, 3.f, 0.25f); break;
	case 10: Step(Data->ControllerSensitivity, 0.25f, 3.f, 0.25f); break;
	case 11: Data->bVibration = !Data->bVibration; break;
	case 12: Data->bMotionReduction = !Data->bMotionReduction; break;
	case 13: Data->bHighContrast = !Data->bHighContrast; break;
	default: break;
	}
	Save->WriteSettings();
	if (AHDIDPlayerController* Controller = GetWorld() ? Cast<AHDIDPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr)
	{
		if (Selected <= 8)
		{
			Controller->ApplyGraphicsSettings();
		}
	}
	if (UHDIDAudioManager* Audio = Instance->GetSubsystem<UHDIDAudioManager>())
	{
		Audio->ApplySettings();
	}
	if (bWasContrast != Data->bHighContrast)
	{
		Rebuild();
		return;
	}
	Rebuild();
}

void UHDIDSettingsWidget::Rebuild()
{
	ValueTexts.Reset();
	RowPlates.Reset();
	SlotIds.Reset();
	SettingRows = 14;

	UGameInstance* Instance = GetGameInstance();
	UHDIDSettingsSave* Data = nullptr;
	if (UHDIDSaveManager* Save = Instance ? Instance->GetSubsystem<UHDIDSaveManager>() : nullptr)
	{
		Data = Save->GetSettings();
	}
	TArray<FHDIDBindSlotView> Binds;
	if (AHDIDPlayerController* Controller = GetWorld() ? Cast<AHDIDPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr)
	{
		if (Controller->GetInputSetup())
		{
			Binds = Controller->GetInputSetup()->GetBindViews();
		}
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Background->SetBrushColor(Contrast(this) ? FLinearColor(0.f, 0.f, 0.f, 0.96f) : Night());
	Place(Canvas, Background, FAnchors(0.f, 0.f, 1.f, 1.f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	UTextBlock* Title = MakeText(this, TEXT("SETTINGS"), 28, Ink(this), false);
	Place(Canvas, Title, FAnchors(0.06f, 0.06f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D(400.f, 40.f), true);

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Place(Canvas, Scroll, FAnchors(0.06f, 0.16f, 0.8f, 0.92f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	TArray<FString> Labels;
	TArray<FString> Values;
	if (Data)
	{
		Labels = {
			TEXT("Master"), TEXT("Music"), TEXT("Effects"), TEXT("Dialogue"), TEXT("Subtitles"), TEXT("Subtitle size"),
			TEXT("Graphics"), TEXT("Resolution"), TEXT("Window"), TEXT("Mouse sensitivity"), TEXT("Controller sensitivity"),
			TEXT("Vibration"), TEXT("Motion reduction"), TEXT("High contrast")
		};
		Values = {
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Data->MasterVolume * 100.f)),
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Data->MusicVolume * 100.f)),
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Data->EffectsVolume * 100.f)),
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Data->DialogueVolume * 100.f)),
			Data->bSubtitles ? TEXT("On") : TEXT("Off"),
			FString::Printf(TEXT("%.2f"), Data->SubtitleScale),
			Data->GraphicsPreset,
			FString::Printf(TEXT("%dx%d"), Data->ResolutionX, Data->ResolutionY),
			Data->WindowMode,
			FString::Printf(TEXT("%.2f"), Data->MouseSensitivity),
			FString::Printf(TEXT("%.2f"), Data->ControllerSensitivity),
			Data->bVibration ? TEXT("On") : TEXT("Off"),
			Data->bMotionReduction ? TEXT("On") : TEXT("Off"),
			Data->bHighContrast ? TEXT("On") : TEXT("Off")
		};
	}
	for (const FHDIDBindSlotView& Bind : Binds)
	{
		Labels.Add(Bind.Display);
		Values.Add(bWaiting && Bind.SlotId == WaitingSlot ? TEXT("PRESS A KEY") : Bind.KeyName);
		SlotIds.Add(Bind.SlotId);
	}

	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		PaintPlate(Plate, Index == Selected, true, this);
		UTextBlock* Row = MakeText(this, Labels[Index] + TEXT("        ") + (Values.IsValidIndex(Index) ? Values[Index] : FString()), 16, Ink(this), false);
		Plate->SetContent(Row);
		Plate->SetPadding(FMargin(12.f, 6.f));
		ValueTexts.Add(Row);
		RowPlates.Add(Plate);
		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Scroll->AddChild(Plate)))
		{
			Slot->SetPadding(FMargin(0.f, 2.f));
		}
	}
	ShowRoot(this, Canvas);
}

void UHDIDHudWidget::Configure()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	LetterTop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LetterTop->SetBrushColor(FLinearColor::Black);
	LetterTop->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, LetterTop, FAnchors(0.f, 0.f, 1.f, 0.f), FVector2D(0.f, 0.f), FVector2D::ZeroVector, FVector2D(0.f, 0.f), false);

	LetterBottom = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	LetterBottom->SetBrushColor(FLinearColor::Black);
	LetterBottom->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, LetterBottom, FAnchors(0.f, 1.f, 1.f, 1.f), FVector2D(0.f, 1.f), FVector2D::ZeroVector, FVector2D(0.f, 0.f), false);

	PromptPlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	PromptPlate->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.024f, 0.88f));
	PromptPlate->SetPadding(FMargin(14.f, 8.f));
	PromptPlate->SetVisibility(ESlateVisibility::Collapsed);
	PromptText = MakeText(this, FString(), 16, Ink(this), false);
	PromptPlate->SetContent(PromptText);
	Place(Canvas, PromptPlate, FAnchors(0.f, 0.f), FVector2D(0.5f, 1.f), FVector2D(960.f, 900.f), FVector2D(280.f, 36.f), true);

	CardText = MakeText(this, FString(), 16, Ink(this), true);
	CardText->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, CardText, FAnchors(0.04f, 0.72f, 0.42f, 0.94f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	NoticeText = MakeText(this, FString(), 18, Ink(this), false);
	NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, NoticeText, FAnchors(0.5f, 0.08f), FVector2D(0.5f, 0.f), FVector2D::ZeroVector, FVector2D(800.f, 32.f), true);

	ExamineText = MakeText(this, FString(), 18, Ink(this), true);
	ExamineText->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, ExamineText, FAnchors(0.28f, 0.38f, 0.72f, 0.58f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	SubtitleText = MakeText(this, FString(), 20, FLinearColor(0.92f, 0.9f, 0.84f), true);
	SubtitleText->SetJustification(ETextJustify::Center);
	SubtitleText->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, SubtitleText, FAnchors(0.18f, 0.84f, 0.82f, 0.94f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	LocationText = MakeText(this, FString(), 16, Ink(this), false);
	LocationText->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, LocationText, FAnchors(0.62f, 0.06f, 0.96f, 0.16f), FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);

	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UHDIDHudWidget::SetPrompt(const FString& Text, const FVector2D& ScreenPosition, bool bVisible)
{
	if (!PromptPlate || !PromptText)
	{
		return;
	}
	if (!bVisible || Text.IsEmpty())
	{
		PromptPlate->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	PromptText->SetText(FText::FromString(Text));
	PromptText->SetColorAndOpacity(FSlateColor(Ink(this)));
	PromptPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(PromptPlate->Slot))
	{
		Slot->SetPosition(ScreenPosition);
		Slot->SetAlignment(FVector2D(0.5f, 1.15f));
		Slot->SetAutoSize(true);
	}
}

void UHDIDHudWidget::ShowCard(const FString& IndexLabel, const FString& Title, const FString& Body)
{
	if (!CardText)
	{
		return;
	}
	CardText->SetText(FText::FromString(IndexLabel + TEXT("\n") + Title + TEXT("\n") + Body));
	CardText->SetColorAndOpacity(FSlateColor(Ink(this)));
	CardText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHDIDHudWidget::ShowNotice(const FString& Text)
{
	if (!NoticeText)
	{
		return;
	}
	NoticeText->SetText(FText::FromString(Text));
	NoticeText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	NoticeLeft = Text.IsEmpty() ? 0.f : 4.f;
}

void UHDIDHudWidget::ShowExamine(const FString& Title, const FString& Body, bool bHeld)
{
	bExamine = bHeld && !Title.IsEmpty();
	if (!ExamineText)
	{
		return;
	}
	ExamineText->SetText(FText::FromString(Title + TEXT("\n") + Body));
	ExamineText->SetVisibility(bExamine ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UHDIDHudWidget::SetLetterbox(float Alpha)
{
	const float Height = FMath::Clamp(Alpha, 0.f, 1.f) * 100.f;
	auto SizeBar = [Height](UBorder* Bar, bool bBottom)
	{
		if (!Bar)
		{
			return;
		}
		Bar->SetVisibility(Height > 1.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Bar->Slot))
		{
			Slot->SetSize(FVector2D(0.f, Height));
			Slot->SetAlignment(FVector2D(0.f, bBottom ? 1.f : 0.f));
		}
	};
	SizeBar(LetterTop, false);
	SizeBar(LetterBottom, true);
}

void UHDIDHudWidget::SetSubtitle(const FString& Speaker, const FString& Line)
{
	if (!SubtitleText)
	{
		return;
	}
	bool bShow = !Line.IsEmpty();
	float Scale = 1.f;
	if (const UGameInstance* Instance = GetGameInstance())
	{
		if (const UHDIDSaveManager* Save = Instance->GetSubsystem<UHDIDSaveManager>())
		{
			if (Save->GetSettings())
			{
				bShow = bShow && Save->GetSettings()->bSubtitles;
				Scale = Save->GetSettings()->SubtitleScale;
			}
		}
	}
	if (!bShow)
	{
		SubtitleText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const FString Spoken = Speaker.IsEmpty() ? Line : (Speaker + TEXT("\n") + Line);
	SubtitleText->SetText(FText::FromString(Spoken));
	SubtitleText->SetFont(Font(FMath::RoundToInt(20.f * Scale)));
	SubtitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHDIDHudWidget::ToggleLocation()
{
	bLocation = !bLocation;
	if (!LocationText)
	{
		return;
	}
	if (!bLocation)
	{
		LocationText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	FString Name = TEXT("Unmarked");
	if (const UWorld* World = GetWorld())
	{
		if (const AHDIDGameMode* Mode = World->GetAuthGameMode<AHDIDGameMode>())
		{
			if (const APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr)
			{
				if (const AHDIDGreyboxWorld* Greybox = Mode->GetGreybox())
				{
					const FString LocationId = Greybox->GetLocationIdAt(Pawn->GetActorLocation());
					if (const UGameInstance* Instance = GetGameInstance())
					{
						if (const UHDIDCatalogSubsystem* Catalog = Instance->GetSubsystem<UHDIDCatalogSubsystem>())
						{
							if (const FHDIDLocationDef* Location = Catalog->FindLocation(LocationId))
							{
								Name = Location->Name;
							}
						}
					}
				}
			}
		}
	}
	LocationText->SetText(FText::FromString(Name));
	LocationText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHDIDHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (NoticeLeft > 0.f)
	{
		NoticeLeft -= InDeltaTime;
		if (NoticeLeft <= 0.f && NoticeText)
		{
			NoticeText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
