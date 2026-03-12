#include "GameSystem/T3GameMode.h"

#include "Equipment/T3PlayerEquipmentComponent.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"
#include "GameSystem/T3WorldSubsystem.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3SkillComponentBase.h"

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
	if (const UT3PlayerEquipmentComponent* EquipComp = Character->FindComponentByClass<UT3PlayerEquipmentComponent>())
	{
		SaveGame->AttackPower = EquipComp->GetCurrentAttackPower();
		EquipComp->GetEquipmentSaveData(SaveGame->WeaponSaveData, SaveGame->ArmorSaveData);
	}
	
	//스킬
	TObjectPtr<UT3SkillComponentBase> SkillComponent;
	if (const TObjectPtr<UT3CombatComponent> CombatComponent = Character->GetCombatComponent(); !CombatComponent || !CombatComponent->GetSkillComponent())
	{
		UE_LOG(LogTemp, Error, TEXT("%s : SkillComponent 접근 불가"), *GetNameSafe(this));
		return false;
	}
	else
	{
		SkillComponent = CombatComponent->GetSkillComponent();
	}
	//SkillComponent->;
	
	//저장했던 적 상태 제거
	SaveGame->EnemyStates.Empty();
	
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

void AT3GameMode::RegainLostMoney(const int32 LostMoneyID) const
{
	T3GameInstance->GetLostMoneyData()->RegainLostMoney(LostMoneyID);
	if (!T3GameInstance->SaveLostMoney())
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 잃어버린 재화 갱신 실패"), *GetNameSafe(this));
		return;
	}
}

bool AT3GameMode::YouHaveBeenCorrupted(const AT3CharacterBase* Character) const
{
	if (!Character || !Character->InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 게임 오버 처리 실패 - 캐릭터 또는 인벤토리가 유효하지 않음"), *GetNameSafe(this));
		return false;
	}
	
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 게임 오버 처리 실패 - T3GameInstance가 없음"), *GetNameSafe(this));
		return false;
	}
	
	//잃어버린 재화 내용을 마지막 저장 데이터에 반영
	if (T3GameInstance->LoadGame())
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 게임 오버 처리 실패 - 저장된 게임 데이터 없음"), *GetNameSafe(this));
		return false;
	}
	T3GameInstance->GetSavedGameData()->Money = 0;
	
	//잃어버린 재화 정보
	const int32 LostAmount = Character->InventoryComponent->GetMoney();
	const FLostMoney NewLostMoney = FLostMoney(T3GameInstance->GetCurrentLevel(), Character->GetActorLocation(), LostAmount);
	
	//잃어버린 것을 반영하기 위한 저장
	T3GameInstance->GetLostMoneyData()->AddLostMoney(NewLostMoney);
	const bool SaveGameResult = T3GameInstance->SaveGame();
	const bool SaveLostMoneyResult = T3GameInstance->SaveLostMoney();
	if (!SaveGameResult || !SaveLostMoneyResult)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 게임 오버 처리 실패 - 저장 실패"), *GetNameSafe(this));
		return false;
	}
	
	//마지막 저장 위치로
	T3GameInstance->OpenLevelBySavedData();
	
	return true;
}
