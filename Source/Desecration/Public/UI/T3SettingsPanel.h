#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3SettingsPanel.generated.h"

class UCanvasPanel;
class UHorizontalBox;
class UT3SettingsPanelCategory;
class AT3TitlePlayerController;
class UT3GameInstance;
class AT3TitleGameMode;
class UButton;
class UComboBoxString;
class USlider;

DECLARE_DELEGATE(FOnClosePanel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChangeLanguage);

UCLASS()
class DESECRATION_API UT3SettingsPanel : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SettingsPanelCategory;
	
protected:
	virtual void NativeOnInitialized() override;
	
private:
	//상단 탭 버튼에 대한 동작
	void OnClickTabButton(int32 PanelNum);
	
	//확인 버튼
	UFUNCTION()
	void OnClickConfirmButton();
	
public:
	//설정 패널 열기
	void OpenSettingsPanel();
	
	//언어 변경후 적용하기
	void ApplyChangeLanguage();
	
	//설정 패널 닫을 때 실행할 내용
	FOnClosePanel OnClosePanel;
	
	//언어 변경시 실행할 내용
	UPROPERTY(BlueprintAssignable)
	FOnChangeLanguage OnChangeLanguage;
	
private:
	//상단 탭 버튼이 있는 가로 박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UHorizontalBox> TabButtonsBox;
	
	//상단 탭 버튼
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> TabButtons;
	
	//닫기 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> CloseButton;
	
	//확인 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> ConfirmButton;

	//범주별로 나눈 설정 위젯들
	UPROPERTY(Transient)
	TArray<TObjectPtr<UT3SettingsPanelCategory>> CategoryWidgets;
	
	//설정 위젯의 부모
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UCanvasPanel> SettingCategoriesParent;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
	
	//현재 열린 카테고리 패널 번호
	int32 CurrentPanelNum;
};
