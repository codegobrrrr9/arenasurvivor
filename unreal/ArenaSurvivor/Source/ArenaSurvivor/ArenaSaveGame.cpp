#include "ArenaSaveGame.h"
#include "Kismet/GameplayStatics.h"

static const TCHAR* ArenaSaveSlot = TEXT("arena");

int32 UArenaSaveGame::LoadHighScore()
{
	if (UGameplayStatics::DoesSaveGameExist(ArenaSaveSlot, 0))
	{
		if (UArenaSaveGame* Save = Cast<UArenaSaveGame>(UGameplayStatics::LoadGameFromSlot(ArenaSaveSlot, 0)))
		{
			return Save->HighScore;
		}
	}
	return 0;
}

void UArenaSaveGame::SaveHighScore(int32 Score)
{
	UArenaSaveGame* Save = Cast<UArenaSaveGame>(UGameplayStatics::CreateSaveGameObject(UArenaSaveGame::StaticClass()));
	if (Save == nullptr) return;
	Save->HighScore = Score;
	UGameplayStatics::SaveGameToSlot(Save, ArenaSaveSlot, 0);
}
