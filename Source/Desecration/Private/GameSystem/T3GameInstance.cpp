#include "GameSystem/T3GameInstance.h"

#include "GameFramework/GameUserSettings.h"
#include "GameSystem/T3SaveGame.h"
#include "GameSystem/T3SaveLostMoney.h"
#include "GameSystem/T3SaveUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"

FText UT3GameInstance::GetTextFromTable(const FString& Namespace, const FString& Key)
{
	FText ReturnValue;
	FText::FindTextInLiveTable_Advanced(FTextKey(Namespace), FTextKey(Key), ReturnValue);
	return ReturnValue;
}

FString UT3GameInstance::GetStringFromTable(const FString& Namespace, const FString& Key)
{
	return GetTextFromTable(Namespace, Key).ToString();
}

void UT3GameInstance::Init()
{
	Super::Init();
	
	//게임 데이터
	LoadGame();
	
	//설정
	if (!LoadUSerSettings())
	{
		MakeFirstSettings();
	}
}

void UT3GameInstance::MakeFirstSettings()
{
	//효과음, 배경음 모두 0.8을 기본으로
	SoundClassBGM->Properties.Volume = SoundClassSE->Properties.Volume = 0.8f;
	
	//그래픽 퀄리티는 높음
	if (GEngine)
	{
		GEngine->GetGameUserSettings()->SetOverallScalabilityLevel(2);
		GEngine->GetGameUserSettings()->ApplySettings(true);
	}
	
	//CurrentSettings를 사용하는 구간
	if (!CurrentSettings)
	{
		CurrentSettings = NewObject<UT3SaveUserSettings>();
	}
	CurrentSettings->ResetUserSettings();

	//회전 감도는 1을 기본으로
	CurrentSettings->CameraSpeed = 1.0f;
	
	SaveUserSettings();
}

TObjectPtr<UT3SaveGame> UT3GameInstance::MakeFirstGameData()
{
	if (!SavedGameData)
	{
		SavedGameData = NewObject<UT3SaveGame>();
	}
	SavedGameData->ResetGameData();
	
	return SavedGameData;
}

void UT3GameInstance::MakeFirstLostMoneyData()
{
	if (!LostMoneyData)
	{
		LostMoneyData = NewObject<UT3SaveLostMoney>();
	}
	LostMoneyData->ResetGameData();
}

bool UT3GameInstance::SaveGame()
{
	return UGameplayStatics::SaveGameToSlot(SavedGameData, SAVE_GAME_NAME, 0);
}

bool UT3GameInstance::LoadGame()
{
	TObjectPtr<UT3SaveGame> SavedData = Cast<UT3SaveGame>(UGameplayStatics::LoadGameFromSlot(SAVE_GAME_NAME, 0));
	if (!SavedData)
	{
		return false;
	}
	
	SavedGameData = SavedData;
	SavedGameData->bSetLocation = true;
	return true;
}

bool UT3GameInstance::SaveUserSettings()
{
	return UGameplayStatics::SaveGameToSlot(CurrentSettings, SAVE_USER_SETTINGS_NAME, 0);
}

bool UT3GameInstance::LoadUSerSettings()
{
	TObjectPtr<UT3SaveUserSettings> T3UserSettings = Cast<UT3SaveUserSettings>(UGameplayStatics::LoadGameFromSlot(SAVE_USER_SETTINGS_NAME, 0));
	if (!T3UserSettings)
	{
		return false;
	}
	
	CurrentSettings = T3UserSettings;
	return true;
}

bool UT3GameInstance::SaveLostMoney()
{
	return UGameplayStatics::SaveGameToSlot(LostMoneyData, SAVE_LOST_MONEY_NAME, 0);
}

bool UT3GameInstance::LoadLostMoney()
{
	TObjectPtr<UT3SaveLostMoney> T3LostMoney = Cast<UT3SaveLostMoney>(UGameplayStatics::LoadGameFromSlot(SAVE_LOST_MONEY_NAME, 0));
	if (!T3LostMoney)
	{
		return false;
	}
	
	LostMoneyData = T3LostMoney;
	return true;
}

//void UT3GameInstance::OpenLevel(const ELevelName LevelName) const
//{	
	//레벨 이동
	//const FName DisplayName = FName(UEnum::GetDisplayValueAsText(LevelName).ToString());
	//UGameplayStatics::OpenLevel(GetWorld(), DisplayName);
//}

// T3GameInstance.cpp


void UT3GameInstance::OpenLevel(const ELevelName LevelName)
{	
    // 1. LevelMap에 해당 키가 있는지 확인
    if (!LevelMap.Contains(LevelName))
    {
        UE_LOG(LogTemp, Error, TEXT("Level %d is not registered!"), (int32)LevelName);
        return;
    }

    // 2. 소프트 포인터로부터 경로 추출
    FString LevelPath = LevelMap[LevelName].GetLongPackageName();
    
    // 3. 경로가 비어있는지 확인 (패키징 시 데이터 유실 체크)
    if (LevelPath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("LevelPath is empty for Level %d!"), (int32)LevelName);
        return;
    }

    // 4. WorldContextObject 확인 (GetWorld()가 안전한지 체크)
    UWorld* CurrentWorld = GetWorld();
    if (!CurrentWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("GetWorld() returned NULL!"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Attempting to Open Level: %s"), *LevelPath);
	CurrentLevel = LevelName;
    
    // 최종 호출
    UGameplayStatics::OpenLevel(CurrentWorld, FName(*LevelPath));
}

void UT3GameInstance::OpenLevelBySavedData()
{
	if (SavedGameData)
	{
		OpenLevel(SavedGameData->SavedLevelName);
	}
}
