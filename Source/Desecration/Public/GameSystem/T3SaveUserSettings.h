#pragma once

#include "CoreMinimal.h"
#include "T3SaveGameParent.h"
#include "T3SaveUserSettings.generated.h"

//주의 : 언리얼 엔진에서 직접 다루는 GameUserSettings와는 다른 클래스 (이건 T3'Save'UserSettings)
UCLASS()
class DESECRATION_API UT3SaveUserSettings : public UT3SaveGameParent
{
	GENERATED_BODY()
	
public:
	//초기 설정
	virtual void ResetGameData() override;
	
	//수직 회전 반전
	UPROPERTY()
	bool bInvertVertical;
	
	//회전 감도
	UPROPERTY()
	float CameraSpeed;
};
