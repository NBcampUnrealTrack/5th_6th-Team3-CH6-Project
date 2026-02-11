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
	
	const FString CHECK_NEW_GAME = TEXT("처음부터 하시겠습니까?\n저장된 게임이 사라집니다.");
	const FString QUIT_GAME_STRING = TEXT("종료하시겠습니까?");
};
