#include "UI/T3InputNamePanel.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "UI/T3SelectClassWidget.h"

void UT3InputNamePanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	//버튼 바인딩
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::OnClickConfirmButton);
	CancelButton->OnClicked.AddDynamic(this, &ThisClass::OnClickCancelButton);
	
	//이름 입력 칸 초기화
	InputNameBox->OnTextCommitted.AddDynamic(this, &ThisClass::OnTextCommitted);
	InputNameBox->SetText(FText::GetEmpty());
}

void UT3InputNamePanel::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	//엔터 감지시 확인 버튼을 누른 것으로 취급
	if (CommitMethod == ETextCommit::Type::OnEnter)
	{
		OnClickConfirmButton();
	}
}

void UT3InputNamePanel::OnClickConfirmButton()
{
	//이름 미입력시 무시
	const FString InputName = InputNameBox->GetText().ToString();
	if (InputName.IsEmpty())
	{
		return;
	}
	
	//튜토리얼 시작
	if (const TObjectPtr<UT3SelectClassWidget> WidgetInstance = SelectClassWidget.Get())
	{
		WidgetInstance->TutorialStart(InputName);
	}
}

void UT3InputNamePanel::OnClickCancelButton()
{
	if (const TObjectPtr<UT3SelectClassWidget> WidgetInstance = SelectClassWidget.Get())
	{
		WidgetInstance->SetActiveInputNamePanel(false);
	}
}
