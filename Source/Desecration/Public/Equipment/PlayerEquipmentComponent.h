// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EquipmentTypes.h"
#include "TestItemInstance.h"
#include "PlayerEquipmentComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DESECRATION_API UPlayerEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UPlayerEquipmentComponent();
	
protected:
	virtual void BeginPlay() override;
	
public:
	// ==========================================================
	// 설정 (에디터에서 할당)
	// ==========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Data|Weapon")
	TObjectPtr<UDataTable> WeaponBaseTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data|Weapon")
	TObjectPtr<UDataTable> WeaponGrowthTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data|Armor")
	TObjectPtr<UDataTable> ArmorBaseTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data|Armor")
	TObjectPtr<UDataTable> ArmorGrowthTable;

	// 초기 장비 ID
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FName DefaultWeaponID;

	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	FName DefaultArmorID;

	// ==========================================================
	// 상태 (현재 장착 중인 것)
	// ==========================================================
	// 무기 (나중에 인벤토리에서 이리로 옮겨옴)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	TObjectPtr<UTestItemInstance> WeaponInstance;

	// 방어구
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	TObjectPtr<UTestItemInstance> ArmorInstance;
	
	// 실제로 월드에 보여지는 액터 (메쉬, 이펙트 껍데기)
	UPROPERTY(VisibleInstanceOnly, Category = "Visual")
	TObjectPtr<AActor> SpawnedWeaponActor;

	// ==========================================================
	// 기능
	// ==========================================================
	// 무기를 장착하는 함수 (객체를 받아서 처리)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipWeapon(UTestItemInstance* NewItem);
	
	// [신규] 방어구 장착 (이게 없어서 안 됐던 것!)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipArmor(UTestItemInstance* NewItem);
	
	// [통합] 강화 함수
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool TryUpgrade(EEquipmentType TargetType, int32 MaxAllowedLevel);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetCurrentAttackPower() const { return CurrentAttackPower; }

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetCurrentDefensePower() const { return CurrentDefensePower; }


	// [신규] 룬 데이터 테이블
	UPROPERTY(EditDefaultsOnly, Category = "Data|Rune")
	TObjectPtr<UDataTable> RuneTable;
	
	// [신규] 룬 장착 기능
	UFUNCTION(BlueprintCallable, Category = "Rune")
	bool TrySocketRune(UTestItemInstance* TargetItem, FName RuneID);
	
protected:
	
	// [신규] 실제로 계산된 스탯 값을 저장하는 캐시 변수
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	float CurrentAttackPower;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stats")
	float CurrentDefensePower;

private:
	// 내부적으로 비주얼(메쉬, 이펙트) 업데이트
	void UpdateWeaponVisuals();

	void UpdateArmorVisuals(); // 방어구 외형/스탯 갱신용
	
	// [신규] 내부적으로 스탯을 다시 계산하고 변수를 업데이트하는 함수
	void RefreshStats();

	// [이동] 기존의 무거운 로직은 여기로 숨깁니다.
	float CalculateWeaponPower() const;
	float CalculateArmorPower() const;
	
    // 룬 보너스 합산 헬퍼
    float CalculateRuneTotalBonus(const UTestItemInstance* Item, ERuneStatType StatType) const;
};
