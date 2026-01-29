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
	
	//저장된 게임
	CurrentSaveGame = T3GameInstance->LoadGame();
	
	//버튼 바인딩
	NewGameButton->OnClicked.AddDynamic(this, &ThisClass::OnClickNewGameButton);
	SettingsButton->OnClicked.AddDynamic(this, &ThisClass::OnClickSettingsButton);
	QuitButton->OnClicked.AddDynamic(this, &ThisClass::OnClickQuitButton);
	//불러오기는 저장된 게임이 있을 때만 사용
	if (CurrentSaveGame)
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
	
}

void UTitleLevelWidget::OnClickLoadButton()
{
	
}

void UTitleLevelWidget::OnClickSettingsButton()
{
	
}

void UTitleLevelWidget::OnClickQuitButton()
{
	
}
