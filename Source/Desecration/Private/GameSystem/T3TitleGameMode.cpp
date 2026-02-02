#include "GameSystem/T3TitleGameMode.h"

#include "GameSystem/GlobalEnums.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void AT3TitleGameMode::MoveToSelectClassLevel()
{
	const FName LevelName = FName(UEnum::GetDisplayValueAsText(ELevelName::SelectClass).ToString());
	UGameplayStatics::OpenLevel(GetWorld(), LevelName);
}

void AT3TitleGameMode::MoveToLastSavedLevel()
{
	//TODO : 마지막 저장 장소인 맵으로 이동
}

void AT3TitleGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
