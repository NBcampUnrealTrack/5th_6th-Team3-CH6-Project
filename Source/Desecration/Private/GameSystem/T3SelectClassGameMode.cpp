#include "GameSystem/T3SelectClassGameMode.h"

#include "GameSystem/GlobalEnums.h"
#include "Kismet/GameplayStatics.h"

bool AT3SelectClassGameMode::MakeFirstGameData()
{
	//TODO : 게임 데이터 생성
	
	return true;
}

void AT3SelectClassGameMode::TutorialStart()
{
	//TODO : 튜토리얼 레벨로 이동하기
}

void AT3SelectClassGameMode::ReturnToTitleLevel()
{
	const FName LevelName = FName(UEnum::GetDisplayValueAsText(ELevelName::Title).ToString());
	UGameplayStatics::OpenLevel(GetWorld(), LevelName);
}
