#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3SoundSettings.generated.h"

class UT3SettingsPanel;

UCLASS()
class DESECRATION_API UT3SoundSettings : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SettingsPanel;
};
