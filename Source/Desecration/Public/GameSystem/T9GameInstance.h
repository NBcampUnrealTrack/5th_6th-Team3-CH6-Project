#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SettingEnums.h"
#include "T9GameInstance.generated.h"

//설정값을 저장하는 구조체
struct FSettings
{
	EResolution Resolution;
	EScreenMode ScreenMode;
};

UCLASS()
class DESECRATION_API UT9GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	//현재 설정
	FSettings CurrentSettings;
};
