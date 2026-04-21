#pragma once

#include "CoreMinimal.h"
#include "GlobalEnums.h"
#include "T3SaveGame.h"
#include "GameFramework/GameMode.h"
#include "T3GameMode.generated.h"

enum class ESaveType : uint8;
class UT3CharacterDataAsset;
class AT3LostMoney;
class UT3GameInstance;
class AT3CharacterBase;
enum class ECharacterClass : uint8;
enum class ELevelName : uint8;

UCLASS()
class DESECRATION_API AT3GameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
private:
	//잃어버린 재화 액터 생성
	void MakeLostMoneyActors();
	
public:
	//캐릭터의 클래스
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	ECharacterClass GetPlayerClass();
	
	/**
	 * 게임 저장하기 
	 * @details 각 매개변수 중 필수가 아닌 것은 저장 범위에 따라 적절하게 값을 넣거나 무시하면 됩니다.
	 * @param Character : (필수) 현재 조종중인 캐릭터
	 * @param SaveType : (필수) 저장 범위
	 * @param LevelName : 저장하려는 레벨(맵) 이름, 맵 저장시에 사용하며 타이틀 및 클래스 선택 레벨로 지정하면 무시됨
	 * @param TargetLocation : 저장하려는 맵 내의 위치
	 * @param TargetRotation : 저장시 캐릭터의 회전값
	 * @return true : 저장 성공
	 */
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	bool SaveGame_V2(
		const AT3CharacterBase* Character,
		const ESaveType SaveType,
		const ELevelName LevelName = ELevelName::Tutorial, 
		const FVector& TargetLocation = FVector(0,0,0), 
		const FRotator& TargetRotation = FRotator(0,0,0));
	
	/**
	 * 게임 저장하기 
	 * @details 각 매개변수 중 필수가 아닌 것은 저장 범위에 따라 적절하게 값을 넣거나 무시하면 됩니다.
	 * @param Character : (필수) 현재 조종중인 캐릭터
	 * @param SaveType : (필수) 저장 범위 (비트 연산을 통해 2개 이상의 범위를 사용할 수 있음)
	 * @param LevelName : 저장하려는 레벨(맵) 이름, 타이틀 및 클래스 선택 레벨로 지정하면 무시됨
	 * @param TargetLocation : 저장하려는 맵 내의 위치
	 * @param TargetRotation : 저장시 캐릭터의 회전값
	 * @return true : 저장 성공
	 */
	bool SaveGame_V2(
		const AT3CharacterBase* Character,
		const uint8 SaveType,
		const ELevelName LevelName = ELevelName::Tutorial, 
		const FVector& TargetLocation = FVector(0,0,0), 
		const FRotator& TargetRotation = FRotator(0,0,0));
	
	/**
	 * 게임 저장하기 
	 * @return true : 저장 성공
	 * @deprecated 세이브 로직 최적화를 했습니다. SaveGame_V2()를 사용하세요.
	 */
	UE_DEPRECATED(5.7, "세이브 로직 최적화를 했습니다. SaveGame_V2()를 사용하세요.")
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	bool SaveGame(
		const AT3CharacterBase* Character, 
		const ELevelName LevelName, 
		const bool bTemporarySave,
		const FVector TargetLocation,    // 추가된 위치 정보
		const FRotator TargetRotation    // 추가된 회전 정보
	);
	
	/**
	 * 인벤토리 및 물약 강화 상태만 저장하기
	 * @return true : 저장 성공
	 */
	UE_DEPRECATED(5.7, "세이브 로직 최적화를 했습니다. SaveGame_V2()를 사용하세요.")
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	bool SaveInventoryAndPotionLevel(const AT3CharacterBase* Character);
	
	/**
	 * 스킬만 저장하기
	 * @return true : 저장 성공
	 * @deprecated 세이브 로직 최적화를 했습니다. SaveGame_V2()를 사용하세요.
	 */
	UE_DEPRECATED(5.7, "세이브 로직 최적화를 했습니다. SaveGame_V2()를 사용하세요.")
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	bool SaveOnlySkill(const AT3CharacterBase* Character);

	/**
	 * 스탯만 저장하기
	 * @return true : 저장 성공
	 * @deprecated 세이브 로직 최적화를 했습니다. SaveGame_V2()를 사용하세요.
	 */
	UE_DEPRECATED(5.7, "새로 만들어진 SaveGame함수(매개변수 있는 것)를 사용하세요")
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	bool SaveOnlyStat(const AT3CharacterBase* Character);
	
	//게임 불러오기 : 현재 저장된 데이터에 기록된 맵으로 이동
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	void LoadGame() const;
	
	//저장된 게임 데이터를 기반으로 캐릭터 세팅
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	void SetCharacterBySavedData(AT3CharacterBase* Character);
	
	//잃어버린 재화를 되찾음
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void RegainLostMoney(const int32 LostMoneyID) const;
	
	//게임 오버에 대한 처리
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	bool YouHaveBeenCorrupted(const AT3CharacterBase* Character) const;
	
	//개발용 : 그 자리에서 즉시 저장
	UFUNCTION(BlueprintCallable, Category = "Test")
	void InstantSave();
	
private:
	//잃어버린 재화 액터
	UPROPERTY(EditDefaultsOnly, Category = "Game Over", meta = (AllowPrivateAccess = true))
	TSubclassOf<AT3LostMoney> LostMoneyClass;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
