#pragma once

#include "CoreMinimal.h"
#include "T3SaveGameParent.h"
#include "T3SaveUserSettings.generated.h"

UCLASS()
class DESECRATION_API UT3SaveUserSettings : public UT3SaveGameParent
{
	GENERATED_BODY()
	
public:
	//초기 설정
	virtual void ResetGameData() override;
	
	//게임에 사용할 컨트롤러 값
	UPROPERTY()
	int32 UsingController;
	
	//수직 회전 반전
	UPROPERTY()
	bool bInvertVertical;
	
	//회전 감도
	UPROPERTY()
	float CameraSpeed;
};
