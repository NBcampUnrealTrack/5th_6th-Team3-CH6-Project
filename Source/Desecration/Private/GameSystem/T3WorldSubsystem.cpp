#include "GameSystem/T3WorldSubsystem.h"

#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"

void UT3WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//게임 인스턴스
	const TObjectPtr<UT3GameInstance> T3GameInstance = Cast<UT3GameInstance>(GetWorld()->GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance가 null"), *GetNameSafe(this));
		return;
	}
	
	//이 서브클래스는 게임 내에서만 작동
	if (T3GameInstance->GetCurrentLevel() == ELevelName::Title || T3GameInstance->GetCurrentLevel() == ELevelName::SelectClass)
	{
		return;
	}
	
	//저장된 게임
	T3SaveGame = T3GameInstance->GetSavedGameData();
}

int32 UT3WorldSubsystem::GetObjectState(const int32 ObjectID) const
{
	if (!T3SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return 0;
	}
	
	const int32* Result = T3SaveGame->LevelObjectStates.Find(ObjectID);
	if (Result == nullptr)
	{
		return 0;
	}
	
	return *Result;
}

void UT3WorldSubsystem::SetOrAddObjectState(const int32 ObjectID, const int32 NewState) const
{
	if (!T3SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return;
	}
	
	if (T3SaveGame->LevelObjectStates.Contains(ObjectID))
	{
		T3SaveGame->LevelObjectStates[ObjectID] = NewState;
		return;
	}
	
	T3SaveGame->LevelObjectStates.Emplace(ObjectID, NewState);
}

int32 UT3WorldSubsystem::GetEnemyState(const int32 EnemyID) const
{
	if (!T3SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return 0;
	}
	
	const int32* Result = T3SaveGame->EnemyStates.Find(EnemyID);
	if (Result == nullptr)
	{
		return 0;
	}
	
	return *Result;
}

void UT3WorldSubsystem::SetOrAddEnemyState(const int32 EnemyID, const int32 NewState) const
{
	if (!T3SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return;
	}
	
	if (T3SaveGame->EnemyStates.Contains(EnemyID))
	{
		T3SaveGame->EnemyStates[EnemyID] = NewState;
		return;
	}
	
	T3SaveGame->EnemyStates.Emplace(EnemyID, NewState);
}