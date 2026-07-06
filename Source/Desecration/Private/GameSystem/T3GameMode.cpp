#include "GameSystem/T3GameMode.h"

#include "Equipment/T3PlayerEquipmentComponent.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"
#include "Interaction/T3LostMoney.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3PlayerController.h"
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
	
	//이 레벨에서 잃어버린 재화 생성
	MakeLostMoneyActors();
	bSpawnLostMoneyEnd = true;
}

void AT3GameMode::MakeLostMoneyActors()
{
	if (!T3GameInstance->GetSavedGameData())
	{
		return;
	}
	
	for (const TTuple<int32, FLostMoney> LostMoneyInfo : T3GameInstance->GetSavedGameData()->LostMoneyList)
	{
		//이 레벨에 해당되는 것만 생성
		FLostMoney LostMoney = LostMoneyInfo.Value;
		if (LostMoney.LevelName != T3GameInstance->GetCurrentLevel())
		{
			continue;
		}

		if (TObjectPtr<AActor> SpawnedActor = GetWorld()->SpawnActor(LostMoneyClass, &LostMoney.Location))
		{
			TObjectPtr<AT3LostMoney> SpawnedLostMoney = Cast<AT3LostMoney>(SpawnedActor);
			if (!SpawnedLostMoney)
			{
				SpawnedLostMoney->Destroy();
				continue;
			}
			//정상적으로 생성했으면 번호와 재화의 양 지정하기
			SpawnedLostMoney->SetLostMoneyID(LostMoneyInfo.Key);
			SpawnedLostMoney->SetMoney(LostMoney.Money);
		}
	}
}

ECharacterClass AT3GameMode::GetPlayerClass()
{
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance가 NULL"), *GetNameSafe(this));
		return ECharacterClass::Paladin;
	}
	
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	return SaveGame->PlayerClass;
}

bool AT3GameMode::SaveGameV2(const AT3CharacterBase* Character, const ESaveType SaveType, const ELevelName LevelName, const FVector& TargetLocation, const FRotator& TargetRotation)
{
	return SaveGameV2_MultiType(Character, static_cast<uint8>(SaveType), LevelName, TargetLocation, TargetRotation);
}

bool AT3GameMode::SaveGameV2_MultiType(const AT3CharacterBase* Character, const TSet<ESaveType> SaveTypes, const ELevelName LevelName, const FVector& TargetLocation, const FRotator& TargetRotation)
{
	uint8 SumValue = 0;
	for (const ESaveType& SaveType : SaveTypes)
	{
		if (SaveType == ESaveType::All)//전체 저장(0)이 포함되면 나머지가 필요없음
		{
			SumValue = 0;
			break;
		}
		SumValue += static_cast<uint8>(SaveType);
	}
	return SaveGameV2_MultiType(Character, SumValue, LevelName, TargetLocation, TargetRotation);
}

bool AT3GameMode::SaveGameV2_MultiType(const AT3CharacterBase* Character, const uint8 SaveTypes, const ELevelName LevelName, const FVector& TargetLocation, const FRotator& TargetRotation)
{
	//캐릭터 정보를 저장된 게임 데이터에 저장한다.
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	if (!SaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : SaveGame이 NULL"), *GetNameSafe(this));
		return false;
	}
	
	//현재 위치
	if (SaveTypes == 0 || SaveTypes & static_cast<uint8>(ESaveType::Location))
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 로케이션 위치"), *SaveGame->PlayerLocation.ToString());
		if (LevelName >= ELevelName::Tutorial)
		{
			SaveGame->SavedLevelName = LevelName;
		}
		SaveGame->PlayerLocation = TargetLocation;
		SaveGame->PlayerRotation = TargetRotation;
	}
	
	//스탯
	if (SaveTypes == 0 || SaveTypes & static_cast<uint8>(ESaveType::Stat))
	{
		SaveGame->MaxHP = Character->GetMaxHP();
		SaveGame->CurrentHP = Character->GetCurrentHP();
		SaveGame->MaxMana = Character->GetMaxMana();
		SaveGame->CurrentMana = Character->GetCurrentMana();
		SaveGame->MaxStamina = Character->GetMaxStamina();
		SaveGame->CurrentStamina = Character->GetCurrentStamina();
		SaveGame->StatAttackPower = Character->GetStatAttackPower();
		SaveGame->WeaponAttackPower = Character->GetWeaponAttackPower();
		SaveGame->RuneAttackPower = Character->GetRuneAttackPower();
		SaveGame->CriticalChance = Character->GetCriticalChance();
		SaveGame->CriticalDamage = Character->GetCriticalDamage();
		SaveGame->MoveSpeed = Character->GetMoveSpeed();
		SaveGame->Vigor = Character->GetVigor();
		SaveGame->Endurance = Character->GetEndurance();
		SaveGame->Mind = Character->GetMind();
		SaveGame->Strength = Character->GetStrength();
		SaveGame->Intelligence = Character->GetIntelligence();
		SaveGame->WeaponLevel = Character->GetWeaponLevel();
		SaveGame->CharacterLevel = Character->GetCharacterLevel();
	}
	
	//인벤토리
	if (SaveTypes == 0 || SaveTypes & static_cast<uint8>(ESaveType::Inventory))
	{
		const TObjectPtr<UT3InventoryComponent> InventoryComponent = Character->InventoryComponent;
		if (!InventoryComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("%s : InventoryComponent가 null"), *GetNameSafe(this));
			return false;
		}
		//참고 : 인벤토리 공간은 T3InventoryComponent에서 정한 값을 따른다.
		constexpr int32 InvenSize = 20;
		for (int32 iNum = 0; iNum < InvenSize; ++iNum)
		{
			//유효 인덱스 검사
			if (!(SaveGame->Items.IsValidIndex(iNum) && InventoryComponent->Items.IsValidIndex(iNum) &&
				SaveGame->RuneItems.IsValidIndex(iNum) && InventoryComponent->RuneItems.IsValidIndex(iNum) &&
				SaveGame->EtcItems.IsValidIndex(iNum) && InventoryComponent->EtcItems.IsValidIndex(iNum)))
			{
				break;
			}
		
			SaveGame->Items[iNum] = InventoryComponent->Items[iNum];
			SaveGame->RuneItems[iNum] = InventoryComponent->RuneItems[iNum];
			SaveGame->EtcItems[iNum] = InventoryComponent->EtcItems[iNum];
		}
		SaveGame->Money = InventoryComponent->GetMoney();
		SaveGame->WeaponNormalStoneCount = InventoryComponent->GetWeaponNormalStoneCount();
		SaveGame->WeaponEpicStoneCount = InventoryComponent->GetWeaponEpicStoneCount();
		SaveGame->WeaponLegendaryStoneCount = InventoryComponent->GetWeaponLegendaryStoneCount();
		SaveGame->ArmorNormalStoneCount = InventoryComponent->GetArmorNormalStoneCount();
		SaveGame->ArmorEpicStoneCount = InventoryComponent->GetArmorEpicStoneCount();
		SaveGame->ArmorLegendaryStoneCount = InventoryComponent->GetArmorLegendaryStoneCount();
		//포션
		SaveGame->HPPotionMaxCount = InventoryComponent->GetMaxHPPotionCount();
		SaveGame->MPPotionMaxCount = InventoryComponent->GetMaxMPPotionCount();
		SaveGame->PotionAmountUpgradeLevel = InventoryComponent->GetPotionAmountUpgradeLevel();
		SaveGame->PotionRecoveryUpgradeLevel = InventoryComponent->GetPotionRecoveryUpgradeLevel();
	}
	
	//재화만 따로 저장
	if (SaveTypes & static_cast<uint8>(ESaveType::Money))
	{
		if (const TObjectPtr<UT3InventoryComponent> InventoryComponent = Character->InventoryComponent)
		{
			SaveGame->Money = InventoryComponent->GetMoney();
		}
	}
	
	//장비
	if (SaveTypes == 0 || SaveTypes & static_cast<uint8>(ESaveType::Equipment))
	{
		if (const UT3PlayerEquipmentComponent* EquipComp = Character->FindComponentByClass<UT3PlayerEquipmentComponent>())
		{
			EquipComp->GetEquipmentSaveData(SaveGame->WeaponSaveData, SaveGame->ArmorSaveData, SaveGame->ClassSocketedRuneIDs);
			EquipComp->GetAccessorySaveData(SaveGame->AccessorySaveData);
		}
	}
	
	//스킬
	if (SaveTypes == 0 || SaveTypes & static_cast<uint8>(ESaveType::Skill))
	{
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
		
		for (TTuple<int32, bool> UnlockState : SkillComponent->SkillUnlockStates)
		{
			bool& State = SaveGame->SkillUnlockStates.FindOrAdd(UnlockState.Key);
			State = UnlockState.Value;
		}
		SaveGame->EquippedSkillIDs = SkillComponent->EquippedSkillIDs;
	}
	
	return true;
}

bool AT3GameMode::SaveGame(const AT3CharacterBase* Character, const ELevelName LevelName, const bool bTemporarySave, const FVector TargetLocation, const FRotator TargetRotation)
{
	return SaveGameV2_MultiType(Character, 0, LevelName, TargetLocation, TargetRotation);
}

bool AT3GameMode::SaveInventoryAndPotionLevel(const AT3CharacterBase* Character)
{
	//인벤토리 저장
	//기존 코드에는 장비에도 접근해서 장비 저장도 포함
	constexpr uint8 SaveTypes = static_cast<uint8>(ESaveType::Inventory) + static_cast<uint8>(ESaveType::Equipment);
	return SaveGameV2_MultiType(Character, SaveTypes);
}

bool AT3GameMode::SaveOnlySkill(const AT3CharacterBase* Character)
{
	//스킬 저장
    return SaveGameV2(Character, ESaveType::Skill);
}

void AT3GameMode::LoadGame() const
{
	T3GameInstance->OpenLevelBySavedData();
}

void AT3GameMode::SetCharacterBySavedData(AT3CharacterBase* Character)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : Character가 null"), *GetNameSafe(this));
		return;
	}

	//GameInstance 유효성 검사
	if (!T3GameInstance)
	{
		T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
		if (!T3GameInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("%s : GameInstance가 null"), *GetNameSafe(this));
			return;
		}
	}

	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	if (!SaveGame) return;

	// 위치 설정 (타이머를 사용하여 지연 실행)
	// 랜드스케이프가 렌더링/물리 데이터를 준비할 시간을 0.2초 정도 벌어줍니다.
#ifdef WITH_EDITOR
	if (!T3GameInstance->bDoNotMoveCharacterBySavedData)
#else
	if (true)
#endif
	{
		FVector TargetLocation = SaveGame->PlayerLocation;
		FRotator TargetRotation = SaveGame->PlayerRotation;

		FTimerHandle LocationTimerHandle;
		// [람다 캡처] Character와 TargetLocation 등을 안전하게 전달합니다.
		GetWorldTimerManager().SetTimer(LocationTimerHandle, [Character, TargetLocation, TargetRotation]()
			{
				if (Character && Character->IsValidLowLevel())
				{
					UE_LOG(LogTemp, Error, TEXT("이 위치로 텔레포트합니다. : %s"), *TargetLocation.ToString());
					
					// ETeleportType::TeleportPhysics를 사용하여 물리 엔진에 순간이동임을 알립니다.
					Character->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);

					UE_LOG(LogTemp, Log, TEXT("Delayed Location Set Success for: %s"), *Character->GetName());
				}
			}, 2.0f, false);
	}

	//스탯 적용 (스탯은 즉시 적용해도 안전합니다)
	Character->SetMaxHP(SaveGame->MaxHP);
	Character->SetCurrentHP(SaveGame->CurrentHP);
	Character->SetMaxMana(SaveGame->MaxMana);
	Character->SetCurrentMana(SaveGame->CurrentMana);
	Character->SetMaxStamina(SaveGame->MaxStamina);
	Character->SetCurrentStamina(SaveGame->CurrentStamina);
	Character->SetStatAttackPower(SaveGame->StatAttackPower);
	Character->SetAttackPower(SaveGame->WeaponAttackPower);
	Character->SetRuneAttackPower(SaveGame->RuneAttackPower);
	Character->SetCriticalChance(SaveGame->CriticalChance);
	Character->SetCriticalDamage(SaveGame->CriticalDamage);
	Character->SetMoveSpeed(SaveGame->MoveSpeed);
	Character->SetVigor(SaveGame->Vigor);
	Character->SetEndurance(SaveGame->Endurance);
	Character->SetMind(SaveGame->Mind);
	Character->SetStrength(SaveGame->Strength);
	Character->SetIntelligence(SaveGame->Intelligence);
	Character->SetWeaponLevel(SaveGame->WeaponLevel);
	Character->SetCharacterLevel(SaveGame->CharacterLevel);

	//인벤토리 컴포넌트 유효성 검사
	TObjectPtr<UT3InventoryComponent> InventoryComponent = Character->InventoryComponent;
	if (InventoryComponent)
	{
		constexpr int32 InvenSize = 20;
		for (int32 iNum = 0; iNum < InvenSize; ++iNum)
		{
			//유효 인덱스 검사
			if (!(SaveGame->Items.IsValidIndex(iNum) && InventoryComponent->Items.IsValidIndex(iNum) &&
				SaveGame->RuneItems.IsValidIndex(iNum) && InventoryComponent->RuneItems.IsValidIndex(iNum) &&
				SaveGame->EtcItems.IsValidIndex(iNum) && InventoryComponent->EtcItems.IsValidIndex(iNum)))
			{
				break;
			}
			InventoryComponent->Items[iNum] = SaveGame->Items[iNum];
			InventoryComponent->RuneItems[iNum] = SaveGame->RuneItems[iNum];
			InventoryComponent->EtcItems[iNum] = SaveGame->EtcItems[iNum];
		}
		InventoryComponent->SetMoney(SaveGame->Money);
		InventoryComponent->SetWeaponNormalStoneCount(SaveGame->WeaponNormalStoneCount);
		InventoryComponent->SetWeaponEpicStoneCount(SaveGame->WeaponEpicStoneCount);
		InventoryComponent->SetWeaponLegendaryStoneCount(SaveGame->WeaponLegendaryStoneCount);
		InventoryComponent->SetArmorNormalStoneCount(SaveGame->ArmorNormalStoneCount);
		InventoryComponent->SetArmorEpicStoneCount(SaveGame->ArmorEpicStoneCount);
		InventoryComponent->SetArmorLegendaryStoneCount(SaveGame->ArmorLegendaryStoneCount);
		
		//포션
		InventoryComponent->LoadPotionUpgradeLevel(SaveGame->PotionAmountUpgradeLevel, SaveGame->PotionRecoveryUpgradeLevel);
		InventoryComponent->SetHPPotionCount(SaveGame->HPPotionMaxCount);
		InventoryComponent->SetMPPotionCount(SaveGame->MPPotionMaxCount);

		//인벤토리 갱신
		InventoryComponent->OnInventoryInitialized.Broadcast();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryComponent is Null on %s"), *Character->GetName());
	}

	//장비 컴포넌트
	if (UT3PlayerEquipmentComponent* EquipComp = Character->FindComponentByClass<UT3PlayerEquipmentComponent>())
	{
		EquipComp->LoadEquipmentFromSave(SaveGame->WeaponSaveData, SaveGame->ArmorSaveData, SaveGame->ClassSocketedRuneIDs);
		EquipComp->LoadAccessoryFromSave(SaveGame->AccessorySaveData);
	}
	
	//스킬
	TObjectPtr<UT3SkillComponentBase> SkillComponent;
	if (const TObjectPtr<UT3CombatComponent> CombatComponent = Character->GetCombatComponent(); !CombatComponent || !CombatComponent->GetSkillComponent())
	{
		UE_LOG(LogTemp, Error, TEXT("%s : SkillComponent 접근 불가"), *GetNameSafe(this));
		return;
	}
	else
	{
		SkillComponent = CombatComponent->GetSkillComponent();
	}
	for (TTuple<int32, bool> SavedState : SaveGame->SkillUnlockStates)
	{
		SkillComponent->SetSkillUnlockState(SavedState.Key, SavedState.Value);
	}
	// 장착된 스킬 배열을 순서대로 복원 (최대 4개, 순서 유지)
	for (int32 SkillID : SaveGame->EquippedSkillIDs)
	{
		if (SkillID != 0)
		{
			SkillComponent->SetSkillSlot(SkillID, true);
		}
	}
}

void AT3GameMode::RegainLostMoney(const int32 LostMoneyID) const
{
	if (!T3GameInstance->GetSavedGameData())
	{
		UE_LOG(LogTemp, Error, TEXT("%s : 잃어버린 재화 갱신 실패"), *GetNameSafe(this));
		return;
	}
	T3GameInstance->GetSavedGameData()->RegainLostMoney(LostMoneyID);
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
	
	//잃어버린 재화 내용을 저장 데이터에 반영
	T3GameInstance->GetSavedGameData()->Money = 0;
	
	//잃어버린 재화 정보
	const int32 LostAmount = Character->InventoryComponent->GetMoney();
	const FLostMoney NewLostMoney = FLostMoney(T3GameInstance->GetCurrentLevel(), Character->GetActorLocation(), LostAmount);
	T3GameInstance->GetSavedGameData()->AddLostMoney(NewLostMoney);

	//마지막 저장 위치로
	LoadGame();
	
	return true;
}

void AT3GameMode::InstantSave()
{
#if WITH_EDITOR
	if (!T3GameInstance)
	{
		return;
	}
	
	TObjectPtr<AT3PlayerController> T3Controller = Cast<AT3PlayerController>(GetWorld()->GetFirstPlayerController());
	if (!T3Controller)
	{
		return;
	}
	
	TObjectPtr<AT3CharacterBase> Character = Cast<AT3CharacterBase>(T3Controller->GetPawn());
	if (!Character)
	{
		return;
	}
	
#else
	return;
#endif
}