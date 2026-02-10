#include "GameSystem/T3TitleGameMode.h"

#include "GameSystem/GlobalEnums.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"
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
	if (const TObjectPtr<UT3SaveGame> SavedGame = T3GameInstance->GetSavedGameData())
	{
		T3GameInstance->OpenLevel(SavedGame->SavedLevelName);
	}
}

void AT3TitleGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
