#include "UI/T3PopUpMenu.h"

#include "Components/Button.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3GameMode.h"
#include "Player/T3PlayerController.h"
#include "UI/T3ConfirmPanel.h"

void UT3PopUpMenu::NativeConstruct()
{
	Super::NativeConstruct();
	
	//플레이어 컨트롤러
	T3PlayerController = Cast<AT3PlayerController>(GetOwningPlayer());
	if (!T3PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3PlayerController is NULL"), *GetNameSafe(this));
		return;
	}
	
	//게임 모드
	T3GameMode = Cast<AT3GameMode>(GetWorld()->GetAuthGameMode());
	if (!T3GameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameMode is NULL"), *GetNameSafe(this));
		return;
	}
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
	
	//버튼 바인딩
	ResumeButton->OnClicked.AddDynamic(this, &ThisClass::OnClickResumeButton);
	LoadButton->OnClicked.AddDynamic(this, &ThisClass::OnClickLoadButton);
	SettingsButton->OnClicked.AddDynamic(this, &ThisClass::OnClickSettingsButton);
	TitleButton->OnClicked.AddDynamic(this, &ThisClass::OnClickTitleButton);
	
	//위젯은 닫힌 상태로 시작
	ConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UT3PopUpMenu::SetActivePopUpMenu(const bool bActive)
{
	//열기 여부에 따라 UI/게임 전용으로 변경
	if (bActive)
	{
		const FInputModeUIOnly UIOnly;
		T3PlayerController->SetInputMode(UIOnly);
	}
	else
	{
		const FInputModeGameOnly GameOnly;
		T3PlayerController->SetInputMode(GameOnly);
	}
	
	SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UT3PopUpMenu::OnClickResumeButton()
{
	SetActivePopUpMenu(false);
}

void UT3PopUpMenu::OnClickLoadButton()
{
	T3GameMode->LoadGame();
}

void UT3PopUpMenu::OnClickSettingsButton()
{
	
}

void UT3PopUpMenu::OnClickTitleButton()
{
	ConfirmPanel->ShowConfirmPanel(CheckGotoTitle);
	ConfirmPanel->OnClickConfirmButtonAction.AddDynamic(this, &ThisClass::GotoTitle);
}

void UT3PopUpMenu::GotoTitle()
{
	T3GameInstance->OpenLevel(ELevelName::Title);
}
