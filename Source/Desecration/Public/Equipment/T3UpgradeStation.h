// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Equipment/T3EquipmentTypes.h"
#include "T3UpgradeStation.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class AT3CharacterBase;
class UT3PlayerEquipmentComponent;
class UT3InventoryComponent;
class AT3PlayerController;

// ============================================================================
// FT3UpgradeUIData - UI에 표시할 장비 정보 구조체
// ============================================================================
USTRUCT(BlueprintType)
struct FT3UpgradeUIData
{
	GENERATED_BODY()

	// 장비 ID (RowName, 코드용)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FName ItemID = NAME_None;

	// 장비 표시 이름 (UI용, 현지화 가능)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FText DisplayName;

	// 장비 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 장비 타입
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	ET3EquipmentType EquipmentType = ET3EquipmentType::Weapon;

	// 현재 강화 레벨
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	int32 CurrentLevel = 0;

	// 현재 스탯 (공격력 or 방어력)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	float CurrentStat = 0.0f;

	// 다음 레벨 스탯 (강화 미리보기)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	float NextLevelStat = 0.0f;

	// 강화 가능 여부
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bCanUpgrade = false;

	// 최대 레벨 도달 여부
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsMaxLevel = false;

	// 장비 장착 여부
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsEquipped = false;
};

// ============================================================================
// 델리게이트 선언 (UI 갱신용)
// ============================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeUIOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeUIClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeSuccess, ET3EquipmentType, UpgradedType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeFailed, FText, FailReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSynthesisSuccess, FName, ResultRuneID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSynthesisFailed, FText, FailReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSynthesisSlotsChanged);

// ============================================================================
// AT3UpgradeStation - 강화 스테이션 액터
// ============================================================================
UCLASS()
class DESECRATION_API AT3UpgradeStation : public AActor
{
	GENERATED_BODY()

public:
	AT3UpgradeStation();

protected:
	virtual void BeginPlay() override;

	// ========================================================================
	// 컴포넌트
	// ========================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneComponent;

	// ========================================================================
	// 설정
	// ========================================================================
	// 강화 최대 레벨 제한
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 MaxUpgradeLevel = 10;

	// ========================================================================
	// 강화석 아이콘 (에디터에서 설정) — 무기/방어구 × 3등급 = 6종
	// ========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons|Weapon")
	TObjectPtr<UTexture2D> WeaponNormalStoneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons|Weapon")
	TObjectPtr<UTexture2D> WeaponEpicStoneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons|Weapon")
	TObjectPtr<UTexture2D> WeaponLegendaryStoneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons|Armor")
	TObjectPtr<UTexture2D> ArmorNormalStoneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons|Armor")
	TObjectPtr<UTexture2D> ArmorEpicStoneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons|Armor")
	TObjectPtr<UTexture2D> ArmorLegendaryStoneIcon;

	// ========================================================================
	// 상태
	// ========================================================================
	// UI 열림 상태
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsUpgradeUIOpen = false;

	// 위젯 클래스 (에디터에서 WBP_UpgradeUI 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> UpgradeWidgetClass;

	// 현재 생성된 위젯 인스턴스
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> UpgradeWidgetInstance;

public:
	// ========================================================================
	// UI 델리게이트 (Blueprint에서 바인딩)
	// ========================================================================
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeUIOpened OnUpgradeUIOpened;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeUIClosed OnUpgradeUIClosed;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeSuccess OnUpgradeSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeFailed OnUpgradeFailed;

	// ========================================================================
	// UI 제어 함수 (Core - Widget Blueprint에서 호출)
	// ========================================================================
	// UI 열기/닫기
	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	void OpenUpgradeUI(AT3PlayerController* T3PC);

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	void CloseUpgradeUI(AT3PlayerController* T3PC);

	UFUNCTION(BlueprintPure, Category = "Upgrade|UI")
	bool IsUpgradeUIOpen() const { return bIsUpgradeUIOpen; }

	// ========================================================================
	// 장비 정보 조회 (Core - Widget Blueprint에서 호출)
	// ========================================================================
	// 무기 정보 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Data")
	FT3UpgradeUIData GetWeaponUIData() const;

	// 방어구 정보 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Data")
	FT3UpgradeUIData GetArmorUIData() const;

	// 특정 타입 장비 정보 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Data")
	FT3UpgradeUIData GetEquipmentUIData(ET3EquipmentType EquipmentType) const;

	// ========================================================================
	// 강화석 조회 (Core - Widget Blueprint에서 직접 호출)
	// 강화석은 장비별 데이터가 아닌 스테이션 공유 자원이므로
	// UIData가 아닌 UpgradeStation 레퍼런스에서 직접 접근
	// ========================================================================

	// 특정 장비 타입 + 등급 강화석 보유량 조회
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	int32 GetStoneCount(ET3EquipmentType EquipmentType, ET3UpgradeStoneGrade Grade) const;

	// 특정 장비 타입 + 등급 강화석 아이콘 조회
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	UTexture2D* GetStoneIcon(ET3EquipmentType EquipmentType, ET3UpgradeStoneGrade Grade) const;

	// 현재 장비 레벨에 사용 가능한 강화석 등급 목록 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Currency")
	TArray<ET3UpgradeStoneGrade> GetAvailableStones(ET3EquipmentType EquipmentType, int32 CurrentEquipmentLevel) const;

	// 특정 등급 강화석이 해당 레벨에 사용 가능한지
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	bool CanUseStone(ET3UpgradeStoneGrade Grade, int32 CurrentEquipmentLevel) const;

	// 강화석 최대 적용 레벨 조회 (Normal→3, Epic→5, Legendary→7)
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	static int32 GetMaxLevelForStone(ET3UpgradeStoneGrade Grade);

	// 현재 장비 타입/레벨에서 자동 선택될 강화석 등급 조회 (UI 표시용)
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	bool GetNextStoneGrade(ET3EquipmentType EquipmentType, int32 CurrentEquipmentLevel, ET3UpgradeStoneGrade& OutGrade) const;

	// ========================================================================
	// 강화 실행 (Core - Widget Blueprint에서 호출)
	// 사용 가능한 강화석 중 최하급부터 자동 소모
	// ========================================================================
	// 무기 강화 (버튼 클릭 시)
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Action")
	bool UpgradeWeapon();

	// 방어구 강화 (버튼 클릭 시)
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Action")
	bool UpgradeArmor();

	// 특정 타입 장비 강화
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Action")
	bool UpgradeEquipment(ET3EquipmentType EquipmentType);

private:
	// 현재 레벨에서 사용 가능한 최하급 강화석 자동 선택
	bool SelectLowestAvailableStone(ET3EquipmentType EquipmentType, int32 CurrentEquipmentLevel, ET3UpgradeStoneGrade& OutGrade) const;

	// 강화석 1개 차감
	void ConsumeStone(ET3EquipmentType EquipmentType, ET3UpgradeStoneGrade Grade);

public:

	// ========================================================================
	// 유틸리티 (Core)
	// ========================================================================
	// EquipmentComponent 조회 헬퍼
	UFUNCTION(BlueprintPure, Category = "Upgrade|Utility")
	UT3PlayerEquipmentComponent* GetPlayerEquipmentComponent() const;

	// InventoryComponent 조회 헬퍼
	UFUNCTION(BlueprintPure, Category = "Upgrade|Utility")
	UT3InventoryComponent* GetPlayerInventoryComponent() const;

#pragma region Synthesis
public:
	UPROPERTY(BlueprintReadOnly, Category = "Rune|Synthesis")
	TArray<FName> SynthesisSlots;

	UFUNCTION(BlueprintCallable, Category = "Rune|Synthesis")
	bool AddRuneToSynthesisSlot(FName RuneID);

	UFUNCTION(BlueprintCallable, Category = "Rune|Synthesis")
	bool RemoveRuneFromSynthesisSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Rune|Synthesis")
	bool CanAddRuneToSlot(FName RuneID) const;

	UFUNCTION(BlueprintPure, Category = "Rune|Synthesis")
	bool CanSynthesize() const;

	UFUNCTION(BlueprintCallable, Category = "Rune|Synthesis")
	bool SynthesizeRune();

	UFUNCTION(BlueprintCallable, Category = "Rune|Synthesis")
	void ClearSynthesisSlots();

	UFUNCTION(BlueprintPure, Category = "Rune|Synthesis")
	FName GetLockedRuneID() const;
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSynthesisSuccess OnSynthesisSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSynthesisFailed OnSynthesisFailed;
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnSynthesisSlotsChanged OnSynthesisSlotsChanged;
#pragma endregion
};
