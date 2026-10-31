#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ArenaSaveGame.generated.h"

UCLASS()
class ARENASURVIVOR_API UArenaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 HighScore = 0;

	static int32 LoadHighScore();
	static void SaveHighScore(int32 Score);
};
