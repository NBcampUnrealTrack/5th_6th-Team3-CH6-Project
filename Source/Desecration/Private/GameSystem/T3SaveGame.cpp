#include "GameSystem/T3SaveGame.h"

#include "Item/Component/T3InventoryComponent.h"

void UT3SaveGame::ResetGameData()
{
	PlayerClass = EPlayerClass::None;
	PlayerName = TEXT("");
	SavedLevelName = ELevelName::Tutorial;
	PlayerLocation = FVector::ZeroVector;
}
