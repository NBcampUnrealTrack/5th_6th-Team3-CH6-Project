// T3HolyGaugeWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3HolyGaugeWidget.generated.h"

class UProgressBar;


UCLASS()
class DESECRATION_API UT3HolyGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HolyGaugeBar;

	// 게이지 업데이트 함수
	UFUNCTION()
	void UpdateGauge(float CurrentGauge, float MaxGauge);

	virtual void NativeConstruct() override;
	
};
