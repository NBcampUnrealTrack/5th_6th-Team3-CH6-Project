#include "GameSystem/T3SaveGame.h"

#include "Equipment/T3EquipmentTypes.h"
#include "Equipment/T3PlayerEquipmentComponent.h" 
#include "GameSystem/T3GameInstance.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Player/T3CharacterDataAsset.h"

void UT3SaveGame::ResetGameData()
{
	//스탯 초기화를 위한 게임 인스턴스
	// TObjectPtr<UT3GameInstance> T3GameInstance = Cast<UT3GameInstance>(GetWorld()->GetGameInstance());
	// if (!T3GameInstance)
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
	// 	return;
	// }
	
	PlayerClass = EPlayerClass::None;
	PlayerName = TEXT("");
	//캐릭터 위치
	SavedLevelName = ELevelName::Tutorial;
	PlayerLocation = FVector(-80, 185, 102);
	bSetLocation = true;
	
	//스탯
	//TODO : 하드코딩된 초기 스탯 수정하기
	//const TObjectPtr<UT3CharacterDataAsset> CharacterData = T3GameInstance->GetCharacterData();
	MaxHP = 150.0f;
	CurrentHP = MaxHP;
	MaxMana = 100.0f;
	CurrentMana = MaxMana;
	MaxStamina = 100.0f;
	CurrentStamina = MaxStamina;
	AttackPower = 50.0f;
	CriticalChance = 0.1f;
	CriticalDamage = 1.5f;
	MoveSpeed = 500.0f;
	
	//인벤토리
	//TODO : 인벤토리 크기 확인하기
	for (int32 iNum = 0; iNum < 20; iNum++)
	{
		Items.Add(FInventorySlot());
	}
	Money = 0;
	//TODO : 포션 강화 완성시 초기화 연동
	//강화석
	NormalStoneCount = 0;
	EpicStoneCount = 0;
	LegendaryStoneCount = 0;
	
	//장비
	WeaponSaveData.ItemID = NAME_None;
	WeaponSaveData.Level = 0;
	WeaponSaveData.Type = ET3EquipmentType::Weapon;

	ArmorSaveData.ItemID = NAME_None;
	ArmorSaveData.Level = 0;
	ArmorSaveData.Type = ET3EquipmentType::Armor;
	
	//TODO : 스킬 초기화
}
