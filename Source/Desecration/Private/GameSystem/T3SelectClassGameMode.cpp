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

void AT3SelectClassGameMode::MakeFirstGameData(const FText& PlayerName, const EPlayerClass SelectedPlayerClass)
{
	//첫 게임 데이터 생성
	TObjectPtr<UT3SaveGame> FirstGameData = T3GameInstance->MakeFirstGameData();
	FirstGameData->PlayerClass = SelectedPlayerClass;
	FirstGameData->PlayerName = PlayerName;
	
	//게임 데이터 저장
	T3GameInstance->SaveGame();
}

void AT3SelectClassGameMode::TutorialStart()
{
	T3GameInstance->OpenLevel(ELevelName::Tutorial);
}

void AT3SelectClassGameMode::ReturnToTitleLevel()
{
	T3GameInstance->OpenLevel(ELevelName::Title);
}
