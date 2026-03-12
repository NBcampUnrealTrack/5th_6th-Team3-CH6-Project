#include "GameSystem/T3WorldSubsystem.h"

#include "GameSystem/T3GameInstance.h"

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
	
	//물체의 상태 확인
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	LevelObjectStates.Append(SaveGame->LevelObjectStates);
	
	//잃어버린 재화
	for (TTuple<int32, FLostMoney> LostMoney : SaveGame->LostMoneyList)
	{
		if (T3GameInstance->GetCurrentLevel() != LostMoney.Value.LevelName)
		{
			continue;
		}
		
		LostMoneyList.Emplace(LostMoney.Key, LostMoney.Value);
	}
}

int32 UT3WorldSubsystem::GetState(const int32 ObjectID) const
{
	const int32* Result = LevelObjectStates.Find(ObjectID);
	if (Result == nullptr)
	{
		return 0;
	}
	
	return *Result;
}

void UT3WorldSubsystem::SetOrAddState(const int32 ObjectID, const int32 NewState)
{
	if (LevelObjectStates.Contains(ObjectID))
	{
		LevelObjectStates[ObjectID] = NewState;
		return;
	}
	
	LevelObjectStates.Emplace(ObjectID, NewState);
}

TMap<int32, int32> UT3WorldSubsystem::GetAllStates()
{
	return LevelObjectStates;
}

TMap<int32, FLostMoney> UT3WorldSubsystem::GetAllLostMoney()
{
	return LostMoneyList;
}
