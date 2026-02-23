#include "GameSystem/T3GameInstance.h"

#include "GameFramework/GameUserSettings.h"
#include "GameSystem/T3SaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"

void UT3GameInstance::Init()
{
	Super::Init();
	
	CurrentSettings = TSharedPtr<FSettings>();
	
	if (!GEngine)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : GEngine이 null"), *GetNameSafe(this));
		return;
	}
	UserSettings = GEngine->GetGameUserSettings();
	if (!UserSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : GEngine의 GameUserSettings가 null"), *GetNameSafe(this));
		return;
	}
	
	LoadGame();
}

void UT3GameInstance::MakeFirstSettings()
{
	//현 모니터의 최고 크기에 전체 창모드를 기본으로
	FIntPoint MaxResolution;
	if (TArray<FIntPoint> Resolutions; UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions))
	{
		MaxResolution = Resolutions.Last();
		ET3Resolution SuitableValue = static_cast<ET3Resolution>(MaxResolution.X * 10000 + MaxResolution.Y);
		CurrentSettings->Resolution = SuitableValue;
		CurrentSettings->ScreenMode = ET3ScreenMode::WindowedFullscreen;
	}
	else//최대 해상도 확인 실패시 최소 해상도에 창모드로
	{
		MaxResolution = FIntPoint(800, 600);
		CurrentSettings->Resolution = ET3Resolution::W800H600;
		CurrentSettings->ScreenMode = ET3ScreenMode::Windowed;
	}
	UserSettings->SetScreenResolution(MaxResolution);
	
	//그래픽 퀄리티는 낮음으로
	UserSettings->SetOverallScalabilityLevel(0);
	CurrentSettings->GraphicQuality = EGraphicQuality::Low;
	
	//화면 모드, 해상도 적용
	UserSettings->ApplySettings(true);
	
	//효과음, 배경음 모두 0.8을 기본으로
	CurrentSettings->SoundEffectsVolume = 0.8f;
	CurrentSettings->BackgroundVolume = 0.8f;
	//마우스 감도는 1을 기본으로
	CurrentSettings->MouseSensitivity = 1.0f;
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

void UT3GameInstance::SetResolution(ET3Resolution Resolution)
{
	CurrentSettings->Resolution = Resolution;
	
	//ET3Resolution의 각 항목은 [가로 * 10000 + 세로]인 값을 가진다.
	int32 Width = static_cast<int>(Resolution) / 10000;
	int32 Height = static_cast<int>(Resolution) % 10000;
	UserSettings->SetScreenResolution(FInt32Point(Width, Height));
	UserSettings->ApplySettings(true);
}

void UT3GameInstance::SetScreenMode(ET3ScreenMode ScreenMode)
{
	CurrentSettings->ScreenMode = ScreenMode;
	UserSettings->SetFullscreenMode(static_cast<EWindowMode::Type>(ScreenMode));
	UserSettings->ApplySettings(true);
	
#if WITH_EDITOR
	//화면 모드 변경은 에디터에서 알 수 없기 때문에 로그로 표시
	const FString LogOutValue = UEnum::GetValueAsString(ScreenMode);
	UE_LOG(LogTemp, Warning, TEXT("화면 모드 변경 : %s"), *LogOutValue);
#endif
}

void UT3GameInstance::SetSoundEffectsVolume(const float Volume)
{
	SoundClassSE->Properties.Volume = Volume;
	CurrentSettings->SoundEffectsVolume = Volume;
}

void UT3GameInstance::SetBackgroundVolume(const float Volume)
{
	SoundClassBGM->Properties.Volume = Volume;
	CurrentSettings->BackgroundVolume = Volume;
}

void UT3GameInstance::SetGraphicQuality(const EGraphicQuality GraphicQuality)
{
	CurrentSettings->GraphicQuality = GraphicQuality;
	UserSettings->SetOverallScalabilityLevel(static_cast<int32>(GraphicQuality));
	UserSettings->ApplySettings(true);
}

//void UT3GameInstance::OpenLevel(const ELevelName LevelName) const
//{	
	//레벨 이동
	//const FName DisplayName = FName(UEnum::GetDisplayValueAsText(LevelName).ToString());
	//UGameplayStatics::OpenLevel(GetWorld(), DisplayName);
//}

// T3GameInstance.cpp


void UT3GameInstance::OpenLevel(const ELevelName LevelName) const // const 유지
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
    
    // 최종 호출
    UGameplayStatics::OpenLevel(CurrentWorld, FName(*LevelPath));
}