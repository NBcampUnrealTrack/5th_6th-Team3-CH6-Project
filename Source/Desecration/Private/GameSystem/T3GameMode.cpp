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

EPlayerClass AT3GameMode::GetPlayerClass()
{
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	return SaveGame->PlayerClass;
}

bool AT3GameMode::SaveGame(const AT3CharacterBase* Character, const ELevelName LevelName, const bool bTemporarySave)
{
	//캐릭터 정보를 저장된 게임 데이터에 저장한다.
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	if (!SaveGame)
	{
		return false;
	}
	//현재 위치
	SaveGame->SavedLevelName = LevelName;
	SaveGame->PlayerLocation = Character->GetActorLocation();
	//스탯
	SaveGame->MaxHP = Character->GetMaxHP();
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
	
	//임시 저장이라면 세이브 데이터를 가지고만 있고 직접 저장하지 않는다.
	if (bTemporarySave)
	{
		return true;
	}
	
	//저장
	return T3GameInstance->SaveGame();
}

void AT3GameMode::LoadGame()
{
	//저장된 게임을 불러오는데 성공하면 그 맵으로 이동
	if (T3GameInstance->LoadGame())
	{
		const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
		T3GameInstance->OpenLevel(SaveGame->SavedLevelName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s : Can't load the save game"), *GetNameSafe(this));
	}
}

/*
void AT3GameMode::SetCharacterBySavedData(AT3CharacterBase* Character)
{
	//저장된 게임 데이터에서 캐릭터 정보를 가져온다. 
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	if (!SaveGame)
	{
		return;
	}
	//캐릭터 상태
	//이동은 데이터 로드 후 1회만 적용
	if (SaveGame->bSetLocation)
	{
		SaveGame->bSetLocation = false;
		Character->SetActorLocation(SaveGame->PlayerLocation);
	}
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
		if (InventoryComponent->Items.IsValidIndex(iNum))
		{
			InventoryComponent->Items[iNum] = SaveGame->Items[iNum];
		}
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
*/


void AT3GameMode::SetCharacterBySavedData(AT3CharacterBase* Character)
{
    if (!Character) return;

    // 1. GameInstance 유효성 검사 (가장 중요)
    if (!T3GameInstance) 
    {
        // 캐싱이 안 되어 있다면 여기서 시도
        T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
        if (!T3GameInstance) return;
    }

    const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
    if (!SaveGame) return;

    // 2. 위치 설정
    if (SaveGame->bSetLocation)
    {
        SaveGame->bSetLocation = false;
        Character->SetActorLocation(SaveGame->PlayerLocation);
    }

    // 3. 스탯 적용
    Character->SetCurrentHP(SaveGame->CurrentHP);
    Character->SetCurrentMana(SaveGame->CurrentMana);
    Character->SetCurrentStamina(SaveGame->CurrentStamina);
    Character->SetAttackPower(SaveGame->AttackPower);
    Character->SetCriticalChance(SaveGame->CriticalChance);
    Character->SetCriticalDamage(SaveGame->CriticalDamage);
    Character->SetMoveSpeed(SaveGame->MoveSpeed);

    // 4. 인벤토리 컴포넌트 유효성 검사 (매우 중요)
    TObjectPtr<UT3InventoryComponent> InventoryComponent = Character->InventoryComponent;
    if (InventoryComponent) // 여기서 Null 체크 필수!
    {
        const int32 SlotNums = SaveGame->Items.Num();
        for (int32 iNum = 0; iNum < SlotNums; ++iNum)
        {
            if (InventoryComponent->Items.IsValidIndex(iNum) && SaveGame->Items.IsValidIndex(iNum))
            {
                InventoryComponent->Items[iNum] = SaveGame->Items[iNum];
            }
        }
        InventoryComponent->SetMoney(SaveGame->Money);
        InventoryComponent->SetNormalStoneCount(SaveGame->NormalStoneCount);
        InventoryComponent->SetEpicStoneCount(SaveGame->EpicStoneCount);
        InventoryComponent->SetLegendaryStoneCount(SaveGame->LegendaryStoneCount);
    }
    else 
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryComponent is Null on %s"), *Character->GetName());
    }

    // 5. 장비 컴포넌트 유효성 검사
    if (UT3PlayerEquipmentComponent* EquipComp = Character->FindComponentByClass<UT3PlayerEquipmentComponent>())
    {
        EquipComp->LoadEquipmentFromSave(SaveGame->WeaponSaveData, SaveGame->ArmorSaveData);
    }
}