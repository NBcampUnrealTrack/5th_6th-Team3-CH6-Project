// T3TaoistCloneController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "T3TaoistCloneController.generated.h"


UCLASS()
class DESECRATION_API AT3TaoistCloneController : public AAIController
{
	GENERATED_BODY()
	
public:
	// 분신에게 특정 타겟으로 이동 명령을 내리는 함수
	void UpdateTargetTracking(AActor* Target);
};
