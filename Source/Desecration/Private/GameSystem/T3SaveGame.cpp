#include "GameSystem/T3SaveGame.h"

void UT3SaveGame::ResetGameData()
{
	PlayerClass = EPlayerClass::None;
	PlayerName = FText();
	SavedLevelName = ELevelName::Tutorial;
	SavePointPos = 0;
}
