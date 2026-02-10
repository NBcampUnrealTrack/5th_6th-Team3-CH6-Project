#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "T3GameMode.generated.h"

class UT3GameInstance;
class AT3CharacterBase;

UCLASS()
class DESECRATION_API AT3GameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	
public:
	//게임 저장하기 (true : 저장 성공)
	UFUNCTION(BlueprintPure, Category = "Saved Game Data")
	bool SaveGame(const AT3CharacterBase* Character);
	
	//저장된 게임 데이터를 기반으로 캐릭터 세팅
	UFUNCTION(BlueprintCallable, Category = "Saved Game Data")
	void SetCharacterBySavedData(AT3CharacterBase* Character);
	
private:
	//게임 인스턴스
	UPROPERTY()
	TObjectPtr<UT3GameInstance> T3GameInstance;
};
