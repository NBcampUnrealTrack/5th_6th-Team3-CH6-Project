#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "T3GameMode.generated.h"

class AT3GameState;
class UT3GameInstance;
class AT3CharacterBase;
enum class EPlayerClass;
enum class ELevelName : uint8;

UCLASS()
class DESECRATION_API AT3GameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//캐릭터의 클래스
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	EPlayerClass GetPlayerClass();
	
	//게임 저장하기 (true : 저장 성공)
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	bool SaveGame(const AT3CharacterBase* Character, const ELevelName LevelName, const bool bTemporarySave);
	
	//게임 불러오기 : 마지막으로 저장한 데이터를 다시 불러오고 그 데이터에 기록된 맵으로 이동
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	void LoadGame();
	
	//저장된 게임 데이터를 기반으로 캐릭터 세팅
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	void SetCharacterBySavedData(AT3CharacterBase* Character);
	
private:
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
	
	//게임 스테이트
	UPROPERTY()
	TObjectPtr<AT3GameState> T3GameState;
};
