// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Equipment/T3EquipmentTypes.h"
#include "Equipment/T3TestItemInstance.h"
#include "Item/Rune/T3RuneBase.h"
#include "T3PlayerEquipmentComponent.generated.h"

struct FT3RuneItemData;
class AT3CharacterBase;

// 장비 스탯 변경 시 발송되는 델리게이트
// 캐릭터팀에서 바인딩하여 공격력/방어력을 동기화
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipmentStatsChanged, float, NewAttackPower, float, NewDefensePower, float, NewWeaponLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRuneSocketChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DESECRATION_API UT3PlayerEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT3PlayerEquipmentComponent();

	// 장비 스탯 변경 델리게이트 (캐릭터팀 바인딩용)
	// BeginPlay 초기 장착, 강화, 룬 장착 시 자동 발송
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquipmentStatsChanged OnEquipmentStatsChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<AT3CharacterBase> OwnerCharacter = nullptr;

public:
	// ==========================================================
	// 설정 (에디터에서 할당)
	// ==========================================================
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UDataTable> WeaponTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UDataTable> ArmorTable;

	UPROPERTY(EditDefaultsOnly, Category = "Data|Rune")
	int32 MaxRuneSockets;

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
	TObjectPtr<UT3TestItemInstance> WeaponInstance;

	// 방어구
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	TObjectPtr<UT3TestItemInstance> ArmorInstance;

	// 실제로 월드에 보여지는 액터 (메쉬, 이펙트 껍데기)
	UPROPERTY(VisibleInstanceOnly, Category = "Visual")
	TObjectPtr<AActor> SpawnedWeaponActor;

	// ==========================================================
	// 룬 상태 (EquipmentComponent가 직접 관리)
	// ==========================================================
	// [저장용] 소켓된 룬 ID 목록
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Rune")
	TArray<FName> WeaponSocketedRuneIDs;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Rune")
	TArray<FName> ArmorSocketedRuneIDs;

	// [런타임용] 활성화된 룬 객체 (저장 불필요, Transient)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UT3RuneBase>> WeaponActiveRunes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UT3RuneBase>> ArmorActiveRunes;
	
	// ==========================================================
	// 기능
	// ==========================================================
	// 무기를 장착하는 함수 (객체를 받아서 처리)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipWeapon(UT3TestItemInstance* NewItem);

	// [신규] 방어구 장착 (이게 없어서 안 됐던 것!)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipArmor(UT3TestItemInstance* NewItem);
	// 세이브 데이터로 장비 복원 (세이브팀에서 호출)
	// 기존 장비를 교체하고 스탯을 재계산함
	UFUNCTION(BlueprintCallable, Category = "Equipment|Save")
	void LoadEquipmentFromSave(const FT3ItemSaveData& WeaponData, const FT3ItemSaveData& ArmorData);

	// 현재 장비 상태를 세이브 데이터로 반환 (세이브팀에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Equipment|Save")
	void GetEquipmentSaveData(FT3ItemSaveData& OutWeaponData, FT3ItemSaveData& OutArmorData) const;
	
	// [통합] 강화 함수
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool TryUpgrade(ET3EquipmentType TargetType, int32 MaxAllowedLevel);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetCurrentAttackPower() const { return CurrentAttackPower; }

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetCurrentDefensePower() const { return CurrentDefensePower; }

	UFUNCTION(BlueprintCallable)
	bool GetSocketedRuneData(ET3EquipmentType EquipmentType, int32 SlotIndex, FT3RuneItemData& OutRuneData) const;
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

	float CalculateWeaponPower() const;
	float CalculateArmorPower() const;

	void RestoreRunes(const TArray<FName>& RuneIDs, TArray<TObjectPtr<UT3RuneBase>>& OutActiveRunes);

#pragma region Rune
public:
	UPROPERTY(BlueprintAssignable)
	FOnRuneSocketChanged OnRuneSocketChanged;
	
	UFUNCTION(BlueprintCallable, Category = "Rune")
	bool SocketRune(FName RuneID, ET3EquipmentType TargetEquipment);

	UFUNCTION(BlueprintCallable, Category = "Rune")
	bool UnsocketRune(FName RuneID, ET3EquipmentType TargetEquipment);

	UFUNCTION(BlueprintCallable, Category = "Rune")
	bool SocketRuneAuto(FName RuneID);
	
private:
	TMap<TSubclassOf<UT3RuneBase>, float> RuneCooldownEndTimeMap;
	
#pragma endregion
};
