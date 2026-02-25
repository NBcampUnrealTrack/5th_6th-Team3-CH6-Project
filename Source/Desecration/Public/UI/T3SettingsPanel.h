#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3SettingsPanel.generated.h"

class UT3SettingsPanelCategory;
class AT3TitlePlayerController;
class UT3GameInstance;
class AT3TitleGameMode;
class UButton;
class UComboBoxString;
class USlider;

UCLASS()
class DESECRATION_API UT3SettingsPanel : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SettingsPanelCategory;
	
public:
	virtual void NativeConstruct() override;
	
private:
	//확인 버튼
	UFUNCTION()
	void OnClickConfirmButton();
	
	//확인 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> ConfirmButton;
	
	//게임 모드
	UPROPERTY()
	TObjectPtr<AT3TitleGameMode> TitleGameMode;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
	
	//풀레이어 컨트롤러
	UPROPERTY()
	TObjectPtr<AT3TitlePlayerController> TitlePlayerController;
};
