#include "UI/T3TitleLevelWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameSystem/T3GameInstance.h"
#include "Player/T3TitlePlayerController.h"
#include "UI/T3ConfirmPanel.h"
#include "UI/T3SettingsPanel.h"

void UT3TitleLevelWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
	if (TitlePlayerController->DoesSavedDataExist())
	{
		LoadButton->OnClicked.AddDynamic(this, &ThisClass::OnClickLoadButton);
	}
	else
	{
		LoadButton->SetIsEnabled(false);
	}
	
	//설정 패널을 닫으면 타이틀 버튼을 복구
	SettingsPanel->OnClosePanel.BindUObject(this, &ThisClass::RestoreTitleButtons);
}

void UT3TitleLevelWidget::OnClickNewGameButton()
{
	//저장된 게임이 있는 경우 패널을 통해 물어보기
	if (TitlePlayerController->DoesSavedDataExist())
	{
		ConfirmPanel->ShowConfirmPanel(UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, CheckNewGame));
		ConfirmPanel->OnClickConfirmButtonAction.AddDynamic(TitlePlayerController, &AT3TitlePlayerController::MoveToSelectClassLevel);
		return;
	}
	
	TitlePlayerController->MoveToSelectClassLevel();
}

void UT3TitleLevelWidget::OnClickLoadButton()
{
	//저장된 게임이 없다면 무시
	if (!TitlePlayerController->DoesSavedDataExist())
	{
		return;
	}
	
	ConfirmPanel->ShowConfirmPanel(UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, LoadGameMessage));
	ConfirmPanel->OnClickConfirmButtonAction.AddDynamic(TitlePlayerController, &AT3TitlePlayerController::MoveToLastSavedLevel);
}

void UT3TitleLevelWidget::OnClickSettingsButton()
{
	SettingsPanel->OpenSettingsPanel();
	TitleButtonsBox->SetVisibility(ESlateVisibility::Hidden);
}

void UT3TitleLevelWidget::OnClickQuitButton()
{
	ConfirmPanel->ShowConfirmPanel(UT3GameInstance::GetStringFromTable(NAMESPACE_NAME, QuitGameMessage));
	ConfirmPanel->OnClickConfirmButtonAction.AddDynamic(TitlePlayerController, &AT3TitlePlayerController::QuitGame);
}

void UT3TitleLevelWidget::RestoreTitleButtons()
{
	TitleButtonsBox->SetVisibility(ESlateVisibility::Visible);
}
