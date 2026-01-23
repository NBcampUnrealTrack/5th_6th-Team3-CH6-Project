#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SettingEnums.h"
#include "T3GameInstance.generated.h"

class UT3SaveGame;

//설정값을 저장하는 구조체
struct FSettings
{
	EResolution Resolution;
	EScreenMode ScreenMode;
	float SoundEffectsVolume;
	float BackgroundVolume;
};

UCLASS()
class DESECRATION_API UT3GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;
	
	//저장된 게임
	FObjectPtr<UT3SaveGame> LoadGame();
	
	//게임 저장하기 (true : 저장 성공)
	//TODO : 게임 저장을 위한 매개변수 추가
	bool SaveGame();
	
	//현재 설정
	FORCEINLINE TSharedPtr<FSettings> GetCurrentSettings() const { return CurrentSettings; }

private:
	//현재 설정
	TSharedPtr<FSettings> CurrentSettings;
	
	//저장된 게임 데이터
	FObjectPtr<UT3SaveGame> SavedGameData;
	
	const FString SAVE_GAME_NAME = TEXT("SaveSlot1");
};
