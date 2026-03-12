#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameSystem/GlobalEnums.h"
#include "T3SaveLostMoney.generated.h"

//게임 오버로 인해 잃어버린 재화에 대한 정보
USTRUCT(BlueprintType)
struct FLostMoney
{
	GENERATED_BODY()
	
	FLostMoney()
	{
		LevelName = ELevelName::Tutorial;
		Location = FVector::Zero();
		Money = 0;
	}
	
	FLostMoney(const ELevelName LevelName, const FVector& Location, const int32 Money)
	{
		this->LevelName = LevelName;
		this->Location = Location;
		this->Money = Money;
	}
	
	//장소
	UPROPERTY()
	ELevelName LevelName;
	
	//얼마나 잃었는가
	UPROPERTY()
	int32 Money;
	
	//위치
	UPROPERTY()
	FVector Location;
};

UCLASS()
class DESECRATION_API UT3SaveLostMoney : public USaveGame
{
	GENERATED_BODY()
	
public:
	//데이터 초기화
	void ResetGameData();
	
	//잃어버린 재화 추가
	void AddLostMoney(FLostMoney NewLostMoney);
	
	//잃어버린 재화를 회수하여 목록에서 제거
	void RegainLostMoney(const int32 LostMoneyID);
	
	//잃어버린 재화 목록
	UPROPERTY()
	TMap<int32, FLostMoney> LostMoneyList;
};
