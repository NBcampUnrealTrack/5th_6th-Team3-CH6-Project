// T3LockOnTarget.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "T3LockOnTarget.generated.h"

UINTERFACE(MinimalAPI)
class UT3LockOnTarget : public UInterface
{
	GENERATED_BODY()
};


class DESECRATION_API IT3LockOnTarget
{
	GENERATED_BODY()

public:

	virtual void SetLockOnWidgetVisible(bool bVisible) = 0;
};
