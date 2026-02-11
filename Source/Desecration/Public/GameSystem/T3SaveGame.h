#pragma once

#include "CoreMinimal.h"
#include "Equipment/T3EquipmentTypes.h"
#include "GameFramework/SaveGame.h"
#include "GameSystem/GlobalEnums.h"
#include "T3SaveGame.generated.h"

struct FInventorySlot;

UCLASS()
class DESECRATION_API UT3SaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	//게임 데이터 초기화
	void ResetGameData();
	
	//플레이어의 클래스
	UPROPERTY()
	EPlayerClass PlayerClass;
	
	//플레이어 이름
	UPROPERTY()
	FString PlayerName;
	
	//저장한 곳의 맵 이름
	UPROPERTY()
	ELevelName SavedLevelName;
	
	//저장한 맵 내의 위치
	UPROPERTY()
	FVector PlayerLocation;
	
#pragma region 캐릭터 스탯
	//최대 HP
	UPROPERTY()
	float MaxHP;
	
	//현재 HP
	UPROPERTY()
	float CurrentHP;
	
	//최대 마나
	UPROPERTY()
	float MaxMana;
	
	//현재 마나
	UPROPERTY()
	float CurrentMana;
	
	//최대 스테미나
	UPROPERTY()
	float MaxStamina;
	
	//현재 스테미나
	UPROPERTY()
	float CurrentStamina;
	
	//공격력
	UPROPERTY()
	float AttackPower;
	
	//크리티컬 확률
	UPROPERTY()
	float CriticalChance;
	
	//크리티컬 대미지
	UPROPERTY()
	float CriticalDamage;
	
	//이동 속도
	UPROPERTY()
	float MoveSpeed;
#pragma endregion
	
#pragma region 인벤토리
	//인벤토리내 아이템 목록
	UPROPERTY()
	TArray<FInventorySlot> Items;
	
	//보유 재화
	UPROPERTY()
	int32 Money;
	
	//TODO : 포션 강화 완성시 변수 타입 및 이름 변경하기
	//현재 HP포션 개수
	UPROPERTY()
	int32 CurrentHPPotionCount;
	
	//현재 MP포션 개수
	UPROPERTY()
	int32 CurrentMPPotionCount;
	
	//현재 HP포션 개수 강화 단계
	UPROPERTY()
	int32 CurrentHPPotionAmountLevel;
	
	//현재 HP포션 회복량 강화 단계
	UPROPERTY()
	int32 CurrentHPIncreaseAmountLevel;
	
	//현재 MP포션 개수 강화 단계
	UPROPERTY()
	int32 CurrentMPPotionAmountLevel;
	
	//현재 MP포션 회복량 강화 단계
	UPROPERTY()
	int32 CurrentMPIncreaseAmountLevel;
#pragma endregion
	
#pragma region 장비
	// 무기 저장 데이터 (ItemID + 강화 레벨 + 타입)
	UPROPERTY()
	FT3ItemSaveData WeaponSaveData;

	// 방어구 저장 데이터
	UPROPERTY()
	FT3ItemSaveData ArmorSaveData;
	
	// 하급 강화석 보유량
	UPROPERTY()
	int32 NormalStoneCount;

	// 중급 강화석 보유량
	UPROPERTY()
	int32 EpicStoneCount;

	// 상급 강화석 보유량
	UPROPERTY()
	int32 LegendaryStoneCount;
#pragma endregion

#pragma region 스킬
	//TODO : 스킬 타입 및 이름에 맞게 변경하기
	//보유 스킬
	UPROPERTY()
	TArray<int32> OwnedSkills;
	
	//스킬 슬롯
	UPROPERTY()
	TArray<int32> EquippedSkills;
#pragma endregion
};
