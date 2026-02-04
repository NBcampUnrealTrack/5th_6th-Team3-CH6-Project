#include "UI/T3InputNamePanel.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "UI/T3SelectClassWidget.h"

void UT3InputNamePanel::NativeConstruct()
{
	Super::NativeConstruct();
	
	//버튼 바인딩
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::OnClickConfirmButton);
	CancelButton->OnClicked.AddDynamic(this, &ThisClass::OnClickCancelButton);
}

void UT3InputNamePanel::OnClickConfirmButton()
{
	//이름 미입력시 무시
	const FText InputText = InputNameBox->GetText(); 
	if (InputText.IsEmpty())
	{
		return;
	}
	
	//튜토리얼 시작
	if (const TObjectPtr<UT3SelectClassWidget> WidgetInstance = SelectClassWidget.Get())
	{
		WidgetInstance->TutorialStart(InputText);
	}
}

void UT3InputNamePanel::OnClickCancelButton()
{
	if (const TObjectPtr<UT3SelectClassWidget> WidgetInstance = SelectClassWidget.Get())
	{
		WidgetInstance->SetActiveInputNamePanel(false);
	}
}
