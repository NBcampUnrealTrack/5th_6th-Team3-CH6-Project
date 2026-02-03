#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3SettingsPanel.generated.h"

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
	
public:
	virtual void NativeConstruct() override;
	
private:
	//배경음 슬라이더
	UFUNCTION()
	void WhileMovingBGMSlider(float value);
	
	//효과음 슬라이더
	UFUNCTION()
	void WhileMovingSESlider(float value);
	
	//마우스 감도 슬라이더
	UFUNCTION()
	void WhileMovingMouseSensitivitySlider(float value);
	
	//해상도 콤보박스
	UFUNCTION()
	void OnSelectResolutionComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	//화면 모드 콤보박스
	UFUNCTION()
	void OnSelectScreenModeComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	//그래픽 퀄리티 콤보박스
	UFUNCTION()
	void OnSelectGraphicQualityComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	//확인 버튼
	UFUNCTION()
	void OnClickConfirmButton();
	
	//배경음 슬라이더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> BGMSlider;
	
	//효과음 슬라이더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> SESlider;
	
	//마우스 감도 슬라이더
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<USlider> MouseSensitivitySlider;
	
	//해상도 콤보박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ResolutionComboBox;
	
	//화면 모드 콤보박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ScreenModeComboBox;
	
	//그래픽 퀄리티 콤보박스
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> GraphicQualityComboBox;
	
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
