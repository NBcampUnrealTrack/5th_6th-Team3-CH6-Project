#include "GameSystem/T3WorldSubsystem.h"

#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveObjectState.h"

void UT3WorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetWorld()->GetGameInstance());
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
	ObjectStateData = T3GameInstance->GetObjectStateData();
}

int32 UT3WorldSubsystem::GetObjectState(const int32 ObjectID) const
{
	if (!ObjectStateData)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return 0;
	}
	
	const int32* Result = ObjectStateData->LevelObjectStates.Find(ObjectID);
	if (Result == nullptr)
	{
		return 0;
	}
	
	return *Result;
}

void UT3WorldSubsystem::SetOrAddObjectState(const int32 ObjectID, const int32 NewState) const
{
	if (!ObjectStateData)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return;
	}
	
	if (ObjectStateData->LevelObjectStates.Contains(ObjectID))
	{
		ObjectStateData->LevelObjectStates[ObjectID] = NewState;
		return;
	}
	
	ObjectStateData->LevelObjectStates.Emplace(ObjectID, NewState);
	
	//저장
	if (!T3GameInstance->SaveObjectState())
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 물체 상태 저장 실패"), *GetNameSafe(this));
	}
}
