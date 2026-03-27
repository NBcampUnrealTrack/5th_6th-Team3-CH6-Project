// Fill out your copyright notice in the Description page of Project Settings.

#include "Equipment/T3UpgradeUIWidget.h"
#include "Equipment/T3UpgradeStation.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Item/Data/T3RuneItemData.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Rune/T3SynthesisSlotWidget.h"

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
		if (UWorld* World = GetWorld())
		{
			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsOfClass(World, AT3UpgradeStation::StaticClass(), FoundActors);
			if (FoundActors.Num() > 0)
			{
				UpgradeStation = Cast<AT3UpgradeStation>(FoundActors[0]);
			}
		}
	}
	
	UT3SynthesisSlotWidget* SlotWidgets[3] = { SynthesisSlot_0, SynthesisSlot_1, SynthesisSlot_2 };
	
	for (int32 i = 0; i < 3; i++)
	{
		if (SlotWidgets[i])
		{
			SlotWidgets[i]->SlotIndex = i;
			SlotWidgets[i]->UpgradeStation = UpgradeStation;
		}
	}

	// 강화석 아이콘은 탭 전환 시 RefreshUI에서 동적 설정

	if (Btn_RuneTab)
	{
		Btn_RuneTab->OnClicked.AddDynamic(this, &UT3UpgradeUIWidget::OnRuneTabClicked);
	}
	
	if (Btn_Synthesize)
	{
		Btn_Synthesize->OnClicked.AddDynamic(this, &UT3UpgradeUIWidget::OnSynthesizeClicked);
	}
	
	if (UpgradeStation)
	{
		UpgradeStation->OnSynthesisSlotsChanged.AddDynamic(this, &UT3UpgradeUIWidget::RefreshSynthesisUI);
	}
	
	// 초기 UI 갱신
	RefreshUI();
}

void UT3UpgradeUIWidget::OnWeaponTabClicked()
{
	CurrentTab = ET3EquipmentType::Weapon;
	
	bIsRuneTabActive = false;
	
	RefreshUI();
	
	OnChangedTap.Broadcast(bIsRuneTabActive);
}

void UT3UpgradeUIWidget::OnArmorTabClicked()
{
	CurrentTab = ET3EquipmentType::Armor;
	
	bIsRuneTabActive = false;

	RefreshUI();
	
	OnChangedTap.Broadcast(bIsRuneTabActive);
}

void UT3UpgradeUIWidget::OnRuneTabClicked()
{
	bIsRuneTabActive = true;
	
	RefreshSynthesisUI();
	
	OnRefreshRuneList();
	
	OnChangedTap.Broadcast(bIsRuneTabActive);
}

void UT3UpgradeUIWidget::OnUpgradeClicked()
{
	if (!UpgradeStation) return;

	// 현재 탭에 맞는 강화 실행 (강화석 자동 선택)
	UpgradeStation->UpgradeEquipment(CurrentTab);

	// 강화 후 UI 갱신 (레벨, 스탯, 버튼 상태 모두 업데이트)
	RefreshUI();
}

void UT3UpgradeUIWidget::OnSynthesizeClicked()
{
	if (!UpgradeStation)
	{
		return;
	}
	
	UpgradeStation->SynthesizeRune();
	
	OnRefreshRuneList();
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
	if (Txt_CurrentLevel)
	{
		if (UIData.bIsMaxLevel)
		{
			Txt_CurrentLevel->SetText(FText::FromString(FString::Printf(TEXT("+%d (MAX)"), UIData.CurrentLevel)));
		}
		else
		{
			Txt_CurrentLevel->SetText(FText::FromString(FString::Printf(TEXT("+%d"), UIData.CurrentLevel)));
		}
	}
	
	if (Txt_NextLevel)
	{
		if (UIData.bIsMaxLevel)
		{
			Txt_NextLevel->SetText(FText::FromString(FString::Printf(TEXT("+%d (MAX)"), UIData.CurrentLevel)));
		}
		else
		{
			Txt_NextLevel->SetText(FText::FromString(FString::Printf(TEXT("+%d"), UIData.CurrentLevel + 1)));
		}
	}

	// 스탯 표시
	// MAX: "공격력: 180" / 그 외: "공격력: 150 → 180"
	
	if (Text_StatName)
	{
		if (CurrentTab == ET3EquipmentType::Weapon)
		{
			Text_StatName->SetText(FText::FromString(FString::Printf(TEXT("공격력"))));
		}
		else
		{
			Text_StatName->SetText(FText::FromString(FString::Printf(TEXT("방어력"))));
		}
	}
	
	if (Txt_CurrentStat)
	{
		Txt_CurrentStat->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), UIData.CurrentStat)));
	}

	if (Txt_NextStat)
	{
		Txt_NextStat->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), UIData.NextLevelStat)));
	}
	
	// 강화석 아이콘 설정 (탭 전환 시 무기/방어구 아이콘 교체)
	if (Img_NormalStone)
	{
		if (UTexture2D* Icon = UpgradeStation->GetStoneIcon(CurrentTab, ET3UpgradeStoneGrade::Normal))
		{
			Img_NormalStone->SetBrushFromTexture(Icon);
		}
	}
	if (Img_EpicStone)
	{
		if (UTexture2D* Icon = UpgradeStation->GetStoneIcon(CurrentTab, ET3UpgradeStoneGrade::Epic))
		{
			Img_EpicStone->SetBrushFromTexture(Icon);
		}
	}
	if (Img_LegendaryStone)
	{
		if (UTexture2D* Icon = UpgradeStation->GetStoneIcon(CurrentTab, ET3UpgradeStoneGrade::Legendary))
		{
			Img_LegendaryStone->SetBrushFromTexture(Icon);
		}
	}

	// 강화석 아이콘 색상 처리 (사용 가능: 원래 색상 / 사용 불가: 어둡게)
	const FLinearColor ActiveColor = FLinearColor::White;
	const FLinearColor InactiveColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);

	if (Img_NormalStone)
	{
		bool bCanUse = UpgradeStation->CanUseStone(ET3UpgradeStoneGrade::Normal, UIData.CurrentLevel)
			&& UpgradeStation->GetStoneCount(CurrentTab, ET3UpgradeStoneGrade::Normal) > 0;
		Img_NormalStone->SetColorAndOpacity(bCanUse ? ActiveColor : InactiveColor);
	}
	if (Img_EpicStone)
	{
		bool bCanUse = UpgradeStation->CanUseStone(ET3UpgradeStoneGrade::Epic, UIData.CurrentLevel)
			&& UpgradeStation->GetStoneCount(CurrentTab, ET3UpgradeStoneGrade::Epic) > 0;
		Img_EpicStone->SetColorAndOpacity(bCanUse ? ActiveColor : InactiveColor);
	}
	if (Img_LegendaryStone)
	{
		bool bCanUse = UpgradeStation->CanUseStone(ET3UpgradeStoneGrade::Legendary, UIData.CurrentLevel)
			&& UpgradeStation->GetStoneCount(CurrentTab, ET3UpgradeStoneGrade::Legendary) > 0;
		Img_LegendaryStone->SetColorAndOpacity(bCanUse ? ActiveColor : InactiveColor);
	}

	// 소모 예정 강화석 Border 강조
	const FLinearColor SelectedBorderColor = FLinearColor(1.0f, 0.85f, 0.0f, 1.0f);
	const FLinearColor DefaultBorderColor = FLinearColor::Transparent;

	ET3UpgradeStoneGrade NextGrade;
	bool bHasNextStone = UpgradeStation->GetNextStoneGrade(CurrentTab, UIData.CurrentLevel, NextGrade);

	if (Border_NormalStoneFocus)
	{
		bool bIsSelected = bHasNextStone && NextGrade == ET3UpgradeStoneGrade::Normal;
		Border_NormalStoneFocus->SetBrushColor(bIsSelected ? SelectedBorderColor : DefaultBorderColor);
	}
	if (Border_EpicStoneFocus)
	{
		bool bIsSelected = bHasNextStone && NextGrade == ET3UpgradeStoneGrade::Epic;
		Border_EpicStoneFocus->SetBrushColor(bIsSelected ? SelectedBorderColor : DefaultBorderColor);
	}
	if (Border_LegendaryStoneFocus)
	{
		bool bIsSelected = bHasNextStone && NextGrade == ET3UpgradeStoneGrade::Legendary;
		Border_LegendaryStoneFocus->SetBrushColor(bIsSelected ? SelectedBorderColor : DefaultBorderColor);
	}

	// 강화석 보유량 표시 (현재 탭의 장비 타입 기준)
	if (Txt_NormalStoneCount)
	{
		Txt_NormalStoneCount->SetText(FText::Format(FText::FromString(TEXT("보유량 : {0}")), FText::AsNumber(UpgradeStation->GetStoneCount(CurrentTab, ET3UpgradeStoneGrade::Normal))));
	}
	if (Txt_EpicStoneCount)
	{
		Txt_EpicStoneCount->SetText(FText::Format(FText::FromString(TEXT("보유량 : {0}")), FText::AsNumber(UpgradeStation->GetStoneCount(CurrentTab, ET3UpgradeStoneGrade::Epic))));
	}
	if (Txt_LegendaryStoneCount)
	{
		Txt_LegendaryStoneCount->SetText(FText::Format(FText::FromString(TEXT("보유량 : {0}")), FText::AsNumber(UpgradeStation->GetStoneCount(CurrentTab, ET3UpgradeStoneGrade::Legendary))));
	}

	// 강화 버튼 활성화/비활성화
	// bCanUpgrade는 최대레벨 or 강화석 없음이면 이미 false
	if (Btn_Upgrade)
	{
		Btn_Upgrade->SetIsEnabled(UIData.bCanUpgrade);
	}
}

void UT3UpgradeUIWidget::RefreshSynthesisUI()
{
	if (!UpgradeStation)
	{
		return;
	}

	UT3InventoryComponent* Inventory = UpgradeStation->GetPlayerInventoryComponent();
	
	const TArray<FName>& Slots = UpgradeStation->SynthesisSlots;

	UT3SynthesisSlotWidget* SlotWidgets[3] = { SynthesisSlot_0, SynthesisSlot_1, SynthesisSlot_2 };

	for (int32 i = 0; i < 3; i++)
	{
		if (!SlotWidgets[i]) continue;

		if (Slots[i] == NAME_None)
		{
			SlotWidgets[i]->ClearRuneIcon();
			continue;
		}

		if (Inventory && Inventory->RuneTable)
		{
			const FT3RuneItemData* RuneRow = Inventory->RuneTable->FindRow<FT3RuneItemData>(Slots[i], TEXT("RefreshSynthesisUI"));
			if (RuneRow && RuneRow->ItemData.Icon)
			{
				SlotWidgets[i]->SetRuneIcon(RuneRow->ItemData.Icon);
			}
		}
	}

	if (Btn_Synthesize)
	{
		Btn_Synthesize->SetIsEnabled(UpgradeStation->CanSynthesize());
	}
	
	OnRefreshRuneList();
}