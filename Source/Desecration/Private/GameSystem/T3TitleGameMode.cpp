#include "GameSystem/T3TitleGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void AT3TitleGameMode::MoveToSelectClassLevel()
{
	//TODO : 클래스 선택 맵으로 이동
	UGameplayStatics::OpenLevel(GetWorld(), CLASS_LEVEL_NAME);
}

void AT3TitleGameMode::MoveToLastSavedLevel()
{
	//TODO : 마지막 저장 장소인 맵으로 이동
}

void AT3TitleGameMode::QuitGame() const
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
