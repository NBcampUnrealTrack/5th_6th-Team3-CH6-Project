#include "GameSystem/T3GameState.h"

int32 AT3GameState::GetState(const int32 ObjectID) const
{
	const int32* Result = LevelObjectStates.Find(ObjectID);
	
	if (Result == nullptr)
		return 0;
	
	return *Result;
}

void AT3GameState::SetOrAddState(const int32 ObjectID, const int32 NewState)
{
	if (LevelObjectStates.Contains(ObjectID))
	{
		LevelObjectStates[ObjectID] = NewState;
		return;
	}
	
	LevelObjectStates.Add(ObjectID, NewState);
}

void AT3GameState::SetStates(const TMap<int32, int32>& NewStates)
{
	LevelObjectStates.Append(NewStates);
}

TMap<int32, int32> AT3GameState::GetAllStates()
{
	return LevelObjectStates;
}
