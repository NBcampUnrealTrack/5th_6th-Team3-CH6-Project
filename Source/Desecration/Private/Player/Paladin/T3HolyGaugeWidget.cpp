// T3HolyGaugeWidget.cpp


#include "Player/Paladin/T3HolyGaugeWidget.h"
#include "Components/ProgressBar.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3CharacterBase.h"

void UT3HolyGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

}

void UT3HolyGaugeWidget::InitializeWidget(UT3SkillComponentBase* InSkillComp)
{

	// 소유자 캐릭터의 컴포넌트를 찾아 델리게이트 바인딩
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (UT3CombatComponent* CombatComp = OwningPawn->FindComponentByClass<UT3CombatComponent>())
			if (UT3SkillComponentBase* SkillComp = CombatComp->GetSkillComponent())
			{
				{
					// 중복 바인딩 방지를 위해 먼저 제거 후 등록
					InSkillComp->OnResourceChanged.RemoveDynamic(this, &UT3HolyGaugeWidget::UpdateGauge);
					InSkillComp->OnResourceChanged.AddDynamic(this, &UT3HolyGaugeWidget::UpdateGauge);

					// 초기값 0.f
					UpdateGauge(0.f);

					UE_LOG(LogTemp, Log, TEXT("HolyGaugeWidget Initialized Successfully!"));
				}
			}
	}
}

void UT3HolyGaugeWidget::UpdateGauge(float CurrentGauge)
{
	if (HolyGaugeBar)
	{
		float Percent = CurrentGauge / 100.0f;
		HolyGaugeBar->SetPercent(Percent);
	}
}

