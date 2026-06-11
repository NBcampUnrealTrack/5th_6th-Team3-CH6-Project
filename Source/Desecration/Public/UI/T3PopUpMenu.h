#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3PopUpMenu.generated.h"

class UT3SettingsPanel;
class UT3GameInstance;
class AT3PlayerController;
class AT3GameMode;
class UT3ConfirmPanel;
class UButton;

UCLASS()
class DESECRATION_API UT3PopUpMenu : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
public:
	//팝업 메뉴 열기 또는 닫기
	UFUNCTION(BlueprintCallable)
	void SetActivePopUpMenu(bool bActive);
	
	//팝업 메뉴의 열림 여부
	UFUNCTION(BlueprintPure)
	bool IsActivePopUpMenu() const;
	
private:
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
	
	//설정 패널을 닫을 때 실행할 함수
	UFUNCTION()
	void OnCloseSettingsPanel();
	
	//타이틀로 나가기
	UFUNCTION()
	void GotoTitle();

	//메뉴 부분 전체를 가진 보더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UWidget> MenuBorder;
	
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
	
	//설정 패널
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UT3SettingsPanel> SettingsPanel;
	
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
	
	//타이틀 버튼의 메세지 (번역 키 값)
	UPROPERTY(EditDefaultsOnly, Category = "Message", meta = (AllowPrivateAccess = true))
	FString CheckGotoTitle;
	
	//번역 기능에 사용할 네임스페이스 이름
	const FString NAMESPACE_NAME = TEXT("ST_MainGame");
};
