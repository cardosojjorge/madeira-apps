#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "HDIDGameInstance.generated.h"

UCLASS()
class HOWDIDIDIE_API UHDIDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
