#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GlobalEnums.h"
#include "T3GameInstance.generated.h"

class UT3SaveGame;

//설정값을 저장하는 구조체
struct FSettings
{
	ET3Resolution Resolution;//해상도
	ET3ScreenMode ScreenMode;//화면모드
	EGraphicQuality GraphicQuality;//그래픽 퀄리티
	float SoundEffectsVolume;//효과음
	float BackgroundVolume;//배경음
	float MouseSensitivity;//마우스 감도
};

UCLASS()
class DESECRATION_API UT3GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;
	
private:
	//최초 설정값 생성
	void MakeFirstSettings();
	
public:
	//첫 게임 데이터 생성
	TObjectPtr<UT3SaveGame> MakeFirstGameData();
	
	//게임 저장하기 (true : 저장 성공)
	//TODO : 게임 저장을 위한 매개변수 추가
	bool SaveGame();
	
	//해상도 설정하기
	void SetResolution(ET3Resolution Resolution);
	
	//화면 모드 설정하기
	void SetScreenMode(ET3ScreenMode ScreenMode);
	
	//효과음 설정하기
	void SetSoundEffectsVolume(float Volume);
	
	//배경음 설정하기
	void SetBackgroundVolume(float Volume);
	
	//그래픽 설정하기
	void SetGraphicQuality(const EGraphicQuality GraphicQuality);
	
	//저장된 게임
	FORCEINLINE TObjectPtr<UT3SaveGame> LoadGame() { return SavedGameData; }
	
	//현재 설정
	FORCEINLINE TSharedPtr<FSettings> GetCurrentSettings() const { return CurrentSettings; }

private:
	//현재 설정
	TSharedPtr<FSettings> CurrentSettings;
	
	//엔진의 게임 설정
	UPROPERTY()
	TObjectPtr<UGameUserSettings> UserSettings;
	
	//저장된 게임 데이터
	UPROPERTY()
	TObjectPtr<UT3SaveGame> SavedGameData;
	
	//효과음
	UPROPERTY(EditDefaultsOnly, Category = "Sound Class", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundClass> SoundClassSE;
	
	//배경음
	UPROPERTY(EditDefaultsOnly, Category = "Sound Class", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundClass> SoundClassBGM;
	
	const FString SAVE_GAME_NAME = TEXT("SaveSlot1");
};
