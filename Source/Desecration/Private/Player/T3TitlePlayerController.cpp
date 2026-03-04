#include "Player/T3TitlePlayerController.h"

#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3TitleGameMode.h"
#include "UI/T3SettingsPanel.h"
#include "UI/T3TitleLevelWidget.h"

void AT3TitlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//마우스 사용
	bShowMouseCursor = true;
	const FInputModeUIOnly InputModeUIOnly;
	SetInputMode(InputModeUIOnly);
	
	//게임 모드
	TitleGameMode = Cast<AT3TitleGameMode>(GetWorld()->GetAuthGameMode());
	if (!TitleGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitleGameMode is NULL"), *GetNameSafe(this));
		return;
	}
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
	
	//위젯 생성
	TitleLevelWidgetInstance = CreateWidget<UT3TitleLevelWidget>(this, TitleLevelWidgetClass);
	if (!TitleLevelWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : Failed to create TitleLevelWidget"), *GetNameSafe(this));
		return;
	}
	TitleLevelWidgetInstance->AddToViewport();
}

void AT3TitlePlayerController::MoveToSelectClassLevel()
{
	TitleGameMode->MoveToSelectClassLevel();
}

void AT3TitlePlayerController::MoveToLastSavedLevel()
{	
	TitleGameMode->MoveToLastSavedLevel();
}

bool AT3TitlePlayerController::DoesSavedDataExist()
{
	return T3GameInstance->GetSavedGameData() != nullptr;
}

void AT3TitlePlayerController::QuitGame()
{
	TitleGameMode->QuitGame();
}
