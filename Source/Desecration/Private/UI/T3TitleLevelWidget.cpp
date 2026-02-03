#include "UI/T3TitleLevelWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3TitleGameMode.h"
#include "Player/T3TitlePlayerController.h"
#include "UI/T3ConfirmPanel.h"

void UT3TitleLevelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	//게임 모드
	TitleGameMode = Cast<AT3TitleGameMode>(GetWorld()->GetAuthGameMode());
	if (!TitleGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitleGameMode is NULL"), *GetNameSafe(this));
		return;
	}
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!TitleGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
	
	//플레이어 컨트롤러
	TitlePlayerController = Cast<AT3TitlePlayerController>(GetOwningPlayer());
	if (!TitlePlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitlePlayerController is NULL"), *GetNameSafe(this));
		return;
	}
	
	//버튼 바인딩
	NewGameButton->OnClicked.AddDynamic(this, &ThisClass::OnClickNewGameButton);
	SettingsButton->OnClicked.AddDynamic(this, &ThisClass::OnClickSettingsButton);
	QuitButton->OnClicked.AddDynamic(this, &ThisClass::OnClickQuitButton);
	//불러오기는 저장된 게임이 있을 때만 사용
	if (T3GameInstance->LoadGame())
	{
		LoadButton->OnClicked.AddDynamic(this, &ThisClass::OnClickLoadButton);
	}
	else
	{
		LoadButton->SetIsEnabled(false);
		LoadButtonText->SetIsEnabled(false);
	}
}

void UT3TitleLevelWidget::OnClickNewGameButton()
{
	//저장된 게임이 있는 경우 패널을 통해 물어보기
	if (T3GameInstance->LoadGame())
	{
		ConfirmPanel->ShowConfirmPanel(CHECK_NEW_GAME);
		ConfirmPanel->OnClickConfirmButtonAction.AddDynamic(TitleGameMode, &AT3TitleGameMode::MoveToSelectClassLevel);
		return;
	}
	
	TitleGameMode->MoveToSelectClassLevel();
}

void UT3TitleLevelWidget::OnClickLoadButton()
{
	//저장된 게임이 없다면 무시
	if (!T3GameInstance->LoadGame())
	{
		return;
	}
	
	ConfirmPanel->ShowConfirmPanel(TEXT("TODO : 저장된 게임 불러오기 문구 추가"));
	ConfirmPanel->OnClickConfirmButtonAction.AddDynamic(TitleGameMode, &AT3TitleGameMode::MoveToLastSavedLevel);
}

void UT3TitleLevelWidget::OnClickSettingsButton()
{
	TitlePlayerController->SetActiveSettingsPanel(true);
}

void UT3TitleLevelWidget::OnClickQuitButton()
{
	ConfirmPanel->ShowConfirmPanel(QUIT_GAME_STRING);
	ConfirmPanel->OnClickConfirmButtonAction.AddDynamic(TitleGameMode, &AT3TitleGameMode::QuitGame);
}
