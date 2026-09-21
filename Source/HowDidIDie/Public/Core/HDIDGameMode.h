#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HDIDGameMode.generated.h"

class AHDIDGreyboxWorld;

UCLASS()
class HOWDIDIDIE_API AHDIDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHDIDGameMode();

	void StartFirstPlayable();
	void StartChapter(const FString& ChapterId);
	void ContinueGame();
	void ReturnToMenu();
	bool IsInSession() const { return bInSession; }
	AHDIDGreyboxWorld* GetGreybox() const { return Greybox; }

private:
	void EnterSession(bool bFromSave);

	UPROPERTY()
	TObjectPtr<AHDIDGreyboxWorld> Greybox;

	bool bInSession = false;
};
