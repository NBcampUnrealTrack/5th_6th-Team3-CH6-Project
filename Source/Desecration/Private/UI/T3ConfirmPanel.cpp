#include "UI/T3ConfirmPanel.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UT3ConfirmPanel::NativeConstruct()
{
	Super::NativeConstruct();
	
	//버튼 바인딩
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::OnClickConfirmButton);
	CancelButton->OnClicked.AddDynamic(this, &ThisClass::OnClickCancelButton);
	
	SetVisibility(ESlateVisibility::Collapsed);
}

void UT3ConfirmPanel::NativeDestruct()
{
	Super::NativeDestruct();
	
	OnClickConfirmButtonAction.Clear();
}

void UT3ConfirmPanel::ShowConfirmPanel(const FString& Content, const bool bNeedCancelButton)
{
	SetVisibility(ESlateVisibility::Visible);
	ContentText->SetText(FText::FromString(Content));
	
	//취소 버튼
	CancelButton->SetVisibility(bNeedCancelButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UT3ConfirmPanel::OnClickConfirmButton()
{
	OnClickConfirmButtonAction.Broadcast();
	
	OnClickCancelButton();
}

void UT3ConfirmPanel::OnClickCancelButton()
{
	OnClickConfirmButtonAction.Clear();//다음 사용을 위해 바인딩된 내용을 제거한다.
	SetVisibility(ESlateVisibility::Collapsed);
}
