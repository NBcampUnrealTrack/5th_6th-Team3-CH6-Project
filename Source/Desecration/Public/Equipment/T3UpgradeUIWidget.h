// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Equipment/T3EquipmentTypes.h"
#include "T3UpgradeUIWidget.generated.h"

class AT3UpgradeStation;
class UTextBlock;
class UButton;
class UImage;
class UBorder;

/**
 * 강화 UI 위젯 베이스 클래스
 * WBP_UpgradeUI의 Parent Class로 지정하여 사용
 * BindWidget으로 Blueprint 위젯과 C++ 로직을 연결
 */
UCLASS()
class DESECRATION_API UT3UpgradeUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// UpgradeStation 레퍼런스
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
	TObjectPtr<AT3UpgradeStation> UpgradeStation;

	// 현재 선택된 탭
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
	ET3EquipmentType CurrentTab = ET3EquipmentType::Weapon;

	// UI 갱신 (C++에서 전부 처리)
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	void RefreshUI();

protected:
	virtual void NativeConstruct() override;

	// ========================================================================
	// BindWidget - Designer에서 같은 이름의 위젯과 자동 연결
	// ========================================================================

	// 장비 아이콘
	// 현재: 탭 전환 시 현재 장착된 장비의 Icon 텍스처를 표시
	// 추후: 장비 종류 확장 시 선택된 아이템에 따라 동적으로 교체 가능
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Icon;

	// 장비 이름
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_DisplayName;

	// 강화 레벨 (예: "+3 → +4" / "+7 (MAX)")
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Level;

	// 스탯 (예: "공격력: 150 → 180" / "방어력: 200")
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Stat;

	// 강화석 Border (소모 예정 강조용)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_NormalStone;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_RareStone;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_EpicStone;

	// 강화석 아이콘 (등급별)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_NormalStone;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_RareStone;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_EpicStone;

	// 강화석 보유량 (등급별)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_NormalStoneCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_RareStoneCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_EpicStoneCount;

	// 강화 버튼 (최대레벨 or 강화석 없음 → 비활성화)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Upgrade;

	// 탭 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_WeaponTab;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_ArmorTab;

private:
	// 탭 버튼 클릭 핸들러
	UFUNCTION()
	void OnWeaponTabClicked();

	UFUNCTION()
	void OnArmorTabClicked();

	// 강화 버튼 클릭 핸들러
	UFUNCTION()
	void OnUpgradeClicked();
};
