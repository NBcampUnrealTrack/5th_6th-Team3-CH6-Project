#pragma once

#include "CoreMinimal.h"
#include "T3SettingsPanel.h"
#include "Blueprint/UserWidget.h"
#include "GameSystem/T3GameInstance.h"
#include "T3SettingsPanelCategory.generated.h"

UCLASS(Abstract)
class DESECRATION_API UT3SettingsPanelCategory : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SettingsPanel;
	
protected:
	//부모 위젯인 세팅 패널
	UPROPERTY()
	TSoftObjectPtr<UT3SettingsPanel> SettingsPanel;
};
