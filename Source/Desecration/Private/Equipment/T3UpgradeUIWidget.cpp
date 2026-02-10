// Fill out your copyright notice in the Description page of Project Settings.

#include "Equipment/T3UpgradeUIWidget.h"
#include "Equipment/T3UpgradeStation.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"

void UT3UpgradeUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 탭 버튼 바인딩
	if (Btn_WeaponTab)
	{
		Btn_WeaponTab->OnClicked.AddDynamic(this, &UT3UpgradeUIWidget::OnWeaponTabClicked);
	}
	if (Btn_ArmorTab)
	{
		Btn_ArmorTab->OnClicked.AddDynamic(this, &UT3UpgradeUIWidget::OnArmorTabClicked);
	}
	if (Btn_Upgrade)
	{
		Btn_Upgrade->OnClicked.AddDynamic(this, &UT3UpgradeUIWidget::OnUpgradeClicked);
	}

	// UpgradeStation 자동 탐색
	if (!UpgradeStation)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AT3UpgradeStation::StaticClass(), FoundActors);
		if (FoundActors.Num() > 0)
		{
			UpgradeStation = Cast<AT3UpgradeStation>(FoundActors[0]);
		}
	}

	// 강화석 아이콘 설정 (고정 이미지, 한 번만 설정)
	if (UpgradeStation)
	{
		if (Img_NormalStone)
		{
			if (UTexture2D* Icon = UpgradeStation->GetStoneIcon(ET3UpgradeStoneGrade::Normal))
			{
				Img_NormalStone->SetBrushFromTexture(Icon);
			}
		}
		if (Img_EpicStone)
		{
			if (UTexture2D* Icon = UpgradeStation->GetStoneIcon(ET3UpgradeStoneGrade::Epic))
			{
				Img_EpicStone->SetBrushFromTexture(Icon);
			}
		}
		if (Img_LegendaryStone)
		{
			if (UTexture2D* Icon = UpgradeStation->GetStoneIcon(ET3UpgradeStoneGrade::Legendary))
			{
				Img_LegendaryStone->SetBrushFromTexture(Icon);
			}
		}
	}

	// 초기 UI 갱신
	RefreshUI();
}

void UT3UpgradeUIWidget::OnWeaponTabClicked()
{
	CurrentTab = ET3EquipmentType::Weapon;
	RefreshUI();
}

void UT3UpgradeUIWidget::OnArmorTabClicked()
{
	CurrentTab = ET3EquipmentType::Armor;
	RefreshUI();
}

void UT3UpgradeUIWidget::OnUpgradeClicked()
{
	if (!UpgradeStation) return;

	// 현재 탭에 맞는 강화 실행 (강화석 자동 선택)
	UpgradeStation->UpgradeEquipment(CurrentTab);

	// 강화 후 UI 갱신 (레벨, 스탯, 버튼 상태 모두 업데이트)
	RefreshUI();
}

void UT3UpgradeUIWidget::RefreshUI()
{
	if (!UpgradeStation) return;

	// 탭 선택 상태 표시
	const FLinearColor SelectedTabColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor UnselectedTabColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

	if (Btn_WeaponTab)
	{
		Btn_WeaponTab->SetBackgroundColor(CurrentTab == ET3EquipmentType::Weapon ? SelectedTabColor : UnselectedTabColor);
	}
	if (Btn_ArmorTab)
	{
		Btn_ArmorTab->SetBackgroundColor(CurrentTab == ET3EquipmentType::Armor ? SelectedTabColor : UnselectedTabColor);
	}

	// 현재 탭에 해당하는 장비 데이터 조회
	FT3UpgradeUIData UIData = UpgradeStation->GetEquipmentUIData(CurrentTab);

	// 장비 아이콘 표시
	// 현재: 장착된 장비의 데이터 테이블 Icon을 그대로 표시
	// 추후: 장비 종류 확장 시 선택된 아이템 인스턴스 기반으로 교체
	if (Img_Icon)
	{
		if (UIData.Icon)
		{
			Img_Icon->SetBrushFromTexture(UIData.Icon);
			Img_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			Img_Icon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 장비 이름 표시
	if (Txt_DisplayName)
	{
		Txt_DisplayName->SetText(UIData.DisplayName);
	}

	// 강화 레벨 표시
	// MAX: "+7 (MAX)" / 그 외: "+3 → +4"
	if (Txt_Level)
	{
		if (UIData.bIsMaxLevel)
		{
			Txt_Level->SetText(FText::FromString(FString::Printf(TEXT("+%d (MAX)"), UIData.CurrentLevel)));
		}
		else
		{
			Txt_Level->SetText(FText::FromString(FString::Printf(TEXT("+%d → +%d"), UIData.CurrentLevel, UIData.CurrentLevel + 1)));
		}
	}

	// 스탯 표시
	// MAX: "공격력: 180" / 그 외: "공격력: 150 → 180"
	if (Txt_Stat)
	{
		FString StatLabel = (CurrentTab == ET3EquipmentType::Weapon) ? TEXT("공격력") : TEXT("방어력");

		if (UIData.bIsMaxLevel)
		{
			Txt_Stat->SetText(FText::FromString(FString::Printf(TEXT("%s: %.0f"), *StatLabel, UIData.CurrentStat)));
		}
		else
		{
			Txt_Stat->SetText(FText::FromString(FString::Printf(TEXT("%s: %.0f → %.0f"), *StatLabel, UIData.CurrentStat, UIData.NextLevelStat)));
		}
	}

	// 강화석 아이콘 색상 처리 (사용 가능: 원래 색상 / 사용 불가: 어둡게)
	const FLinearColor ActiveColor = FLinearColor::White;
	const FLinearColor InactiveColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);

	if (Img_NormalStone)
	{
		bool bCanUse = UpgradeStation->CanUseStone(ET3UpgradeStoneGrade::Normal, UIData.CurrentLevel)
			&& UpgradeStation->GetStoneCount(ET3UpgradeStoneGrade::Normal) > 0;
		Img_NormalStone->SetColorAndOpacity(bCanUse ? ActiveColor : InactiveColor);
	}
	if (Img_EpicStone)
	{
		bool bCanUse = UpgradeStation->CanUseStone(ET3UpgradeStoneGrade::Epic, UIData.CurrentLevel)
			&& UpgradeStation->GetStoneCount(ET3UpgradeStoneGrade::Epic) > 0;
		Img_EpicStone->SetColorAndOpacity(bCanUse ? ActiveColor : InactiveColor);
	}
	if (Img_LegendaryStone)
	{
		bool bCanUse = UpgradeStation->CanUseStone(ET3UpgradeStoneGrade::Legendary, UIData.CurrentLevel)
			&& UpgradeStation->GetStoneCount(ET3UpgradeStoneGrade::Legendary) > 0;
		Img_LegendaryStone->SetColorAndOpacity(bCanUse ? ActiveColor : InactiveColor);
	}

	// 소모 예정 강화석 Border 강조 (배경색으로 표시, 이미지에 영향 없음)
	const FLinearColor SelectedBorderColor = FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);  // 노란 배경 (불투명)
	const FLinearColor DefaultBorderColor = FLinearColor::Transparent;

	ET3UpgradeStoneGrade NextGrade;
	bool bHasNextStone = UpgradeStation->GetNextStoneGrade(UIData.CurrentLevel, NextGrade);

	if (Border_NormalStone)
	{
		bool bIsSelected = bHasNextStone && NextGrade == ET3UpgradeStoneGrade::Normal;
		Border_NormalStone->SetBrushColor(bIsSelected ? SelectedBorderColor : DefaultBorderColor);
	}
	if (Border_EpicStone)
	{
		bool bIsSelected = bHasNextStone && NextGrade == ET3UpgradeStoneGrade::Epic;
		Border_EpicStone->SetBrushColor(bIsSelected ? SelectedBorderColor : DefaultBorderColor);
	}
	if (Border_LegendaryStone)
	{
		bool bIsSelected = bHasNextStone && NextGrade == ET3UpgradeStoneGrade::Legendary;
		Border_LegendaryStone->SetBrushColor(bIsSelected ? SelectedBorderColor : DefaultBorderColor);
	}

	// 강화석 보유량 표시
	if (Txt_NormalStoneCount)
	{
		Txt_NormalStoneCount->SetText(FText::AsNumber(UpgradeStation->GetStoneCount(ET3UpgradeStoneGrade::Normal)));
	}
	if (Txt_EpicStoneCount)
	{
		Txt_EpicStoneCount->SetText(FText::AsNumber(UpgradeStation->GetStoneCount(ET3UpgradeStoneGrade::Epic)));
	}
	if (Txt_LegendaryStoneCount)
	{
		Txt_LegendaryStoneCount->SetText(FText::AsNumber(UpgradeStation->GetStoneCount(ET3UpgradeStoneGrade::Legendary)));
	}

	// 강화 버튼 활성화/비활성화
	// bCanUpgrade는 최대레벨 or 강화석 없음이면 이미 false
	if (Btn_Upgrade)
	{
		Btn_Upgrade->SetIsEnabled(UIData.bCanUpgrade);
	}
}
