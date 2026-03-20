#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "T3GameMode.generated.h"

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
	
	//캐릭터의 클래스를 기반으로 캐릭터 데이터 에셋을 가져온다
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	UT3CharacterDataAsset* GetCharacterDataAsset();
	
	//게임 저장하기 (true : 저장 성공)
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	bool SaveGame(const AT3CharacterBase* Character, const ELevelName LevelName, const bool bTemporarySave);
	
	//게임 불러오기 : 마지막으로 저장한 데이터를 다시 불러오고 그 데이터에 기록된 맵으로 이동
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	void LoadGame();
	
	//저장된 게임 데이터를 기반으로 캐릭터 세팅
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	void SetCharacterBySavedData(AT3CharacterBase* Character);
	
	//잃어버린 재화를 되찾음
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	void RegainLostMoney(const int32 LostMoneyID) const;
	
	//게임 오버에 대한 처리
	UFUNCTION(BlueprintCallable, Category = "Game Over")
	bool YouHaveBeenCorrupted(const AT3CharacterBase* Character) const;
	
private:
	//잃어버린 재화 액터
	UPROPERTY(EditDefaultsOnly, Category = "Game Over", meta = (AllowPrivateAccess = true))
	TSubclassOf<AT3LostMoney> LostMoneyClass;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
