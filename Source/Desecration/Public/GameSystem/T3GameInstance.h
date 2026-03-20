#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GlobalEnums.h"
#include "T3GameInstance.generated.h"

enum class ECharacterClass : uint8;
class UT3SaveLostMoney;
class UT3SaveUserSettings;
class UT3CharacterDataAsset;
class UT3SaveGame;

USTRUCT(BlueprintType)
struct FLevelProgressData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLevelUnlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, bool> SavePoints;
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
	
	UPROPERTY(BlueprintReadWrite)
	TMap<ELevelName, FLevelProgressData> LevelProgressMap;
	
	// 레벨 해금
	UFUNCTION(BlueprintCallable)
	void UnlockLevel(ELevelName LevelName);

	// 세이브포인트 해금
	UFUNCTION(BlueprintCallable)
	void UnlockSavePoint(ELevelName LevelName, FName SavePointID);

	// 레벨 해금 체크
	UFUNCTION(BlueprintPure)
	bool IsLevelUnlocked(ELevelName LevelName);

	// 세이브포인트 해금 체크
	UFUNCTION(BlueprintPure)
	bool IsSavePointUnlocked(ELevelName LevelName, FName SavePointID);
	
	
private:
	//최초 설정값 생성
	void MakeFirstSettings();
	
public:
	//첫 게임 데이터 생성
	TObjectPtr<UT3SaveGame> MakeFirstGameData();
	
	//잃어버린 재화 데이터 생성
	void MakeFirstLostMoneyData();
	
	/**
	 * 게임 저장하기
	 * @return true : 저장 성공
	 */
	bool SaveGame();
	
	/**
	 * 저장된 게임 불러오기
	 * @return true : 불러오기 성공
	 */
	bool LoadGame();
	
	//유저 세팅 저장하기
	bool SaveUserSettings();
	
	//저장된 유저 세팅 불러오기
	bool LoadUSerSettings();
	
	//잃어버린 재화 정보 저장
	bool SaveLostMoney();
	
	//잃어버린 재화 정보 불러오기
	bool LoadLostMoney();
	
	//지정한 캐릭터 클래스에 해당되는 데이터 에셋
	UT3CharacterDataAsset* GetCharacterDataAsset();

	/**
	 * 레벨(맵) 이동하기
	 * @param LevelName 이동할 레벨 (주의 : TitleLevel이나 SelectClassLevel로 지정하면 게임에서 벗어납니다.)
	 */
	UFUNCTION(BlueprintCallable)
	void OpenLevel(UPARAM() ELevelName LevelName);
	
	//저장된 데이터를 기준으로 레벨(맵) 이동
	void OpenLevelBySavedData();
	
	//현재 레벨
	UFUNCTION(BlueprintPure)
	FORCEINLINE ELevelName GetCurrentLevel() const { return CurrentLevel; }

	//저장된 게임
	FORCEINLINE TObjectPtr<UT3SaveGame> GetSavedGameData() { return SavedGameData; }
	
	//현재 설정
	FORCEINLINE TObjectPtr<UT3SaveUserSettings> GetCurrentSettings() { return CurrentSettings; }
	
	//잃어버린 재화
	FORCEINLINE TObjectPtr<UT3SaveLostMoney> GetLostMoneyData() { return LostMoneyData; }
	
	//배경음 사운드 클래스
	FORCEINLINE TObjectPtr<USoundClass> GetSoundClassBGM() { return SoundClassBGM; }
	
	//효과음 사운드 클래스
	FORCEINLINE TObjectPtr<USoundClass> GetSoundClassSE() { return SoundClassSE; }
	
	//에디터용 : 캐릭터의 위치가 저장 데이터의 영향을 받지 않게 하려면 이 값을 true로 설정
    UPROPERTY(EditDefaultsOnly, Category = "Only For Test")
    bool bDoNotMoveCharacterBySavedData = false;

private:
	//현재 설정
	UPROPERTY()
	TObjectPtr<UT3SaveUserSettings> CurrentSettings;

	//저장된 게임 데이터
	UPROPERTY()
	TObjectPtr<UT3SaveGame> SavedGameData;
	
	//잃어버린 재화
	UPROPERTY()
	TObjectPtr<UT3SaveLostMoney> LostMoneyData; 
	
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
	const FString SAVE_LOST_MONEY_NAME = TEXT("LostMoney");
};
