#include "GameSystem/T3TitleGameMode.h"

#include "GameSystem/GlobalEnums.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void AT3TitleGameMode::MoveToSelectClassLevel()
{
	const TObjectPtr<UEnum> EnumPtr = StaticEnum<ELevelName>();
	UGameplayStatics::OpenLevel(GetWorld(), EnumPtr->GetNameByIndex(static_cast<int32>(ELevelName::SelectClass)));
}

void AT3TitleGameMode::MoveToLastSavedLevel()
{
	//TODO : 마지막 저장 장소인 맵으로 이동
}

void AT3TitleGameMode::QuitGame() const
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
