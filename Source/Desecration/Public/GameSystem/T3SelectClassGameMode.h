#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "T3SelectClassGameMode.generated.h"

enum class ECharacterClass : uint8;
class UT3GameInstance;

UCLASS()
class DESECRATION_API AT3SelectClassGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//시작 데이터 생성하기
	void MakeFirstGameData(const FString& PlayerName, const ECharacterClass SelectedPlayerClass);
	
	//튜토리얼 시작
	void TutorialStart();
	
	//타이틀 레벨로 돌아가기
	void ReturnToTitleLevel();
	
private:
	//무기 공격력 테이블 (기본 공격력 확인용)
	UPROPERTY(EditDefaultsOnly, Category = "Data", meta = (AllowPrivateAccess = true))
	TObjectPtr<UDataTable> WeaponTable;
	
	//WeaponTable에서 찾을 행 이름
	UPROPERTY(EditDefaultsOnly, Category = "Data", meta = (AllowPrivateAccess = true))
	FName RowNameInWeaponTable;
	
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
