// T3HUDSlotWidget.cpp


#include "UI/T3HUDSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Player/T3SkillComponentBase.h"
#include "Kismet/GameplayStatics.h"


void UT3HUDSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	APawn* OwningPawn = GetOwningPlayerPawn();
	if (OwningPawn)
	{
		UT3SkillComponentBase* FoundComp = OwningPawn->FindComponentByClass<UT3SkillComponentBase>();
		if (FoundComp)
		{
			SkillComp = FoundComp;
			// 1. 쿨타임 시작 알림 바인딩
			if (!SkillComp->OnSkillCooldownStarted.IsBound())
			SkillComp->OnSkillCooldownStarted.AddDynamic(this, &UT3HUDSlotWidget::HandleCooldownStarted);
			// 2. 슬롯 교체 알림 바인딩
			if (!SkillComp->OnSkillSlotUpdated.IsBound())
			SkillComp->OnSkillSlotUpdated.AddDynamic(this, &UT3HUDSlotWidget::HandleSkillSlotUpdated);
		}
	}
}

void UT3HUDSlotWidget::HandleCooldownStarted(int32 SkillID, float CooldownTime)
{
	// 어떤 슬롯이든 쿨타임이 시작되면 일단 틱 로직 가동
	bIsTickActive = true;
}

void UT3HUDSlotWidget::HandleSkillSlotUpdated(int32 SlotIndex, int32 SkillID, const FSkillData& SkillData)
{
	// 스킬이 바뀌었을 때, 바뀐 스킬이 이미 쿨타임 중일 수 있으므로 틱을 켬
	bIsTickActive = true;
}

void UT3HUDSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsTickActive || !SkillComp) return;

	// 각 슬롯 업데이트 및 현재 쿨타임 진행 여부 반환
	bool bCurrentActive = ProcessCooldown(1, CurrentSkillCooldownBar, CurrentSkillCooldownTime, bCurrentWasCoolingDown);
	bool bNextActive = ProcessCooldown(2, NextSkillCooldownBar, NextSkillCooldownTime, bNextWasCoolingDown);

	// 모든 슬롯의 쿨타임이 끝났다면 틱 정지
	if (!bCurrentActive && !bNextActive)
	{
		bIsTickActive = false;
	}
}

bool UT3HUDSlotWidget::ProcessCooldown(int32 SlotIndex, UProgressBar* Bar, UTextBlock* Text, bool& bOutWasCoolingDown)
{
	if (!Bar || !Text) return false;

	int32 SkillID = SkillComp->GetSkillIDBySlotIndex(SlotIndex);

	// 슬롯이 비어있으면 초기화 후 종료
	if (SkillID == 0)
	{
		Bar->SetVisibility(ESlateVisibility::Hidden);
		Text->SetVisibility(ESlateVisibility::Hidden);
		bOutWasCoolingDown = false;
		return false;
	}

	float Ratio = SkillComp->GetCooldownRemainingRatio(SkillID);
	float RemainingTime = SkillComp->GetRemainingCooldown(SkillID);

	if (Ratio > 0.f)
	{
		bOutWasCoolingDown = true; // 쿨타임 중임을 기록
		Bar->SetVisibility(ESlateVisibility::Visible);
		Text->SetVisibility(ESlateVisibility::Visible);
		Bar->SetPercent(Ratio);
		Text->SetText(FText::AsNumber(FMath::CeilToInt(RemainingTime)));
		return true;
	}
	else
	{
		// 쿨타임 완료 시 쿨타임 종료, 애니메이션 실행
		if (bOutWasCoolingDown)
		{
			bOutWasCoolingDown = false;
			PlayFinishAnimation();
		}

		Bar->SetVisibility(ESlateVisibility::Hidden);
		Text->SetVisibility(ESlateVisibility::Hidden);
		return false;
	}
}

void UT3HUDSlotWidget::PlayFinishAnimation()
{
	if (FinishAnim)
	{
		PlayAnimation(FinishAnim);
	}
}