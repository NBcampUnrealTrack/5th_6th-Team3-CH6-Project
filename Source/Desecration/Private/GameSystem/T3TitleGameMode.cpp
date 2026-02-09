#include "GameSystem/T3TitleGameMode.h"

#include "GameSystem/GlobalEnums.h"
#include "GameSystem/T3GameInstance.h"
#include "Kismet/KismetSystemLibrary.h"

void AT3TitleGameMode::BeginPlay()
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

void AT3TitleGameMode::MoveToSelectClassLevel()
{
	T3GameInstance->OpenLevel(ELevelName::SelectClass);
}

void AT3TitleGameMode::MoveToLastSavedLevel()
{
	//TODO : 마지막 저장 장소인 맵으로 이동
}

void AT3TitleGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
