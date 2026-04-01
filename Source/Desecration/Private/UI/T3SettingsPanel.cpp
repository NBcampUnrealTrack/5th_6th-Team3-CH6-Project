#include "UI/T3SettingsPanel.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "GameSystem/T3GameInstance.h"
#include "UI/T3SettingsPanelCategory.h"

void UT3SettingsPanel::NativeConstruct()
{
	Super::NativeConstruct();
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance가 NULL"), *GetNameSafe(this));
		return;
	}
	
	//각 범주별 설정 위젯을 확인
	for (TObjectPtr<UWidget> ChildWidget : SettingCategoriesParent->GetAllChildren())
	{
		TObjectPtr<UT3SettingsPanelCategory> CategoryWidget = Cast<UT3SettingsPanelCategory>(ChildWidget);
		if (!CategoryWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("%s : SettingCategoriesParent에 T3SettingsPanelCategory이 아닌 위젯이 있음"), *GetNameSafe(this));
			return;
		}
		
		CategoryWidgets.Add(CategoryWidget);
		
		//각 범주별 초기화 진행
		CategoryWidget->T3GameInstance = T3GameInstance;
		CategoryWidget->SettingsPanel = this;
		CategoryWidget->CustomNativeConstruct();
	}
	
	//상단 탭 버튼
	TArray<UWidget*> ChildrenWidget = TabButtonsBox->GetAllChildren();
	if (ChildrenWidget.Num() != CategoryWidgets.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 탭 버튼의 개수와 확인된 설정 범주 위젯의 개수가 다름"), *GetNameSafe(this));
		return;
	}
	//탭 버튼마다 기능 부여
	int32 PanelNum = 0;
	for (TObjectPtr<UWidget> ChildWidget : ChildrenWidget)
	{
		TObjectPtr<UButton> ChildButton = Cast<UButton>(ChildWidget);
		if (!ChildButton)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s : 버튼이 아님"), *ChildWidget.GetName());
			continue;
		}
		TabButtons.Add(ChildButton);

		const TSharedPtr<SButton> SlateButton = StaticCastSharedPtr<SButton>(ChildButton->GetCachedWidget());
		SlateButton->SetOnClicked(FOnClicked::CreateLambda([this, PanelNum]()
		{
			OnClickTabButton(PanelNum);
			return FReply::Handled(); 
		}));
		
		++PanelNum;
	}
	
	//나머지 버튼 바인딩
	CloseButton->OnClicked.AddDynamic(this, &ThisClass::OnClickConfirmButton);
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::OnClickConfirmButton);
	
	//하위 설정 위젯을 모두 숨김
	CurrentPanelNum = -1;
	for (const TObjectPtr CategoryWidget : CategoryWidgets)
	{
		if (!CategoryWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("%s : CategoryWidgets에 NULL인 항목이 있음"), *GetNameSafe(this));
			return;
		}
		CategoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	//이 패널도 숨김 상태로 시작
	SetVisibility(ESlateVisibility::Collapsed);
}

void UT3SettingsPanel::OnClickTabButton(const int32 PanelNum)
{
	//동일 버튼 무시
	if (PanelNum == CurrentPanelNum)
	{
		return;
	}
	
	//이전 버튼 및 패널에 대한 처리
	if (TabButtons.IsValidIndex(CurrentPanelNum))
	{
		TabButtons[CurrentPanelNum]->SetIsEnabled(true);
		CategoryWidgets[CurrentPanelNum]->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	//누른 버튼에 대한 패널 열기
	if (TabButtons.IsValidIndex(PanelNum))
	{
		TabButtons[PanelNum]->SetIsEnabled(false);
		CategoryWidgets[PanelNum]->SetVisibility(ESlateVisibility::Visible);
		CurrentPanelNum = PanelNum;
	}
}

void UT3SettingsPanel::OnClickConfirmButton()
{
	//패널 닫기
	SetVisibility(ESlateVisibility::Collapsed);
	
	//각 범주별 저장 작업
	for (const TObjectPtr CategoryWidget : CategoryWidgets)
	{
		if (!CategoryWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("%s : CategoryWidgets에 NULL인 항목이 있음"), *GetNameSafe(this));
			return;
		}
		
		CategoryWidget->SaveSettings();
	}
	
	//패널을 닫을 때 해야할 일
	OnClosePanel.ExecuteIfBound();
}

void UT3SettingsPanel::OpenSettingsPanel()
{
	//각 범주별 초기화 진행
	for (const TObjectPtr CategoryWidget : CategoryWidgets)
	{
		if (!CategoryWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("%s : CategoryWidgets에 NULL인 항목이 있음"), *GetNameSafe(this));
			return;
		}
		
		CategoryWidget->InitializeSettingsPanel();
	}
	
	//1번째 위젯을 연다.
	//TODO : 버그 수정 후 첫 번째(0)를 여는 것으로 수정
	OnClickTabButton(1);
	
	//패널 활성화
	SetVisibility(ESlateVisibility::Visible);
}

void UT3SettingsPanel::ApplyChangeLanguage()
{
	//각 범주별 언어 변경
	for (const TObjectPtr CategoryWidget : CategoryWidgets)
	{
		if (!CategoryWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("%s : CategoryWidgets에 NULL인 항목이 있음"), *GetNameSafe(this));
			return;
		}
		
		CategoryWidget->ReinitializeByChangeLanguage();
	}
	
	//언어 변경시 실행할 내용
	OnChangeLanguage.Broadcast();
}
