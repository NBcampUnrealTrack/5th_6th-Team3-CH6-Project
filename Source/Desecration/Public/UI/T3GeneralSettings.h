#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3GeneralSettings.generated.h"

class UT3SettingsPanel;

UCLASS()
class DESECRATION_API UT3GeneralSettings : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SettingsPanel;
};
