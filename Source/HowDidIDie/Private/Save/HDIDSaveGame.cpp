#include "Save/HDIDSaveGame.h"

#include "Data/HDIDCatalog.h"
#include "Kismet/GameplayStatics.h"

void UHDIDSaveManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Settings = Cast<UHDIDSettingsSave>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName(), 0));
	if (!Settings)
	{
		Settings = Cast<UHDIDSettingsSave>(UGameplayStatics::CreateSaveGameObject(UHDIDSettingsSave::StaticClass()));
	}
	Story = Cast<UHDIDSaveGame>(UGameplayStatics::CreateSaveGameObject(UHDIDSaveGame::StaticClass()));
}

bool UHDIDSaveManager::HasStorySave() const
{
	return UGameplayStatics::DoesSaveGameExist(StorySlotName(), 0);
}

void UHDIDSaveManager::NewGame(const FString& ChapterId)
{
	Story = Cast<UHDIDSaveGame>(UGameplayStatics::CreateSaveGameObject(UHDIDSaveGame::StaticClass()));
	if (!Story)
	{
		return;
	}

	UHDIDCatalogSubsystem* Catalog = GetGameInstance()->GetSubsystem<UHDIDCatalogSubsystem>();
	const FHDIDChapterFile* Chapter = Catalog ? Catalog->FindChapter(ChapterId) : nullptr;
	Story->CurrentChapterId = ChapterId;
	if (Chapter && Chapter->Cards.Num() > 0)
	{
		Story->CurrentCardId = Chapter->Cards[0].CardID;
		Story->CardsReached.Add(Story->CurrentCardId);
		Story->ProtagonistId = Chapter->ProtagonistId;
	}
	if (Chapter)
	{
		for (const FHDIDTimelineEvent& Event : Chapter->Timeline)
		{
			if (!Event.StartsLocked)
			{
				Story->TimelineUnlocked.Add(Event.EventID);
			}
		}
	}
	WriteStory();
}

bool UHDIDSaveManager::LoadStory()
{
	UHDIDSaveGame* Loaded = Cast<UHDIDSaveGame>(UGameplayStatics::LoadGameFromSlot(StorySlotName(), 0));
	if (!Loaded)
	{
		return false;
	}
	Story = Loaded;
	return true;
}

void UHDIDSaveManager::WriteStory()
{
	if (Story)
	{
		UGameplayStatics::SaveGameToSlot(Story, StorySlotName(), 0);
	}
}

void UHDIDSaveManager::WriteSettings()
{
	if (Settings)
	{
		UGameplayStatics::SaveGameToSlot(Settings, SettingsSlotName(), 0);
	}
}

void UHDIDSaveManager::SetBaseFlag(const FString& Flag, const FString& Value)
{
	if (Story && !Flag.IsEmpty())
	{
		Story->BaseFlags.Add(Flag, Value);
	}
}

void UHDIDSaveManager::RememberTransform(const FVector& Location, const FRotator& Rotation)
{
	if (!Story)
	{
		return;
	}
	Story->SavedLocation = Location;
	Story->SavedRotation = Rotation;
	Story->bHasSavedTransform = true;
}

const TArray<FString>& UHDIDSaveManager::RequiredStoryProperties()
{
	static const TArray<FString> Names = {
		TEXT("CurrentChapterId"),
		TEXT("CurrentCardId"),
		TEXT("ProtagonistId"),
		TEXT("TimelineVisited"),
		TEXT("TimelineUnlocked"),
		TEXT("ChoiceByBranch"),
		TEXT("OneShotChoices"),
		TEXT("CluesFound"),
		TEXT("MemoriesFound"),
		TEXT("Relationships"),
		TEXT("CrossChapterDiscoveries"),
		TEXT("Soul"),
		TEXT("Dedication"),
		TEXT("EndingsUnlocked"),
		TEXT("UnlockedContent"),
		TEXT("SavedLocation"),
		TEXT("SavedRotation"),
		TEXT("bHasSavedTransform"),
		TEXT("ActiveMemoryId"),
		TEXT("BaseFlags"),
		TEXT("CardsReached"),
		TEXT("SpeakersHeard"),
		TEXT("LocationsEntered"),
		TEXT("InspectedIds"),
		TEXT("DialoguesCompleted"),
		TEXT("bTimelineOpened"),
		TEXT("BranchRevisionCount"),
		TEXT("bChapterComplete")
	};
	return Names;
}
