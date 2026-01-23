#include "GameSystem/T3GameInstance.h"

#include "Kismet/GameplayStatics.h"

void UT3GameInstance::Init()
{
	Super::Init();
	
	SavedGameData = UGameplayStatics::LoadGameFromSlot(SAVE_GAME_NAME, 0);
}

FObjectPtr<AT3SaveGame> UT3GameInstance::LoadGame()
{
	return SavedGameData;
}

bool UT3GameInstance::SaveGame()
{
	return UGameplayStatics::SaveGameToSlot(SavedGameData, SAVE_GAME_NAME, 0);
}
