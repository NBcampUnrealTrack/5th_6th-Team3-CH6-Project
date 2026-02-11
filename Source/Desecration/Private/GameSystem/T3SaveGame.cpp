#include "GameSystem/T3SaveGame.h"

#include "Equipment/T3EquipmentTypes.h"
#include "Equipment/T3PlayerEquipmentComponent.h" 

void UT3SaveGame::ResetGameData()
{
	PlayerClass = EPlayerClass::None;
	PlayerName = TEXT("");
	SavedLevelName = ELevelName::Tutorial;
	PlayerLocation = FVector::ZeroVector;
	
	// 장비 초기화
	WeaponSaveData.ItemID = NAME_None;
	WeaponSaveData.Level = 0;
	WeaponSaveData.Type = ET3EquipmentType::Weapon;

	ArmorSaveData.ItemID = NAME_None;
	ArmorSaveData.Level = 0;
	ArmorSaveData.Type = ET3EquipmentType::Armor;

	// 강화석 초기화
	NormalStoneCount = 0;
	EpicStoneCount = 0;
	LegendaryStoneCount = 0;
}
