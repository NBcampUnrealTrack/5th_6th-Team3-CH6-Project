// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Equipment/T3EquipmentTypes.h"
#include "Equipment/T3RuneLogicBase.h"
#include "T3TestItemInstance.generated.h"

/**
 * 장비 아이템의 런타임 인스턴스
 */
UCLASS(BlueprintType)
class DESECRATION_API UT3TestItemInstance : public UObject
{
	GENERATED_BODY()


public:

	// 이 아이템의 원본 ID (DT_WeaponBase의 RowName, 예: "LongSword")
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Data")
	FName ItemID;

	// 현재 강화 레벨 (동적 상태)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Data")
	int32 CurrentLevel = 0;

	// [추가] 자신의 정체성 (무기? 방어구?)
	UPROPERTY()
	ET3EquipmentType ItemType;

	// [저장용] 박혀있는 룬 ID 목록
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rune")
	TArray<FName> SocketedRuneIDs;

	// [런타임용] 실제로 살아있는 룬 객체들
	UPROPERTY(Transient)
	TArray<UT3RuneLogicBase*> ActiveRunes;

	// 룬 장착 함수 (성공 시 true)
	bool AddRune(FName RuneID, int32 MaxSockets)
	{
		if (SocketedRuneIDs.Num() >= MaxSockets) return false; // 구멍 꽉 참
		SocketedRuneIDs.Add(RuneID);
		return true;
	}

	// 룬 제거 함수
	void RemoveRune(int32 SocketIndex)
	{
		if (SocketedRuneIDs.IsValidIndex(SocketIndex))
		{
			SocketedRuneIDs.RemoveAt(SocketIndex);
		}
	}

	// 초기화 함수
	void Init(FName InItemID, int32 InLevel, ET3EquipmentType InType)
	{
		ItemID = InItemID;
		CurrentLevel = InLevel;
		ItemType = InType;
	}
};
