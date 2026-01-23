#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SettingEnums.h"
#include "T3GameInstance.generated.h"

class AT3SaveGame;

//설정값을 저장하는 구조체
struct FSettings
{
	EResolution Resolution;
	EScreenMode ScreenMode;
};

UCLASS()
class DESECRATION_API UT3GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;
	
	//현재 설정
	FORCEINLINE FSettings GetCurrentSettings() const { return CurrentSettings; }

private:
	//현재 설정
	FSettings CurrentSettings;
};
