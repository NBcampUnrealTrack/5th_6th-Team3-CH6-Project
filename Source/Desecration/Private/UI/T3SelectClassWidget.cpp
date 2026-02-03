#include "UI/T3SelectClassWidget.h"

#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Player/T3SelectClassPlayerController.h"

void UT3SelectClassWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	//플레이어 컨트롤러
	SelectClassPlayerController = Cast<AT3SelectClassPlayerController>(GetOwningPlayer());
	if (!SelectClassPlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : SelectClassPlayerController is NULL"), *GetNameSafe(this));
		return;
	}
	
	//버튼 바인딩
	SelectButton->OnClicked.AddDynamic(this, &ThisClass::OnClickSelectButton);
	ReturnButton->OnClicked.AddDynamic(this, &ThisClass::OnClickReturnButton);
	
	//선택 버튼은 비활성화 상태로 시작
	SelectButton->SetIsEnabled(false);
	
	//그외 나머지 초기화
	SelectedPlayerClass = EPlayerClass::None;
}

void UT3SelectClassWidget::OnClickSelectClassButton(const EPlayerClass ButtonValue)
{
	//연속으로 같은 버튼을 누르면 선택으로 간주
	if (SelectedPlayerClass == ButtonValue)
	{
		OnClickSelectButton();
		return;
	}
	
	//TODO : 누른 버튼의 변화
	SelectedPlayerClass = ButtonValue;
	
	SelectButton->SetIsEnabled(true);
}

void UT3SelectClassWidget::OnClickSelectButton()
{
	//클래스 미선택시 무시
	if (SelectedPlayerClass == EPlayerClass::None)
	{
		return;
	}
	
	//TODO : 이름 입력 패널 구현
	//TODO : 튜토리얼 시작 구현
}

void UT3SelectClassWidget::OnClickReturnButton()
{
	SelectClassPlayerController->ReturnToTitleLevel();
}
