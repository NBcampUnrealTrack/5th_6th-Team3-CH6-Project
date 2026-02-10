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
	
	SettingsPanelInstance = CreateWidget<UT3SettingsPanel>(this, SettingsPanelClass);
	if (!SettingsPanelInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : Failed to create SettingsPanel"), *GetNameSafe(this));
		return;
	}
	SettingsPanelInstance->AddToViewport();
	
	//설정 패널은 숨김 상태로 시작
	SettingsPanelInstance->SetVisibility(ESlateVisibility::Collapsed);
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

void AT3TitlePlayerController::SetActiveSettingsPanel(bool bActive)
{
	//타이틀 레벨 위젯과 설정 패널 위젯은 서로 반대로 적용
	TitleLevelWidgetInstance->SetVisibility(bActive ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	SettingsPanelInstance->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	
	//설정 패널 활성화시 패널 내용 초기화
	//TODO : 패널 내용 초기화
}

void AT3TitlePlayerController::QuitGame()
{
	TitleGameMode->QuitGame();
}
