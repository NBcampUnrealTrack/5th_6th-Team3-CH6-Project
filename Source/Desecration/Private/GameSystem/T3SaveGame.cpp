#include "GameSystem/T3SaveGame.h"

#include "Equipment/T3EquipmentTypes.h"
#include "Equipment/T3PlayerEquipmentComponent.h" 
#include "GameSystem/T3GameInstance.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Player/T3CharacterDataAsset.h"

void UT3SaveGame::ResetGameData()
{
	//참고 : 이하의 값 중 일부는 시작부터 다른 값으로 변경될 수 있으니 변경이 필요하면 사용 위치를 추적하는 것을 권장
	
	PlayerClass = ECharacterClass::Paladin;
	PlayerName = TEXT("");
	
	LevelProgressMap.Empty();
	LostMoneyList.Empty();
	LevelObjectStates.Empty();
	
	//캐릭터 위치
	SavedLevelName = ELevelName::Tutorial;
	PlayerLocation = FVector(-80, 185, 2860);
	PlayerRotation = FRotator::ZeroRotator;

	//스탯
	MaxHP = 150.0f;
	CurrentHP = MaxHP;
	MaxMana = 100.0f;
	CurrentMana = MaxMana;
	MaxStamina = 100.0f;
	CurrentStamina = MaxStamina;
	StatAttackPower = 5.0f;
	WeaponAttackPower = 45.0f;
	RuneAttackPower = 0.0f;
	CriticalChance = 0.1f;
	CriticalDamage = 1.5f;
	MoveSpeed = 500.0f;
	
	Vigor = 10;
	Endurance = 10;
	Mind = 10;
	Strength = 5;
	Intelligence = 5;
	WeaponLevel = 0;
	CharacterLevel = 1;
	
	//인벤토리
	constexpr int32 InvenSize = 20;
	Items.SetNum(InvenSize);
	RuneItems.SetNum(InvenSize);
	EtcItems.SetNum(InvenSize);
	for (int iNum = 0; iNum < InvenSize; iNum++)
	{
		Items[iNum].ItemID = RuneItems[iNum].ItemID = EtcItems[iNum].ItemID = NAME_None;
		Items[iNum].ItemStack = RuneItems[iNum].ItemStack = EtcItems[iNum].ItemStack = 0;
	}
	Money = 0;
	
	//포션
	HPPotionMaxCount = 3;
	MPPotionMaxCount = 3;
	PotionAmountUpgradeLevel = 0;
	PotionRecoveryUpgradeLevel = 0;
	
	//강화석
	WeaponNormalStoneCount = 0;
	WeaponEpicStoneCount = 0;
	WeaponLegendaryStoneCount = 0;
	ArmorNormalStoneCount = 0;
	ArmorEpicStoneCount = 0;
	ArmorLegendaryStoneCount = 0;
	
	//장비
	WeaponSaveData.ItemID = NAME_None;
	WeaponSaveData.Level = 0;
	WeaponSaveData.Type = ET3EquipmentType::Weapon;
	WeaponSaveData.SocketedRuneIDs.Empty();

	ArmorSaveData.ItemID = NAME_None;
	ArmorSaveData.Level = 0;
	ArmorSaveData.Type = ET3EquipmentType::Armor;
	ArmorSaveData.SocketedRuneIDs.Empty();
	
	AccessorySaveData.ItemID = NAME_None;
	AccessorySaveData.Level  = 0;
	
	ClassSocketedRuneIDs.Empty();
	
	//스킬
	SkillUnlockStates.Empty();
	EquippedSkillIDs.Empty();
	EquippedSkillIDs.Add(1); // 기본: 1번 스킬만 장착
}

void UT3SaveGame::SetStatByCharacterData(const TObjectPtr<UT3CharacterDataAsset> CharacterData)
{
	PlayerClass = CharacterData->CharacterClass;
		
	MaxHP = CharacterData->MaxHealth;
	CurrentHP = MaxHP;
}

void UT3SaveGame::AddLostMoney(FLostMoney NewLostMoney)
{
	//0원을 잃어버린다?
	if (NewLostMoney.Money <= 0)
	{
		return;
	}
	
	//LostMoneyList내 객체 개수를 키로 잡은 다음 중복이 있다면 1씩 더해서 확인
	int32 Key = LostMoneyList.Num();
	while (true)
	{
		if (!LostMoneyList.Contains(Key))
		{
			break;
		}
		++Key;
	}

	LostMoneyList.Emplace(Key, NewLostMoney);
}

void UT3SaveGame::RegainLostMoney(const int32 LostMoneyID)
{
	LostMoneyList.Remove(LostMoneyID);
}
