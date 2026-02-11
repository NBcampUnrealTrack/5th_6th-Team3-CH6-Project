#include "GameSystem/T3GameMode.h"

#include "Equipment/T3PlayerEquipmentComponent.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Player/T3CharacterBase.h"

void AT3GameMode::BeginPlay()
{
	Super::BeginPlay();
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
}

bool AT3GameMode::SaveGame(const AT3CharacterBase* Character)
{
	//캐릭터 정보를 저장된 게임 데이터에 저장한다.
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	//캐릭터 상태
	SaveGame->PlayerLocation = Character->GetActorLocation();
	//스탯
	SaveGame->CurrentHP = Character->GetCurrentHP();
	SaveGame->MaxMana = Character->GetMaxMana();
	SaveGame->CurrentMana = Character->GetCurrentMana();
	SaveGame->MaxStamina = Character->GetMaxStamina();
	SaveGame->CurrentStamina = Character->GetCurrentStamina();
	SaveGame->AttackPower = Character->GetAttackPower();
	SaveGame->CriticalChance = Character->GetCriticalChance();
	SaveGame->CriticalDamage = Character->GetCriticalDamage();
	SaveGame->MoveSpeed = Character->GetMoveSpeed();
	//인벤토리
	TObjectPtr<UT3InventoryComponent> InventoryComponent = Character->InventoryComponent;
	SaveGame->Items.Empty();
	for (FInventorySlot& Slot : InventoryComponent->Items)
	{
		SaveGame->Items.Add(Slot);
	}
	SaveGame->Money = InventoryComponent->GetMoney();
	SaveGame->NormalStoneCount = InventoryComponent->GetNormalStoneCount();
	SaveGame->EpicStoneCount = InventoryComponent->GetEpicStoneCount();
	SaveGame->LegendaryStoneCount = InventoryComponent->GetLegendaryStoneCount();
	//장비
	if (UT3PlayerEquipmentComponent* EquipComp = Character->FindComponentByClass<UT3PlayerEquipmentComponent>())
	{
		EquipComp->GetEquipmentSaveData(SaveGame->WeaponSaveData, SaveGame->ArmorSaveData);
	}
	
	//저장
	return T3GameInstance->SaveGame();
}

void AT3GameMode::SetCharacterBySavedData(AT3CharacterBase* Character)
{
	//저장된 게임 데이터에서 캐릭터 정보를 가져온다. 
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	//캐릭터 상태
	Character->SetActorLocation(SaveGame->PlayerLocation);
	//스탯
	Character->SetCurrentHP(SaveGame->CurrentHP);
	Character->SetCurrentMana(SaveGame->CurrentMana);
	Character->SetCurrentStamina(SaveGame->CurrentStamina);
	Character->SetAttackPower(SaveGame->AttackPower);
	Character->SetCriticalChance(SaveGame->CriticalChance);
	Character->SetCriticalDamage(SaveGame->CriticalDamage);
	Character->SetMoveSpeed(SaveGame->MoveSpeed);
	//인벤토리
	TObjectPtr<UT3InventoryComponent> InventoryComponent = Character->InventoryComponent;
	const int32 SlotNums = SaveGame->Items.Num();
	for (int32 iNum = 0; iNum < SlotNums; ++iNum)
	{
		InventoryComponent->Items[iNum] = SaveGame->Items[iNum];
	}
	InventoryComponent->SetMoney(SaveGame->Money);
	InventoryComponent->SetNormalStoneCount(SaveGame->NormalStoneCount);
	InventoryComponent->SetEpicStoneCount(SaveGame->EpicStoneCount);
	InventoryComponent->SetLegendaryStoneCount(SaveGame->LegendaryStoneCount);
	//장비
	if (UT3PlayerEquipmentComponent* EquipComp = Character->FindComponentByClass<UT3PlayerEquipmentComponent>())
	{
		EquipComp->LoadEquipmentFromSave(SaveGame->WeaponSaveData, SaveGame->ArmorSaveData);
	}
}
