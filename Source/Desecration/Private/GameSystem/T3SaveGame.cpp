#include "GameSystem/T3SaveGame.h"

void UT3SaveGame::ResetGameData()
{
	PlayerClass = EPlayerClass::None;
	PlayerName = TEXT("");
	SavedLevelName = ELevelName::Tutorial;
	SavePointPos = 0;
}
