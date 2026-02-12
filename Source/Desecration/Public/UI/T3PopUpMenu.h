#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3PopUpMenu.generated.h"

class UT3GameInstance;
class AT3PlayerController;
class AT3GameMode;
class UT3ConfirmPanel;
class UButton;

UCLASS()
class DESECRATION_API UT3PopUpMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
private:
	//팝업 메뉴 열기 또는 닫기
	UFUNCTION(BlueprintCallable)
	void SetActivePopUpMenu(bool bActive);
	
	//계속하기
	UFUNCTION()
	void OnClickResumeButton();
	
	//이어하기
	UFUNCTION()
	void OnClickLoadButton();
	
	//설정
	UFUNCTION()
	void OnClickSettingsButton();
	
	//타이틀
	UFUNCTION()
	void OnClickTitleButton();
	
	//타이틀로 나가기
	UFUNCTION()
	void GotoTitle();
	
	//계속하기 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> ResumeButton;
	
	//이어하기 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> LoadButton;
	
	//설정 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> SettingsButton;
	
	//타이틀 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> TitleButton;
	
	//확인 패널
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UT3ConfirmPanel> ConfirmPanel;
	
	//플레이어 컨트롤러
	UPROPERTY()
	TObjectPtr<AT3PlayerController> T3PlayerController;
	
	//게임 모드
	UPROPERTY()
	TObjectPtr<AT3GameMode> T3GameMode;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
	
	//타이틀 버튼의 메세지
	UPROPERTY(EditDefaultsOnly, Category = "Message", meta = (AllowPrivateAccess = true))
	FString CheckGotoTitle;
};
