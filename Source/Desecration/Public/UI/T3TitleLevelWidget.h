#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3TitleLevelWidget.generated.h"

class UT3ConfirmPanel;
class AT3TitlePlayerController;
class UTextBlock;
class UButton;

UCLASS()
class DESECRATION_API UT3TitleLevelWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
private:
	//새로하기
	UFUNCTION()
	void OnClickNewGameButton();
	
	//이어하기
	UFUNCTION()
	void OnClickLoadButton();
	
	//설정
	UFUNCTION()
	void OnClickSettingsButton();
	
	//나가기
	UFUNCTION()
	void OnClickQuitButton();
	
	//새로하기 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> NewGameButton;
	
	//이어하기 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> LoadButton;
	
	//설정 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> SettingsButton;
	
	//나가기 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> QuitButton;
	
	//확인 패널
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UT3ConfirmPanel> ConfirmPanel;

	//풀레이어 컨트롤러
	UPROPERTY()
	TObjectPtr<AT3TitlePlayerController> TitlePlayerController;
	
	//저장된 데이터가 있을 때 새로하기 버튼을 누르면 나오는 메세지
	UPROPERTY(EditDefaultsOnly, Category = "Message", meta = (AllowPrivateAccess = true))
	FString CheckNewGame;
	
	//게임 종료시의 메세지
	UPROPERTY(EditDefaultsOnly, Category = "Message", meta = (AllowPrivateAccess = true))
	FString QuitGameMessage;
};
