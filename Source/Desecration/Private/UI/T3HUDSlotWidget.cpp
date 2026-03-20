// T3HUDSlotWidget.cpp


#include "UI/T3HUDSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Player/T3SkillComponentBase.h"
#include "Kismet/GameplayStatics.h"


void UT3HUDSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

}

void UT3HUDSlotWidget::InitializeWidget(UT3SkillComponentBase* InSkillComp)
{
	if (!InSkillComp) return;

	SkillComp = InSkillComp;

	// 안전하게 기존 바인딩 제거 후 재등록 (중복 방지)
	SkillComp->OnSkillCooldownStarted.RemoveDynamic(this, &UT3HUDSlotWidget::HandleCooldownStarted);
	SkillComp->OnSkillCooldownStarted.AddDynamic(this, &UT3HUDSlotWidget::HandleCooldownStarted);

	SkillComp->OnSkillSlotUpdated.RemoveDynamic(this, &UT3HUDSlotWidget::HandleSkillSlotUpdated);
	SkillComp->OnSkillSlotUpdated.AddDynamic(this, &UT3HUDSlotWidget::HandleSkillSlotUpdated);

	// 2. 초기 상태 반영 
	for (int32 i = 1; i <= 2; ++i)
	{
		int32 SkillID = SkillComp->GetSkillIDBySlotIndex(i);
		UTexture2D* Icon = SkillComp->GetSkillIconByID(SkillID);
		SetSlotVisual(i, Icon);
	}

	bIsTickActive = true; // 초기 상태 확인을 위해 틱 활성화
	UE_LOG(LogTemp, Log, TEXT("HUD Widget Initialized with SkillComponent"));
}

void UT3HUDSlotWidget::UpdateSkillSlotVisual(int32 SlotIndex, int32 SkillID, const FSkillData& SkillData)
{
	// 블루프린트의 각 스킬 슬롯 업데이트 로직 통합
	SetSlotVisual(SlotIndex, SkillData.SkillIcon);
}

void UT3HUDSlotWidget::SetSlotVisual(int32 SlotIndex, UTexture2D* Icon)
{
	UImage* TargetImage = (SlotIndex == 1) ? FirstSkillImage : (SlotIndex == 2 ? NextSkillImage : nullptr);

	if (!TargetImage) return;

	// 블루프린트의 '슬롯이 비었을 때(SkillID == 0)' 분기 처리
	if (SkillComp->GetSkillIDBySlotIndex(SlotIndex) == 0)
	{
		TargetImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		TargetImage->SetVisibility(ESlateVisibility::Visible);
		TargetImage->SetBrushFromTexture(Icon);
	}
}

void UT3HUDSlotWidget::HandleCooldownStarted(int32 SkillID, float CooldownTime)
{
	// 어떤 슬롯이든 쿨타임이 시작되면 일단 틱 로직 가동
	bIsTickActive = true;
}

void UT3HUDSlotWidget::HandleSkillSlotUpdated(int32 SlotIndex, int32 SkillID, const FSkillData& SkillData)
{
	// 1. 틱 활성화 (쿨타임 처리를 위해)
	bIsTickActive = true;

	// 2. ★ 시각적 이미지 즉시 갱신
	SetSlotVisual(SlotIndex, SkillData.SkillIcon);

	UE_LOG(LogTemp, Log, TEXT("Slot %d Updated: SkillID %d"), SlotIndex, SkillID);
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