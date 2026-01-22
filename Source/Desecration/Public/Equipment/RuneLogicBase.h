// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EquipmentTypes.h"
#include "RuneLogicBase.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class DESECRATION_API URuneLogicBase : public UObject
{
	GENERATED_BODY()
	
		
public:
	// =========================================================
	// 1. 설정값 저장소 (데이터 테이블에서 주입받을 변수들)
	// =========================================================
	UPROPERTY(BlueprintReadOnly, Category = "Config")
	ERuneStatType ConfigStatType;

	UPROPERTY(BlueprintReadOnly, Category = "Config")
	float ConfigValue;

	// =========================================================
	// 2. 초기화 함수 (컴포넌트가 호출해줌)
	// =========================================================
	virtual void Init(ERuneStatType InType, float InValue);

	// =========================================================
	// 3. 기능 함수
	// =========================================================
    
	// 스탯 보너스 계산 (기본 구현: 설정된 타입과 맞으면 값 리턴)
	UFUNCTION(BlueprintNativeEvent, Category = "Rune")
	float GetStatBonus(ERuneStatType CheckType) const;
	virtual float GetStatBonus_Implementation(ERuneStatType CheckType) const;

	// 장착 시 특수 로직 (무적, 버프 등)
	UFUNCTION(BlueprintNativeEvent, Category = "Rune")
	void OnEquip(class ACharacter* OwnerChar);
	virtual void OnEquip_Implementation(class ACharacter* OwnerChar) {} // 기본은 빈 껍데기

	UFUNCTION(BlueprintNativeEvent, Category = "Rune")
	void OnUnequip(class ACharacter* OwnerChar);
	virtual void OnUnequip_Implementation(class ACharacter* OwnerChar) {}
};
