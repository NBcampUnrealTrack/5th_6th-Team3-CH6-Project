#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanelCategory.h"
#include "T3DisplaySettings.generated.h"

//해상도 정보
USTRUCT()
struct FResolution
{
	GENERATED_BODY()
	
	FString Name;
	FIntPoint SizeInfo;
	
	FResolution()
	{
		Name = TEXT("None");
		SizeInfo = FIntPoint();
	}
	
	FResolution(const FString& Name, const int32 Width, const int32 Height)
	{
		this->Name = Name;
		SizeInfo = FIntPoint(Width, Height);
	}
};

UCLASS()
class DESECRATION_API UT3DisplaySettings : public UT3SettingsPanelCategory
{
	GENERATED_BODY()

protected:
	virtual void OnParentConstruct() override;
	virtual void InitializeSettingsPanel() override;
	virtual void ReinitializeByChangeLanguage() override;
	
private:	
	UFUNCTION()
	void OnSelectionChangedResolutionComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	UFUNCTION()
	void OnSelectionChangedScreenModeComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	UFUNCTION()
	void OnSelectionChangedGraphicQualityComboBox(FString SelectedItem, ESelectInfo::Type SelectionType);
	
	//해상도 설정하기
	void SetResolution(const FInt32Point Resolution);
	
	//화면 모드 설정하기
	void SetWindowMode(const EWindowMode::Type WindowMode);
	
	/**
	 * 그래픽 설정하기
	 * @param GraphicQuality 최소 0, 최대 4인 정수 (0 : 낮음, 1 : 중간, 2 : 높음, 3 : 에픽, 4 : 시네마틱)
	 */
	void SetGraphicQuality(const int32 GraphicQuality);
	
	//해상도 설정
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ResolutionComboBox;
	
	//화면 모드 설정
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> ScreenModeComboBox;
	
	//그래픽 품질 설정
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UComboBoxString> GraphicQualityComboBox;
	
	//엔진의 게임 설정
	UPROPERTY()
	TObjectPtr<UGameUserSettings> GameUserSettings;
	
	//해상도 목록
	const TArray<FResolution> RESOLUTION_LIST = 
	{
		FResolution(TEXT("800 x 600"), 800, 600),
		FResolution(TEXT("1024 x 768"), 1024, 768),
		FResolution(TEXT("1280 x 720"), 1280, 720),
		FResolution(TEXT("1280 x 800"), 1280, 800),
		FResolution(TEXT("1280 x 960"), 1280, 960),
		FResolution(TEXT("1366 x 768"), 1366, 768),
		FResolution(TEXT("1440 x 900"), 1440, 900),
		FResolution(TEXT("1600 x 900"), 1600, 900),
		FResolution(TEXT("1680 x 1050"), 1680, 1050),
		FResolution(TEXT("1920 x 1080"), 1920, 1080),
		FResolution(TEXT("1920 x 1200"), 1920, 1200)
	};
	
	//화면 모드에 사용할 번역 키 값
	const TArray<FString> SCREEN_MODE_KEY_STRINGS = { TEXT("WindowMode_Fullscreen"), TEXT("WindowMode_WindowedFullscreen"), TEXT("WindowMode_Windowed") };
	
	//그래픽 품질에 사용할 번역 키 값
	const TArray<FString> GRAPHIC_KEY_STRINGS =
		{ TEXT("Graphic_Low"), TEXT("Graphic_Medium"), TEXT("Graphic_High"), TEXT("Graphic_Epic"), TEXT("Graphic_Cinematic") };
};
