#include "GameSystem/T3SaveObjectState.h"

void UT3SaveObjectState::ResetGameData()
{
	Super::ResetGameData();
	
	//물체 상태
	LevelObjectStates.Empty();
}
