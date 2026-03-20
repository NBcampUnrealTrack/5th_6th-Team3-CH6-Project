#include "Player/T3SelectClassPlayerController.h"

#include "GameSystem/T3SaveGame.h"
#include "GameSystem/T3SelectClassGameMode.h"
#include "UI/T3SelectClassWidget.h"

void AT3SelectClassPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//마우스 사용
	bShowMouseCursor = true;
	const FInputModeGameAndUI InputModeGameAndUI;
	SetInputMode(InputModeGameAndUI);
	
	//게임 모드
	SelectClassGameMode = Cast<AT3SelectClassGameMode>(GetWorld()->GetAuthGameMode());
	if (!SelectClassGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitleGameMode is NULL"), *GetNameSafe(this));
		return;
	}
	
	//위젯 생성
	SelectClassWidgetInstance = CreateWidget<UT3SelectClassWidget>(this, SelectClassWidgetClass);
	if (!SelectClassWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : Failed to create TitleLevelWidget"), *GetNameSafe(this));
		return;
	}
	SelectClassWidgetInstance->AddToViewport();
}

void AT3SelectClassPlayerController::TutorialStart(const FString& PlayerName, const ECharacterClass SelectedPlayerClass)
{	
	SelectClassGameMode->MakeFirstGameData(PlayerName, SelectedPlayerClass);
	SelectClassGameMode->TutorialStart();
}

void AT3SelectClassPlayerController::ReturnToTitleLevel()
{
	SelectClassGameMode->ReturnToTitleLevel();
}
