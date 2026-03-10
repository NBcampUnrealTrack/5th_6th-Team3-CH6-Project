#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameSystem/GlobalEnums.h"
#include "T3SelectClassWidget.generated.h"

class UT3GameInstance;
class UT3InputNamePanel;
class UHorizontalBox;
class AT3SelectClassPlayerController;
class UButton;

UCLASS()
class DESECRATION_API UT3SelectClassWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	
public:
	//이름 입력 패널 활성화 여부
	void SetActiveInputNamePanel(bool bActive);

	//입력한 이름으로 튜토리얼 시작하기
	void TutorialStart(const FString& PlayerName);
	
protected:
	//블루프린트에서 Esc를 누를 때 실행할 함수
	UFUNCTION(BlueprintCallable)
	void OnPressEsc();
	
private:
	//클래스 선택 버튼
	void OnClickSelectClassButton(const EPlayerClass ButtonValue);
	
	//타이틀로 돌아가기
	UFUNCTION()
	void ReturnToTitle();

	//캐릭터 선택 버튼이 있는 패널
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UHorizontalBox> SelectClassBox;
	
	//이름 입력 패널이 있는 부모 위젯
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UWidget> InputNamePanelParent;
	
	//이름 입력 패널
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UT3InputNamePanel> InputNamePanel;
	
	//게임 인스턴스
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UT3GameInstance> T3GameInstance;
	
	//플레이어 컨트롤러
	UPROPERTY()
	TObjectPtr<AT3SelectClassPlayerController> SelectClassPlayerController;
	
	//마지막으로 선택한 클래스
	EPlayerClass SelectedPlayerClass;
};
