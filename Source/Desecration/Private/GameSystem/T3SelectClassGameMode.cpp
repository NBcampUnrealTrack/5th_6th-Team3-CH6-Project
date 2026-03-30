#include "GameSystem/T3SelectClassGameMode.h"

#include "GameSystem/GlobalEnums.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"

void AT3SelectClassGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
}

void AT3SelectClassGameMode::MakeFirstGameData(const FString& PlayerName, const ECharacterClass SelectedPlayerClass)
{
	//첫 게임 데이터 생성
	const TObjectPtr<UT3SaveGame> FirstGameData = T3GameInstance->MakeFirstGameData(SelectedPlayerClass);
	FirstGameData->PlayerName = PlayerName;
	//무기 테이블을 참고하여 추가로 초기화
	if (WeaponTable)
	{
		if (const FT3WeaponDataRow* WeaponRow = WeaponTable->FindRow<FT3WeaponDataRow>(RowNameInWeaponTable, TEXT("Calc")))
		{
			FirstGameData->WeaponAttackPower = WeaponRow->BaseAttackPower;
		}
	}
	
	//그 외 SaveGame기반 클래스 초기화
	T3GameInstance->MakeFirstLostMoneyData();
	T3GameInstance->MakeFirstObjectStateData();
	
	//모든 게임 데이터 저장
	T3GameInstance->SaveGame();
	T3GameInstance->SaveLostMoney();
	T3GameInstance->SaveObjectState();
}

void AT3SelectClassGameMode::TutorialStart_Implementation()
{
	//T3GameInstance->OpenLevel(ELevelName::Tutorial);
}

void AT3SelectClassGameMode::ReturnToTitleLevel()
{
	T3GameInstance->OpenLevel(ELevelName::Title);
}
