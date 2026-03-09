#include "GameSystem/T3GameState.h"

#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"

void AT3GameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance가 null"), *GetNameSafe(this));
		return;
	}
	
	SetStates(T3GameInstance->GetSavedGameData()->LevelObjectStates);
}

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

TMap<int32, int32> AT3GameState::GetAllStates()
{
	return LevelObjectStates;
}

void AT3GameState::SetStates(const TMap<int32, int32>& NewStates)
{
	LevelObjectStates.Append(NewStates);
}
