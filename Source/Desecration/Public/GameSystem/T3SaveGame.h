#pragma once

#include "CoreMinimal.h"
#include "T3SaveGameParent.h"
#include "Equipment/T3EquipmentTypes.h"
#include "GameSystem/T3GameInstance.h"
#include "T3SaveGame.generated.h"

struct FLevelProgressData;
class UT3CharacterDataAsset;
struct FInventorySlot;
struct FSkillData;
enum class ECharacterClass : uint8;

UCLASS()
class DESECRATION_API UT3SaveGame : public UT3SaveGameParent
{
	GENERATED_BODY()
	
public:
	//게임 데이터 초기화
	virtual void ResetGameData() override;
	
	//지정한 캐릭터 데이터로 스탯 변경
	void SetStatByCharacterData(TObjectPtr<UT3CharacterDataAsset> CharacterData);
	
	//플레이어의 클래스
	UPROPERTY()
	ECharacterClass PlayerClass;
	
	//플레이어 이름
	UPROPERTY()
	FString PlayerName;
	
	//저장한 맵 내의 위치
	UPROPERTY()
	FVector PlayerLocation;
	
	//저장했을때의 캐릭터 방향
	UPROPERTY()
	FRotator PlayerRotation;
	
#pragma region 레벨(맵)
	//저장한 곳의 맵 이름
	UPROPERTY()
	ELevelName SavedLevelName;
	
	//현재 도달한 세이브 포인트(룬) 위치
	UPROPERTY()
	TMap<ELevelName, FLevelProgressData> LevelProgressMap;
#pragma endregion
	
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
	
	//스탯 공격력
	UPROPERTY()
	float StatAttackPower;
	
	//무기 공격력
	UPROPERTY()
	float WeaponAttackPower;
	
	//룬 공격력
	UPROPERTY()
	float RuneAttackPower;
	
	//크리티컬 확률
	UPROPERTY()
	float CriticalChance;
	
	//크리티컬 대미지
	UPROPERTY()
	float CriticalDamage;
	
	//이동 속도
	UPROPERTY()
	float MoveSpeed;
	
	//체력
	UPROPERTY()
	int32 Vigor;
	
	//기력
	UPROPERTY()
	int32 Endurance;

	//정신력
	UPROPERTY()
	int32 Mind;
	
	//근력
	UPROPERTY()
	int32 Strength;

	//지력
	UPROPERTY()
	int32 Intelligence;
	
	//장비 강화 레벨
	UPROPERTY()
	float WeaponLevel;
	
	//캐릭터 레벨
	UPROPERTY()
	int32 CharacterLevel;
#pragma endregion
	
#pragma region 인벤토리
	//인벤토리내 아이템 목록
	UPROPERTY()
	TArray<FInventorySlot> Items;
	
	//인벤토리내 룬 목록
	UPROPERTY()
	TArray<FInventorySlot> RuneItems;
	
	//인벤토리내 기타 아이템 목록
	UPROPERTY()
	TArray<FInventorySlot> EtcItems;
	
	//보유 재화
	UPROPERTY()
	int32 Money;
	
	//HP포션 개수
	UPROPERTY()
	int32 HPPotionMaxCount;
	
	//MP포션 개수
	UPROPERTY()
	int32 MPPotionMaxCount;
	
	//포션 개수 강화 단계
	UPROPERTY()
	int32 PotionAmountUpgradeLevel;
	
	//포션 회복량 강화 단계
	UPROPERTY()
	int32 PotionRecoveryUpgradeLevel;
#pragma endregion
	
#pragma region 장비
	// 무기 저장 데이터 (ItemID + 강화 레벨 + 타입)
	UPROPERTY()
	FT3ItemSaveData WeaponSaveData;

	// 방어구 저장 데이터
	UPROPERTY()
	FT3ItemSaveData ArmorSaveData;
	
	// 장신구 저장 데이터
	UPROPERTY()
	FT3AccessorySaveData AccessorySaveData;
	
	// 직업 전용 소켓 룬 저장 데이터
	UPROPERTY()
	TArray<FName> ClassSocketedRuneIDs;
	
	// 무기 강화석 보유량
	UPROPERTY()
	int32 WeaponNormalStoneCount;

	UPROPERTY()
	int32 WeaponEpicStoneCount;

	UPROPERTY()
	int32 WeaponLegendaryStoneCount;

	// 방어구 강화석 보유량
	UPROPERTY()
	int32 ArmorNormalStoneCount;

	UPROPERTY()
	int32 ArmorEpicStoneCount;

	UPROPERTY()
	int32 ArmorLegendaryStoneCount;
#pragma endregion

#pragma region 스킬
	//보유중인 스킬 정보
	UPROPERTY()
	TMap<int32, bool> SkillUnlockStates;
	
	//현재 스킬
	UPROPERTY()
	int32 CurrentSkillSlot;
	
	//다음 스킬
	UPROPERTY()
	int32 NextSkillSlot;
#pragma endregion
};
