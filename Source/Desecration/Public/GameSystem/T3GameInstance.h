#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GlobalEnums.h"
#include "Blueprint/UserWidget.h"
#include "T3GameInstance.generated.h"

enum class ECharacterClass : uint8;
class UT3SaveUserSettings;
class UT3CharacterDataAsset;
class UT3SaveGame;

// 세이브 포인트의 상세 정보를 담는 구조체 추가
USTRUCT(BlueprintType)
struct FSavePointData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsUnlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SaveLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator SaveRotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FLevelProgressData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLevelUnlocked = false;

	// bool 대신 FSavePointData 구조체를 사용하도록 변경
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FSavePointData> SavePoints;
};

UCLASS()
class DESECRATION_API UT3GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	/** 
	 * 지정한 키를 번역하기
	 * @param Namespace 텍스트를 찾을 테이블의 네임스페이스
	 * @param Key 지정할 테이블에서 찾을 키 값
	 * @return 지정한 키 값의 번역 결과 (FText)
	 */
	UFUNCTION(Blueprintpure)
	static FText GetTextFromTable(const FString& Namespace, const FString& Key);
	
	/** 
	 * 지정한 키를 번역하기
	 * @param Namespace 텍스트를 찾을 테이블의 네임스페이스
	 * @param Key 지정할 테이블에서 찾을 키 값
	 * @return 지정한 키 값의 번역 결과 (FString)
	 */
	UFUNCTION(Blueprintpure)
	static FString GetStringFromTable(const FString& Namespace, const FString& Key);
	
	virtual void Init() override;
	
protected:
	virtual void OnStart() override;
	
private:
	//최초 설정값 생성
	void MakeFirstSettings();
	
	//저장된 창 위치를 적용
	void LoadGameWindowPosition(const TObjectPtr<UGameUserSettings> GameUserSettings, const TSharedPtr<SWindow>& GameWindow);
	
	//창 위치 저장
	void SaveGameWindowPosition(const TObjectPtr<UGameUserSettings> GameUserSettings, const TSharedPtr<SWindow>& GameWindow);

public:
	//첫 게임 데이터 생성
	TObjectPtr<UT3SaveGame> MakeFirstGameData(const ECharacterClass SelectedPlayerClass);
	
	/**
	 * 게임을 파일에 저장하기
	 * @return true : 저장 성공
	 */
	bool SaveGameToFile();
	
	/**
	 * 파일에 저장된 게임 불러오기
	 * @return true : 불러오기 성공
	 */
	bool LoadGameFromFile();
	
	//유저 세팅 저장하기
	bool SaveUserSettings();
	
	//저장된 유저 세팅 불러오기
	bool LoadUserSettings();
	
	//지정한 캐릭터 클래스에 해당되는 데이터 에셋
	TObjectPtr<UT3CharacterDataAsset> GetCharacterDataAsset();

	UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
    bool bPendingTeleport = false;

    UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
    FVector TargetTeleportLocation;

    UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
    FRotator TargetTeleportRotation;

    // 추가된 목적지 레벨 변수 (로딩 맵에서 꺼내 쓸 용도)
    UPROPERTY(BlueprintReadWrite, Category = "Level Transition")
    ELevelName TargetLevelName;

    /**
     * 세이브포인트를 지정하여 로딩 맵으로 이동
     */
    UFUNCTION(BlueprintCallable, Category = "Level Transition")
    void TravelToSavePoint(ELevelName LevelName, FName SavePointID);
	
	/**
	 * 레벨(맵) 이동하기
	 * @param LevelName 이동할 레벨 (주의 : TitleLevel이나 SelectClassLevel로 지정하면 게임에서 벗어납니다.)
	 */
	UFUNCTION(BlueprintCallable)
	void OpenLevel(UPARAM() ELevelName LevelName);
	
	//저장된 데이터를 기준으로 레벨(맵) 이동
	UFUNCTION(BlueprintCallable)
	void OpenLevelBySavedData();
	
	// 로딩 UI 시작 (레벨 이동 전 호출)
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void StartCustomLoading();

	// 로딩 UI 종료 (새 레벨 BeginPlay에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Loading")
	void EndCustomLoading();

	// 레벨 해금
	UFUNCTION(BlueprintCallable)
	void UnlockLevel(const ELevelName LevelName);

	// 세이브포인트 해금 (위치/회전 정보 포함 버전으로 업데이트)
	UFUNCTION(BlueprintCallable)
	void UnlockSavePoint(ELevelName LevelName, FName SavePointID, FVector Location, FRotator Rotation);

	// 레벨 해금 체크
	UFUNCTION(BlueprintPure)
	bool IsLevelUnlocked(ELevelName LevelName);

	// 세이브포인트 해금 체크
	UFUNCTION(BlueprintPure)
	bool IsSavePointUnlocked(ELevelName LevelName, FName SavePointID);
	
	// 특정 세이브 포인트의 위치 정보 가져오기 (이동 구현용)
	UFUNCTION(BlueprintPure)
	bool GetSavePointTransform(ELevelName LevelName, FName SavePointID, FVector& OutLocation, FRotator& OutRotation);
	
	//현재 레벨
	UFUNCTION(BlueprintPure)
	FORCEINLINE ELevelName GetCurrentLevel() const { return CurrentLevel; }

	//저장된 게임
	FORCEINLINE TObjectPtr<UT3SaveGame> GetSavedGameData() { return SavedGameData; }
	
	//현재 설정
	FORCEINLINE TObjectPtr<UT3SaveUserSettings> GetCurrentSettings() { return CurrentSettings; }
	
	//배경음 사운드 클래스
	FORCEINLINE TObjectPtr<USoundClass> GetSoundClassBGM() { return SoundClassBGM; }
	
	//효과음 사운드 클래스
	FORCEINLINE TObjectPtr<USoundClass> GetSoundClassSE() { return SoundClassSE; }
	
	//에디터용 : 캐릭터의 위치가 저장 데이터의 영향을 받지 않게 하려면 이 값을 true로 설정
    UPROPERTY(EditDefaultsOnly, Category = "Only For Test")
    bool bDoNotMoveCharacterBySavedData = false;
	
	// 에디터에서 커스텀 로딩 UI 블루프린트를 할당하세요.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> LoadingWidgetClass;

	// 생성된 위젯 인스턴스를 보관 (GC 보호)
	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingWidgetInstance;

private:
	//현재 설정
	UPROPERTY()
	TObjectPtr<UT3SaveUserSettings> CurrentSettings;

	//저장된 게임 데이터
	UPROPERTY()
	TObjectPtr<UT3SaveGame> SavedGameData;
	
	//효과음
	UPROPERTY(EditDefaultsOnly, Category = "Sound Class", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundClass> SoundClassSE;
	
	//배경음
	UPROPERTY(EditDefaultsOnly, Category = "Sound Class", meta = (AllowPrivateAccess = true))
	TObjectPtr<USoundClass> SoundClassBGM;
	

	/**
	 * 캐릭터 데이터
	 * @note 할당 순서는 T3PlayerInputState.h에서 ECharacterClass를 참조
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Character Data", meta = (AllowPrivateAccess = true))
	TArray<TObjectPtr<UT3CharacterDataAsset>> CharacterDataList;

	// T3GameInstance.h
	UPROPERTY(EditAnywhere, Category = "Level Settings")
	TMap<ELevelName, TSoftObjectPtr<UWorld>> LevelMap;
	
	//현재 레벨
	ELevelName CurrentLevel = ELevelName::Title;

	//저장, 불러오기에 사용할 슬롯 이름
	const FString SAVE_GAME_NAME = TEXT("SaveSlot1");
	const FString SAVE_USER_SETTINGS_NAME = TEXT("UserSettings");
};
