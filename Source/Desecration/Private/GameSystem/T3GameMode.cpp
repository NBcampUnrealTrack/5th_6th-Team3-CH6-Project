#include "GameSystem/T3GameMode.h"

#include "Equipment/T3PlayerEquipmentComponent.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3GameState.h"
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
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance가 NULL"), *GetNameSafe(this));
		return;
	}
	
	//게임 스테이트
	T3GameState = Cast<AT3GameState>(GetWorld()->GetGameState());
	if (!T3GameState)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameState가 NULL"), *GetNameSafe(this));
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
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : InventoryComponent가 null"), *GetNameSafe(this));
		return false;
	}
	SaveGame->Items.Empty();
	for (FInventorySlot& Slot : InventoryComponent->Items)
	{
		SaveGame->Items.Add(Slot);
	}
	SaveGame->Money = InventoryComponent->GetMoney();
	SaveGame->NormalStoneCount = InventoryComponent->GetNormalStoneCount();
	SaveGame->EpicStoneCount = InventoryComponent->GetEpicStoneCount();
	SaveGame->LegendaryStoneCount = InventoryComponent->GetLegendaryStoneCount();
	//포션
	SaveGame->HPPotionCount = InventoryComponent->GetHPPotionCount();
	SaveGame->MPPotionCount = InventoryComponent->GetMPPotionCount();
	SaveGame->PotionAmountUpgradeLevel = InventoryComponent->GetPotionAmountUpgradeLevel();
	SaveGame->PotionRecoveryUpgradeLevel = InventoryComponent->GetPotionRecoveryUpgradeLevel();
	
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
		UE_LOG(LogTemp, Warning, TEXT("%s : 저장된 게임을 불러올 수 없음"), *GetNameSafe(this));
	}
}

void AT3GameMode::SetCharacterBySavedData(AT3CharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	//GameInstance 유효성 검사
	if (!T3GameInstance)
	{
		T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
		if (!T3GameInstance)
		{
			return;
		}
	}

	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	if (!SaveGame) return;

	// 위치 설정 (타이머를 사용하여 지연 실행)
	// 랜드스케이프가 렌더링/물리 데이터를 준비할 시간을 0.2초 정도 벌어줍니다.
#ifdef IF_WITH_EDITOR
	if (!T3GameInstance->bDoNotMoveCharacterBySavedData && SaveGame->bSetLocation)
#else
	if (SaveGame->bSetLocation)
#endif
	{
		SaveGame->bSetLocation = false; // 플래그 초기화

		FVector TargetLocation = SaveGame->PlayerLocation;

		FTimerHandle LocationTimerHandle;
		// [람다 캡처] Character와 TargetLocation 등을 안전하게 전달합니다.
		GetWorldTimerManager().SetTimer(LocationTimerHandle, [Character, TargetLocation]()
			{
				if (Character && Character->IsValidLowLevel())
				{
					// ETeleportType::TeleportPhysics를 사용하여 물리 엔진에 순간이동임을 알립니다.
					Character->SetActorLocation(TargetLocation);

					UE_LOG(LogTemp, Log, TEXT("Delayed Location Set Success for: %s"), *Character->GetName());
				}
			}, 2.0f, false);
	}

	//스탯 적용 (스탯은 즉시 적용해도 안전합니다)
	Character->SetCurrentHP(SaveGame->CurrentHP);
	Character->SetCurrentMana(SaveGame->CurrentMana);
	Character->SetCurrentStamina(SaveGame->CurrentStamina);
	Character->SetAttackPower(SaveGame->AttackPower);
	Character->SetCriticalChance(SaveGame->CriticalChance);
	Character->SetCriticalDamage(SaveGame->CriticalDamage);
	Character->SetMoveSpeed(SaveGame->MoveSpeed);

	//인벤토리 컴포넌트 유효성 검사
	TObjectPtr<UT3InventoryComponent> InventoryComponent = Character->InventoryComponent;
	if (InventoryComponent)
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
		
		//포션
		InventoryComponent->SetHPPotionCount(SaveGame->HPPotionCount);
		InventoryComponent->SetMPPotionCount(SaveGame->MPPotionCount);
		InventoryComponent->LoadPotionUpgradeLevel(SaveGame->PotionAmountUpgradeLevel, SaveGame->PotionRecoveryUpgradeLevel);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryComponent is Null on %s"), *Character->GetName());
	}

	//장비 컴포넌트 유효성 검사
	if (UT3PlayerEquipmentComponent* EquipComp = Character->FindComponentByClass<UT3PlayerEquipmentComponent>())
	{
		EquipComp->LoadEquipmentFromSave(SaveGame->WeaponSaveData, SaveGame->ArmorSaveData);
	}
}