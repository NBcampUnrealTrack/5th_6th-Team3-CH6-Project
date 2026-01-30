#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3TitleLevelWidget.generated.h"

class UTextBlock;
class UT3SaveGame;
class UT3GameInstance;
class AT3TitleGameMode;
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
	
	//이어하기 버튼의 텍스트
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UTextBlock> LoadButtonText;
	
	//설정 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> SettingsButton;
	
	//나가기 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> QuitButton;
	
	//게임 모드
	UPROPERTY()
	TObjectPtr<AT3TitleGameMode> TitleGameMode;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
