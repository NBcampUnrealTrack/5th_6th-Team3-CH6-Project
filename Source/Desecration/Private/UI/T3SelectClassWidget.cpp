#include "UI/T3SelectClassWidget.h"

#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "GameSystem/T3GameInstance.h"
#include "Player/T3SelectClassPlayerController.h"
#include "UI/T3InputNamePanel.h"

void UT3SelectClassWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance가 NULL"), *GetNameSafe(this));
		return;
	}
	
	//플레이어 컨트롤러
	SelectClassPlayerController = Cast<AT3SelectClassPlayerController>(GetOwningPlayer());
	if (!SelectClassPlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : SelectClassPlayerController가 NULL"), *GetNameSafe(this));
		return;
	}
	
	//각 클래스 버튼은 SButton에 Lambda로 바인딩한다.
	int32 TempClassValue = static_cast<int32>(EPlayerClass::Warrior);
	for (TObjectPtr<UWidget> ChildWidget : SelectClassBox->GetAllChildren())
	{
		TObjectPtr<UButton> ChildButton = Cast<UButton>(ChildWidget);
		if (!ChildButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s : 버튼이 아님"), *ChildWidget.GetName());
			continue;
		}
		TSharedPtr<SButton> SlateButton = StaticCastSharedPtr<SButton>(ChildButton->GetCachedWidget());
		SlateButton->SetOnClicked(FOnClicked::CreateLambda([this, TempClassValue]()
		{
			OnClickSelectClassButton(static_cast<EPlayerClass>(TempClassValue));
			return FReply::Handled(); 
		}));
		
		++TempClassValue;
	}
	
	//선택 버튼은 비활성화 상태로 시작
	SetActiveInputNamePanel(false);
	
	//이름 입력 패널 초기화
	InputNamePanel->SelectClassWidget = this;
	InputNamePanelParent->SetVisibility(ESlateVisibility::Collapsed);
	
	//그외 나머지 초기화
	SelectedPlayerClass = EPlayerClass::None;
}

void UT3SelectClassWidget::SetActiveInputNamePanel(bool bActive)
{
	InputNamePanelParent->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UT3SelectClassWidget::TutorialStart(const FString& PlayerName)
{
	SelectClassPlayerController->TutorialStart(PlayerName, SelectedPlayerClass);
}

void UT3SelectClassWidget::OnClickSelectClassButton(const EPlayerClass ButtonValue)
{
	//연속으로 같은 버튼을 누르면 이름 입력하기
	if (SelectedPlayerClass == ButtonValue)
	{
		SetActiveInputNamePanel(true);
		return;
	}
	
	//TODO : 누른 버튼의 변화
	SelectedPlayerClass = ButtonValue;
}

void UT3SelectClassWidget::ReturnToTitle()
{
	SelectClassPlayerController->ReturnToTitleLevel();
}
