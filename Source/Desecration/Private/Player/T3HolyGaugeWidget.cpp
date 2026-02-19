// T3HolyGaugeWidget.cpp


#include "Player/T3HolyGaugeWidget.h"
#include "Components/ProgressBar.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3CharacterBase.h"

void UT3HolyGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 소유자 캐릭터의 컴포넌트를 찾아 델리게이트 바인딩
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UT3CombatComponent* CombatComp = OwningPawn->FindComponentByClass<UT3CombatComponent>())
		{
			// 델리게이트에 UpdateGauge 함수를 연결
			CombatComp->OnHolyGaugeChanged.AddDynamic(this, &UT3HolyGaugeWidget::UpdateGauge);

			// 초기값 동기화
			UpdateGauge(CombatComp->HolyGauge, CombatComp->MaxHolyGauge);
		}
	}
}

void UT3HolyGaugeWidget::UpdateGauge(float CurrentGauge, float MaxGauge)
{
	if (HolyGaugeBar && MaxGauge > 0.f)
	{
		float Percent = CurrentGauge / MaxGauge;
		HolyGaugeBar->SetPercent(Percent);
	}
}

