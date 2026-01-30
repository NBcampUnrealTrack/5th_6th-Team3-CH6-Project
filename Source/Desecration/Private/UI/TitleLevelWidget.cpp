#include "UI/TitleLevelWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3TitleGameMode.h"

void UTitleLevelWidget::NativeConstruct()
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

void UTitleLevelWidget::OnClickNewGameButton()
{
	//저장된 게임이 있는 경우 패널을 통해 물어보기
	if (T3GameInstance->LoadGame())
	{
		//TODO : 패널 띄우기
		return;
	}
	
	TitleGameMode->MoveToSelectClassLevel();
}

void UTitleLevelWidget::OnClickLoadButton()
{
	TitleGameMode->MoveToLastSavedLevel();
}

void UTitleLevelWidget::OnClickSettingsButton()
{
	//TODO : 플레이어 컨트롤러에서 세팅 화면 출력해주기
}

void UTitleLevelWidget::OnClickQuitButton()
{
	//TODO : 게임 종료를 위한 패널 띄우기
}
