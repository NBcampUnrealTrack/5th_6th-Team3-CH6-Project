#include "GameSystem/T3GameInstance.h"

#include "GameSystem/T3SaveGame.h"
#include "Kismet/GameplayStatics.h"

void UT3GameInstance::Init()
{
	Super::Init();
	
	CurrentSettings = TSharedPtr<FSettings>();
	
	SavedGameData = UGameplayStatics::LoadGameFromSlot(SAVE_GAME_NAME, 0);
}

FObjectPtr<UT3SaveGame> UT3GameInstance::LoadGame()
{
	return SavedGameData;
}

bool UT3GameInstance::SaveGame()
{
	return UGameplayStatics::SaveGameToSlot(SavedGameData, SAVE_GAME_NAME, 0);
}
