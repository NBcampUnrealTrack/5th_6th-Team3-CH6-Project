// T3MidBossHPBarWidget.cpp

#include "Monster/T3MidBossHPBarWidget.h"
#include "Monster/T3MidBossMonster.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Desecration.h"

void UT3MidBossHPBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!TargetBoss)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_MidBossHPBar: TargetBoss가 설정되지 않음"));
		return;
	}

	// 보스 이름 표시
	if (Txt_BossName)
	{
		Txt_BossName->SetText(FText::FromString(TargetBoss->BossName));
	}

	// 초기 HP 비율 계산
	const float MaxHP = TargetBoss->MidBossStats.MaxHP;
	if (MaxHP > 0.f)
	{
		RedPercent = FMath::Clamp(TargetBoss->MidBossStats.CurrentHP / MaxHP, 0.f, 1.f);
	}
	else
	{
		RedPercent = 0.f;
	}

	YellowPercent = RedPercent;
	TargetPercent = RedPercent;

	// 프로그래스바 초기값 설정
	if (HPBar_Red)
	{
		HPBar_Red->SetPercent(RedPercent);
	}
	if (HPBar_Yellow)
	{
		HPBar_Yellow->SetPercent(YellowPercent);
	}

	// 델리게이트 바인딩 (이벤트 기반 — Tick에서 폴링하지 않음)
	TargetBoss->OnMidBossDamaged.AddDynamic(this, &UT3MidBossHPBarWidget::UpdateHPBar);
	TargetBoss->OnMidBossDeath.AddDynamic(this, &UT3MidBossHPBarWidget::OnBossDeath);

	UE_LOG(LogDesecration, Log, TEXT("T3_MidBossHPBar: 위젯 초기화 완료 (보스: %s, HP: %.0f/%.0f)"),
		*TargetBoss->BossName, TargetBoss->MidBossStats.CurrentHP, MaxHP);
}

void UT3MidBossHPBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Yellow 바 캐치업 보간 (Red 바를 향해 서서히 줄어듦)
	if (bShouldCatchUp && !FMath::IsNearlyEqual(YellowPercent, TargetPercent, 0.001f))
	{
		YellowPercent = FMath::FInterpTo(YellowPercent, TargetPercent, InDeltaTime, YellowInterpSpeed);

		if (HPBar_Yellow)
		{
			HPBar_Yellow->SetPercent(YellowPercent);
		}
	}
	else if (bShouldCatchUp && FMath::IsNearlyEqual(YellowPercent, TargetPercent, 0.001f))
	{
		// 캐치업 완료 — 정확히 맞추고 정지
		YellowPercent = TargetPercent;
		if (HPBar_Yellow)
		{
			HPBar_Yellow->SetPercent(YellowPercent);
		}
		bShouldCatchUp = false;
	}
}

void UT3MidBossHPBarWidget::UpdateHPBar()
{
	if (!TargetBoss)
	{
		return;
	}

	const float MaxHP = TargetBoss->MidBossStats.MaxHP;
	if (MaxHP <= 0.f)
	{
		return;
	}

	// Red 바 즉시 갱신
	RedPercent = FMath::Clamp(TargetBoss->MidBossStats.CurrentHP / MaxHP, 0.f, 1.f);
	if (HPBar_Red)
	{
		HPBar_Red->SetPercent(RedPercent);
	}

	// Yellow 캐치업 목표 설정 (지연 후 시작)
	TargetPercent = RedPercent;

	// 기존 지연 타이머 리셋 (연속 피격 시 마지막 타격 기준으로 지연)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(YellowDelayTimerHandle);

		// 지연 후 캐치업 시작
		FTimerDelegate DelayDelegate;
		DelayDelegate.BindLambda([this]()
		{
			bShouldCatchUp = true;
		});
		World->GetTimerManager().SetTimer(
			YellowDelayTimerHandle, DelayDelegate, YellowDelaySeconds, false);
	}
}

void UT3MidBossHPBarWidget::OnBossDeath()
{
	// Red 바 0으로 즉시 갱신
	RedPercent = 0.f;
	TargetPercent = 0.f;
	if (HPBar_Red)
	{
		HPBar_Red->SetPercent(0.f);
	}

	// Yellow 캐치업 즉시 시작 (지연 없이)
	bShouldCatchUp = true;

	// 지연 후 위젯 제거
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathRemoveTimerHandle,
			this, &UT3MidBossHPBarWidget::RemoveWidget,
			DeathRemoveDelay, false);
	}
}

void UT3MidBossHPBarWidget::RemoveWidget()
{
	// 타이머 클리어 — GC 후 댕글링 포인터 방지
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(YellowDelayTimerHandle);
		World->GetTimerManager().ClearTimer(DeathRemoveTimerHandle);
	}

	// 델리게이트 해제
	if (TargetBoss)
	{
		TargetBoss->OnMidBossDamaged.RemoveDynamic(this, &UT3MidBossHPBarWidget::UpdateHPBar);
		TargetBoss->OnMidBossDeath.RemoveDynamic(this, &UT3MidBossHPBarWidget::OnBossDeath);
	}

	OnWidgetEnd.Broadcast();
	RemoveFromParent();
}
