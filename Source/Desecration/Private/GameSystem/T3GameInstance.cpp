#include "GameSystem/T3GameInstance.h"

#include "GameFramework/GameUserSettings.h"
#include "GameSystem/T3SaveGame.h"
#include "GameSystem/T3SaveLostMoney.h"
#include "GameSystem/T3SaveObjectState.h"
#include "GameSystem/T3SaveUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Player/T3CharacterDataAsset.h"
#include "Sound/SoundClass.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h" // Slate 관련

void UT3GameInstance::StartCustomLoading()
{
    if (LoadingWidgetClass && !LoadingWidgetInstance)
    {
        // 1. 위젯 생성
        LoadingWidgetInstance = CreateWidget<UUserWidget>(this, LoadingWidgetClass);

        if (LoadingWidgetInstance && GEngine && GEngine->GameViewport)
        {
            // 2. AddToViewport 대신 GameViewport에 직접 추가 (레벨 전환 시 파괴 방지)
            GEngine->GameViewport->AddViewportWidgetContent(
                LoadingWidgetInstance->TakeWidget(),
                9999999 // 높은 ZOrder 설정
            );
            
            UE_LOG(LogTemp, Log, TEXT("Custom Loading UI Added to Viewport"));
        }
    }
}

void UT3GameInstance::EndCustomLoading()
{
    if (LoadingWidgetInstance && GEngine && GEngine->GameViewport)
    {
        // 3. 명시적으로 뷰포트에서 제거 (TakeWidget으로 가져왔던 Slate를 제거)
        GEngine->GameViewport->RemoveViewportWidgetContent(LoadingWidgetInstance->TakeWidget());
        
        // 4. 참조 해제 (메모리 정리)
        LoadingWidgetInstance = nullptr;
        
        UE_LOG(LogTemp, Log, TEXT("Custom Loading UI Removed"));
    }
}


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
	
	//잃어버린 재화
	if (LoadLostMoney())
	{
		MakeFirstLostMoneyData();
	}
	
	//물체 상태
	if (LoadObjectState())
	{
		MakeFirstObjectStateData();
	}
	
	//설정
	if (!LoadUserSettings())
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
	CurrentSettings->ResetGameData();

	//회전 감도는 1을 기본으로
	CurrentSettings->CameraSpeed = 1.0f;
	
	SaveUserSettings();
}

TObjectPtr<UT3SaveGame> UT3GameInstance::MakeFirstGameData(const ECharacterClass SelectedPlayerClass)
{
	if (!SavedGameData)
	{
		SavedGameData = NewObject<UT3SaveGame>();
	}
	SavedGameData->ResetGameData();
	if (const int32 ArrayIndex = static_cast<int32>(SelectedPlayerClass); CharacterDataList.IsValidIndex(ArrayIndex))
	{
		SavedGameData->SetStatByCharacterData(CharacterDataList[ArrayIndex]);
	}
	
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

void UT3GameInstance::MakeFirstObjectStateData()
{
	if (!ObjectStateData)
	{
		ObjectStateData = NewObject<UT3SaveObjectState>();
	}
	ObjectStateData->ResetGameData();
}

bool UT3GameInstance::SaveGame()
{
	return UGameplayStatics::SaveGameToSlot(SavedGameData, SAVE_GAME_NAME, 0);
}

bool UT3GameInstance::LoadGame(const bool bSetLocationAfterLoad)
{
	TObjectPtr<UT3SaveGame> SavedData = Cast<UT3SaveGame>(UGameplayStatics::LoadGameFromSlot(SAVE_GAME_NAME, 0));
	if (!SavedData)
	{
		return false;
	}
	
	SavedGameData = SavedData;
	SavedGameData->bSetLocation = bSetLocationAfterLoad;
	return true;
}

bool UT3GameInstance::SaveUserSettings()
{
	return UGameplayStatics::SaveGameToSlot(CurrentSettings, SAVE_USER_SETTINGS_NAME, 0);
}

bool UT3GameInstance::LoadUserSettings()
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

bool UT3GameInstance::SaveObjectState()
{
	return UGameplayStatics::SaveGameToSlot(ObjectStateData, SAVE_OBJECT_STATE_NAME, 0);
}

bool UT3GameInstance::LoadObjectState()
{
	TObjectPtr<UT3SaveObjectState> T3ObjectState = Cast<UT3SaveObjectState>(UGameplayStatics::LoadGameFromSlot(SAVE_LOST_MONEY_NAME, 0));
	if (!T3ObjectState)
	{
		return false;
	}
	
	ObjectStateData = T3ObjectState;
	return true;
}

TObjectPtr<UT3CharacterDataAsset> UT3GameInstance::GetCharacterDataAsset()
{
	if (!SavedGameData)
	{
		return nullptr;
	}
	
	const ECharacterClass CharacterClass = SavedGameData->PlayerClass;
	if (const int32 IndexNum = static_cast<int32>(CharacterClass); CharacterDataList.IsValidIndex(IndexNum))
	{
		return CharacterDataList[IndexNum];
	}
	
	return nullptr;
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

// ===== 레벨 해금 =====
void UT3GameInstance::UnlockLevel(const ELevelName LevelName)
{
	if (!SavedGameData)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return;
	}
	
	if (!SavedGameData->LevelProgressMap.Contains(LevelName))
	{
		FLevelProgressData NewData;
		NewData.bLevelUnlocked = true;
		SavedGameData->LevelProgressMap.Add(LevelName, NewData);
	}
	else
	{
		SavedGameData->LevelProgressMap[LevelName].bLevelUnlocked = true;
	}
	if (SaveGame())
	{
		UE_LOG(LogTemp, Log, TEXT("Level %d Unlocked and Saved Successfully!"), (int32)LevelName);
	}
}

// ===== 세이브포인트 해금 =====
void UT3GameInstance::UnlockSavePoint(ELevelName LevelName, FName SavePointID, FVector Location, FRotator Rotation)
{
	if (!SavedGameData)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return;
	}
	
    // 1. 해당 레벨 데이터가 없으면 새로 생성
	if (!SavedGameData->LevelProgressMap.Contains(LevelName))
	{
		FLevelProgressData NewLevelData;
		NewLevelData.bLevelUnlocked = true;
		SavedGameData->LevelProgressMap.Add(LevelName, NewLevelData);
	}

    // 2. 세이브 포인트 데이터 구성
    FSavePointData PointData;
    PointData.bIsUnlocked = true;
    PointData.SaveLocation = Location;
    PointData.SaveRotation = Rotation;

    // 3. 맵에 추가 또는 갱신
	SavedGameData->LevelProgressMap[LevelName].SavePoints.Add(SavePointID, PointData);
	
	SaveGame();
}

// ===== 레벨 해금 여부 =====
bool UT3GameInstance::IsLevelUnlocked(ELevelName LevelName)
{
	if (!SavedGameData)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return false;
	}
	
	if (SavedGameData->LevelProgressMap.Contains(LevelName))
	{
		return SavedGameData->LevelProgressMap[LevelName].bLevelUnlocked;
	}
	return false;
}

// ===== 세이브포인트 해금 여부 =====
bool UT3GameInstance::IsSavePointUnlocked(ELevelName LevelName, FName SavePointID)
{
	if (!SavedGameData)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return false;
	}
	
	if (SavedGameData->LevelProgressMap.Contains(LevelName))
	{
		if (const FSavePointData* PointData = SavedGameData->LevelProgressMap[LevelName].SavePoints.Find(SavePointID))
		{
			return PointData->bIsUnlocked;
		}
	}
	return false;
}

// ===== 특정 세이브 포인트 위치 정보 가져오기 (추가) =====
bool UT3GameInstance::GetSavePointTransform(ELevelName LevelName, FName SavePointID, FVector& OutLocation, FRotator& OutRotation)
{
	if (!SavedGameData)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 저장된 게임 데이터가 없음"), *GetNameSafe(this));
		return false;
	}
	
    if (SavedGameData->LevelProgressMap.Contains(LevelName))
    {
        if (const FSavePointData* PointData = SavedGameData->LevelProgressMap[LevelName].SavePoints.Find(SavePointID))
        {
            if (PointData->bIsUnlocked)
            {
                OutLocation = PointData->SaveLocation;
                OutRotation = PointData->SaveRotation;
                return true;
            }
        }
    }
    return false;
}


void UT3GameInstance::TravelToSavePoint(ELevelName LevelName, FName SavePointID)
{
    FVector Loc;
    FRotator Rot;

    // 1. 해당 세이브 포인트의 좌표 정보를 가져옴
    if (GetSavePointTransform(LevelName, SavePointID, Loc, Rot))
    {
        // 2. 예약 정보 설정
        bPendingTeleport = true;
        TargetTeleportLocation = Loc;
        TargetTeleportRotation = Rot;
        TargetLevelName = LevelName; // 로딩 맵에서 "어디로 가야 하는지" 알기 위해 저장

        // 3. 로딩 맵으로 이동
        // 주의: ELevelName에 LoadingLevel 항목이 있다면 그걸 사용하시고, 
        // 없다면 직접 FName(TEXT("T3LoadingLevel"))을 넣으셔도 됩니다.
        // 여기서는 일반적인 OpenLevel 방식을 빌려 호출합니다.
        UWorld* CurrentWorld = GetWorld();
        if (CurrentWorld)
        {
            UE_LOG(LogTemp, Warning, TEXT("Moving to Loading Level..."));
            UGameplayStatics::OpenLevel(CurrentWorld, TEXT("T3LoadingLevel"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SavePoint %s in Level %d not found!"), *SavePointID.ToString(), (int32)LevelName);
    }
}
