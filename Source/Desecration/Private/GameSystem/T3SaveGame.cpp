#include "GameSystem/T3SaveGame.h"

void UT3SaveGame::ResetGameData()
{
	PlayerClass = EPlayerClass::None;
	SavedLevelName = ELevelName::Town;
}
