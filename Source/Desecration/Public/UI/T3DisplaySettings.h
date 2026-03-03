#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3DisplaySettings.generated.h"

class UT3SettingsPanel;

UCLASS()
class DESECRATION_API UT3DisplaySettings : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SettingsPanel;
};
