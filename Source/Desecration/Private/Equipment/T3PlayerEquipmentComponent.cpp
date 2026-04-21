// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/T3PlayerEquipmentComponent.h"
#include "Desecration.h"
#include "GameFramework/Character.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMeshActor.h"
#include "Equipment/Accessory/T3AccessoryEffectBase.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Item/Data/T3RuneItemData.h"
#include "Player/T3CharacterBase.h"

UT3PlayerEquipmentComponent::UT3PlayerEquipmentComponent()
    : WeaponTable(nullptr)
    , ArmorTable(nullptr)
	, MaxRuneSockets(1)
    , DefaultWeaponID("BaseWeapon")
    , DefaultArmorID("BaseArmor")
    , WeaponInstance(nullptr)
    , ArmorInstance(nullptr)
    , SpawnedWeaponActor(nullptr)
    , CurrentAttackPower(0.0f)
    , CurrentDefensePower(0.0f)
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UT3PlayerEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    // (나중에는 세이브 파일에서 로드하거나 인벤토리에서 가져오는 로직으로 대체됨)
    // 1. 기본 무기 생성 및 장착
    if (DefaultWeaponID != NAME_None)
    {
        UT3TestItemInstance* NewWeapon = NewObject<UT3TestItemInstance>(this);
        NewWeapon->Init(DefaultWeaponID, 0, ET3EquipmentType::Weapon);
        EquipWeapon(NewWeapon);
    }

    // 2. 기본 방어구 생성 및 장착
    if (DefaultArmorID != NAME_None)
    {
        UT3TestItemInstance* NewArmor = NewObject<UT3TestItemInstance>(this);
        NewArmor->Init(DefaultArmorID, 0, ET3EquipmentType::Armor); // 0강으로 시작
        EquipArmor(NewArmor); // 방어구 장착 함수 호출!
    }
	
	OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
}

void UT3PlayerEquipmentComponent::EquipWeapon(UT3TestItemInstance* NewItem)
{
    if (!NewItem) return;

    // 1. 슬롯에 아이템 등록
    WeaponInstance = NewItem;

    // 2. 비주얼 갱신 (캐릭터팀에서 처리 예정)
    // UpdateWeaponVisuals();
    RefreshStats();

    // 로그 출력
    UE_LOG(LogDesecration, Log, TEXT("Equipped Weapon: %s (Lv.%d), Power: %f"),
        *NewItem->ItemID.ToString(),
        NewItem->CurrentLevel,
        GetCurrentAttackPower());
}

void UT3PlayerEquipmentComponent::EquipArmor(UT3TestItemInstance* NewItem)
{
    if (!NewItem) return;

    // 1. 방어구 인스턴스 교체
    ArmorInstance = NewItem;

    // 2. 상태 갱신 (캐릭터팀에서 비주얼 처리 예정)
    // UpdateArmorVisuals();

    RefreshStats();

    // 로그 확인
    UE_LOG(LogDesecration, Log, TEXT("방어구 장착 완료: %s (Lv.%d) - 방어력: %f"),
        *NewItem->ItemID.ToString(),
        NewItem->CurrentLevel,
        GetCurrentDefensePower());
}

// ============================================================================
// 세이브/로드 (세이브팀에서 호출)
// ============================================================================

void UT3PlayerEquipmentComponent::LoadEquipmentFromSave(const FT3ItemSaveData& WeaponData, const FT3ItemSaveData& ArmorData, const TArray<FName>& InClassRuneIDs)
{
	if (WeaponData.ItemID != NAME_None)
	{
		UT3TestItemInstance* LoadedWeapon = NewObject<UT3TestItemInstance>(this);
		LoadedWeapon->Init(WeaponData.ItemID, WeaponData.Level, ET3EquipmentType::Weapon);
		EquipWeapon(LoadedWeapon);
	}

	if (ArmorData.ItemID != NAME_None)
	{
		UT3TestItemInstance* LoadedArmor = NewObject<UT3TestItemInstance>(this);
		LoadedArmor->Init(ArmorData.ItemID, ArmorData.Level, ET3EquipmentType::Armor);
		EquipArmor(LoadedArmor);
	}

	WeaponSocketedRuneIDs = WeaponData.SocketedRuneIDs;
	ArmorSocketedRuneIDs = ArmorData.SocketedRuneIDs;
	ClassSocketedRuneIDs = InClassRuneIDs;

	RestoreRunes(WeaponSocketedRuneIDs, WeaponActiveRunes);
	RestoreRunes(ArmorSocketedRuneIDs, ArmorActiveRunes);
	RestoreRunes(ClassSocketedRuneIDs, ClassActiveRunes);

	OnRuneSocketChanged.Broadcast();
}

void UT3PlayerEquipmentComponent::GetEquipmentSaveData(FT3ItemSaveData& OutWeaponData, FT3ItemSaveData& OutArmorData, TArray<FName>& OutClassRuneIDs) const
{
	if (WeaponInstance)
	{
		OutWeaponData.ItemID = WeaponInstance->ItemID;
		OutWeaponData.Level = WeaponInstance->CurrentLevel;
		OutWeaponData.Type = ET3EquipmentType::Weapon;
	}
	else
	{
		OutWeaponData.ItemID = NAME_None;
		OutWeaponData.Level = 0;
		OutWeaponData.Type = ET3EquipmentType::Weapon;
	}

	if (ArmorInstance)
	{
		OutArmorData.ItemID = ArmorInstance->ItemID;
		OutArmorData.Level = ArmorInstance->CurrentLevel;
		OutArmorData.Type = ET3EquipmentType::Armor;
	}
	else
	{
		OutArmorData.ItemID = NAME_None;
		OutArmorData.Level = 0;
		OutArmorData.Type = ET3EquipmentType::Armor;
	}

	OutWeaponData.SocketedRuneIDs = WeaponSocketedRuneIDs;
	OutArmorData.SocketedRuneIDs = ArmorSocketedRuneIDs;
	OutClassRuneIDs = ClassSocketedRuneIDs;
}

void UT3PlayerEquipmentComponent::UpdateWeaponVisuals()
{
    if (!WeaponInstance || !WeaponTable) return;

    // 1. 통합 테이블에서 데이터 조회
    const FT3WeaponDataRow* WeaponRow = WeaponTable->FindRow<FT3WeaponDataRow>(WeaponInstance->ItemID, TEXT("VisualUpdate"));
    if (!WeaponRow) return;

    // 2. 액터 스폰 및 메쉬 설정
    UWorld* World = GetWorld();
    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!World || !OwnerChar) return;

    // 기존 무기 제거
    if (SpawnedWeaponActor)
    {
        SpawnedWeaponActor->Destroy();
        SpawnedWeaponActor = nullptr;
    }

    // 무기 메쉬 로딩 (SoftPtr -> HardPtr)
    UStaticMesh* LoadedMesh = WeaponRow->Mesh.LoadSynchronous();
    if (!LoadedMesh) return;

    // 3. 이펙트 클래스 결정
    UClass* ActorClassToSpawn = AStaticMeshActor::StaticClass(); // 기본값

    if (WeaponInstance->CurrentLevel > 0)
    {
        int32 ArrayIndex = WeaponInstance->CurrentLevel - 1;

        // 해당 레벨 데이터가 있고, 이펙트가 설정되어 있다면
        if (WeaponRow->LevelStats.IsValidIndex(ArrayIndex))
        {
            const FT3WeaponLevelData& LevelData = WeaponRow->LevelStats[ArrayIndex];
            if (!LevelData.VisualEffectClass.IsNull())
            {
                ActorClassToSpawn = LevelData.VisualEffectClass.LoadSynchronous();
            }
        }
    }

    // 4. 스폰
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerChar;

    SpawnedWeaponActor = World->SpawnActor<AActor>(ActorClassToSpawn, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

    if (SpawnedWeaponActor)
    {
        // AStaticMeshActor인 경우 무브빌리티를 'Movable'로 변경해야 함
        if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(SpawnedWeaponActor))
        {
            MeshActor->SetMobility(EComponentMobility::Movable);
            MeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            MeshActor->GetStaticMeshComponent()->SetStaticMesh(LoadedMesh);
            MeshActor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("NoCollision"));
        }

        // 캐릭터 손 소켓에 부착
        SpawnedWeaponActor->AttachToComponent(OwnerChar->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("WeaponSocket"));
    }
}

void UT3PlayerEquipmentComponent::UpdateArmorVisuals()
{
    // 데이터 테이블이 없거나 장착된 게 없으면 패스
    if (!ArmorInstance || !ArmorTable) return;

    // 1. 통합 테이블에서 방어구 데이터 찾기
    const FT3ArmorDataRow* ArmorRow = ArmorTable->FindRow<FT3ArmorDataRow>(ArmorInstance->ItemID, TEXT("ArmorVisual"));
    if (!ArmorRow) return;

    // 2. 비주얼 적용 로직
    // 무기는 Actor를 스폰했지만, 방어구는 보통 캐릭터의 Mesh를 교체하거나 Material을 바꿉니다.
    // 아직 구체적인 방어구 비주얼 방식(통짜 메쉬 교체 vs 파츠별 교체)이 정해지지 않았으므로,
    // 우선은 데이터가 잘 로드되는지만 로그로 확인하고 넘어갑니다.

    // 예시: 나중에 이런 코드가 들어갑니다.
    /*
    USkeletalMesh* ArmorMesh = ArmorRow->Mesh.LoadSynchronous();
    if (ArmorMesh)
    {
        ACharacter* Character = Cast<ACharacter>(GetOwner());
        Character->GetMesh()->SetSkeletalMesh(ArmorMesh);
    }
    */

    UE_LOG(LogDesecration, Log, TEXT("[Visual] 방어구 외형 정보를 갱신했습니다."));
}

float UT3PlayerEquipmentComponent::CalculateWeaponPower() const
{
    if (!WeaponInstance || !WeaponTable) return 0.0f;

    const FT3WeaponDataRow* WeaponRow = WeaponTable->FindRow<FT3WeaponDataRow>(WeaponInstance->ItemID, TEXT("Calc"));
    if (!WeaponRow) return 0.0f;

    // 0강이면 Base 공격력 반환
    if (WeaponInstance->CurrentLevel == 0) return WeaponRow->BaseAttackPower;

    // 강화 레벨이 있으면 해당 레벨의 고정 공격력 반환
    int32 Idx = WeaponInstance->CurrentLevel - 1;
    if (WeaponRow->LevelStats.IsValidIndex(Idx))
    {
        return WeaponRow->LevelStats[Idx].FixedAttackPower;
    }

    return WeaponRow->BaseAttackPower;
}

float UT3PlayerEquipmentComponent::CalculateArmorPower() const
{
    if (!ArmorInstance || !ArmorTable) return 0.0f;

    const FT3ArmorDataRow* ArmorRow = ArmorTable->FindRow<FT3ArmorDataRow>(ArmorInstance->ItemID, TEXT("Calc"));
    if (!ArmorRow) return 0.0f;

    // 0강이면 Base 방어력 반환
    if (ArmorInstance->CurrentLevel == 0) return ArmorRow->BaseDefensePower;

    // 강화 레벨이 있으면 해당 레벨의 고정 방어력 반환
    int32 Idx = ArmorInstance->CurrentLevel - 1;
    if (ArmorRow->LevelStats.IsValidIndex(Idx))
    {
        return ArmorRow->LevelStats[Idx].FixedDefensePower;
    }

    return ArmorRow->BaseDefensePower;
}

void UT3PlayerEquipmentComponent::RefreshStats()
{
    CurrentAttackPower = CalculateWeaponPower();
    CurrentDefensePower = CalculateArmorPower();

    OnEquipmentStatsChanged.Broadcast(CurrentAttackPower, CurrentDefensePower, WeaponInstance->CurrentLevel);
}

bool UT3PlayerEquipmentComponent::TryUpgrade(ET3EquipmentType TargetType, int32 MaxAllowedLevel)
{
    // 1. 대상 식별
    UT3TestItemInstance* TargetItem = (TargetType == ET3EquipmentType::Weapon) ? WeaponInstance : ArmorInstance;

    if (!TargetItem)
    {
        UE_LOG(LogDesecration, Warning, TEXT("강화할 아이템이 없습니다."));
        return false;
    }

    // 2. 레벨 제한 확인
    int32 NextLevel = TargetItem->CurrentLevel + 1;
    if (NextLevel > MaxAllowedLevel) return false;

    // 3. 데이터 테이블 확인
    bool bCanUpgrade = false;

    if (TargetType == ET3EquipmentType::Weapon)
    {
        if (WeaponTable)
        {
            const FT3WeaponDataRow* Row = WeaponTable->FindRow<FT3WeaponDataRow>(TargetItem->ItemID, TEXT("Upgrade"));
            if (Row && Row->LevelStats.IsValidIndex(NextLevel - 1))
            {
                bCanUpgrade = true;
            }
        }
    }
    else if (TargetType == ET3EquipmentType::Armor)
    {
        if (ArmorTable)
        {
            const FT3ArmorDataRow* Row = ArmorTable->FindRow<FT3ArmorDataRow>(TargetItem->ItemID, TEXT("Upgrade"));
            if (Row && Row->LevelStats.IsValidIndex(NextLevel - 1))
            {
                bCanUpgrade = true;
            }
        }
    }

    if (!bCanUpgrade)
    {
        UE_LOG(LogDesecration, Warning, TEXT("최고 레벨이거나 성장 데이터를 찾을 수 없습니다."));
        return false;
    }

    // 4. 적용
    TargetItem->CurrentLevel = NextLevel;

    // 5. 비주얼 갱신 (캐릭터팀에서 처리 예정)
    // if (TargetType == ET3EquipmentType::Weapon)
    // {
    //     UpdateWeaponVisuals();
    // }
    // else
    // {
    //     UpdateArmorVisuals();
    // }

    RefreshStats();

    UE_LOG(LogDesecration, Log, TEXT("장비 강화 성공! %s Lv.%d"),
        TargetType == ET3EquipmentType::Weapon ? TEXT("무기") : TEXT("방어구"),
        NextLevel);

    return true;
}

bool UT3PlayerEquipmentComponent::GetSocketedRuneData(ET3EquipmentType EquipmentType, int32 SlotIndex,
	FT3RuneItemData& OutRuneData) const
{
	const TArray<FName>& SocketedIDs =
		(EquipmentType == ET3EquipmentType::Weapon) ? WeaponSocketedRuneIDs :
		(EquipmentType == ET3EquipmentType::Armor)  ? ArmorSocketedRuneIDs  : ClassSocketedRuneIDs;

	if (!SocketedIDs.IsValidIndex(SlotIndex))
	{
		return false;
	}

	UT3InventoryComponent* Inventory = GetOwner()->FindComponentByClass<UT3InventoryComponent>();

	if (!Inventory || !Inventory->RuneTable)
	{
		return false;
	}

	const FT3RuneItemData* Row = Inventory->RuneTable->FindRow<FT3RuneItemData>(SocketedIDs[SlotIndex], TEXT("GetSocketedRuneData"));

	if (!Row)
	{
		return false;
	}

	OutRuneData = *Row;
	return true;
}

bool UT3PlayerEquipmentComponent::SocketRune(FName RuneID, ET3EquipmentType TargetEquipment)
{
	if (RuneID == NAME_None)
	{
		UE_LOG(LogTemp, Error, TEXT("룬 이름이 유효하지 않음"))
		return false;
	}

	UT3InventoryComponent* Inventory = GetOwner()->FindComponentByClass<UT3InventoryComponent>();

	if (!Inventory || !Inventory->RuneTable || Inventory->GetRuneItemCountByRuneID(RuneID) <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("인벤토리가 유효하지 않거나 룬 테이블이 없거나 룬이 없음"))
		return false;
	}

	TArray<FName>& SocketedIDs =
		(TargetEquipment == ET3EquipmentType::Weapon) ? WeaponSocketedRuneIDs :
		(TargetEquipment == ET3EquipmentType::Armor)  ? ArmorSocketedRuneIDs  : ClassSocketedRuneIDs;
	
	TArray<TObjectPtr<UT3RuneBase>>& ActiveRunes =
		(TargetEquipment == ET3EquipmentType::Weapon) ? WeaponActiveRunes :
		(TargetEquipment == ET3EquipmentType::Armor)  ? ArmorActiveRunes  : ClassActiveRunes;

	if (SocketedIDs.Num() >= MaxRuneSockets)
	{
		UE_LOG(LogTemp, Error, TEXT("룬이 이미 최대 개수 장착됨"))
		return false;
	}

	if (SocketedIDs.Contains(RuneID))
	{
		UE_LOG(LogTemp, Error, TEXT("같은 룬이 중복 장착 됨."))
		return false;
	}

	const FT3RuneItemData* RuneRow = Inventory->RuneTable->FindRow<FT3RuneItemData>(RuneID, TEXT("SocketRune"));

	if (!RuneRow || !RuneRow->RuneLogicClass)
	{
		UE_LOG(LogTemp, Error, TEXT("데이터가 이상함"));
		return false;
	}

	if (RuneRow->EquipmentType != TargetEquipment)
	{
		UE_LOG(LogTemp, Warning, TEXT("룬 타입이 장비 슬롯과 맞지 않음"));
		return false;
	}
	
	if (TargetEquipment == ET3EquipmentType::ClassSpecific && RuneRow->RequiredClass != ECharacterClass::None
	&& OwnerCharacter->GetCurrentClass() != RuneRow->RequiredClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("직업 전용 룬: 직업이 맞지 않음"));
		return false;
	}
	
	Inventory->RemoveRuneItemByCount(RuneID);

	UT3RuneBase* NewRune = NewObject<UT3RuneBase>(this, RuneRow->RuneLogicClass);
	
	NewRune->SetGrade(RuneRow->RuneGrade);
	
	SocketedIDs.Add(RuneID);
	ActiveRunes.Add(NewRune);
	
	NewRune->OnSocketed(OwnerCharacter);

	if (float* EndTime = RuneCooldownEndTimeMap.Find(RuneRow->RuneLogicClass))
	{
		float Remaining = *EndTime - GetWorld()->GetTimeSeconds();
		
		NewRune->RestoreCooldown(Remaining);
		
		RuneCooldownEndTimeMap.Remove(RuneRow->RuneLogicClass);
	}
	
	OnRuneSocketChanged.Broadcast();
	
	return true;
}

bool UT3PlayerEquipmentComponent::UnsocketRune(FName RuneID, ET3EquipmentType TargetEquipment)
{
	TArray<FName>& SocketedIDs =
		(TargetEquipment == ET3EquipmentType::Weapon) ? WeaponSocketedRuneIDs :
		(TargetEquipment == ET3EquipmentType::Armor)  ? ArmorSocketedRuneIDs  : ClassSocketedRuneIDs;

	TArray<TObjectPtr<UT3RuneBase>>& ActiveRunes =
		(TargetEquipment == ET3EquipmentType::Weapon) ? WeaponActiveRunes :
		(TargetEquipment == ET3EquipmentType::Armor)  ? ArmorActiveRunes  : ClassActiveRunes;

	int32 Index = SocketedIDs.Find(RuneID);

	if (Index == INDEX_NONE)
	{
		return false;
	}

	if (!ActiveRunes[Index]) 
	{
		return false;
	}
	
	if (!ActiveRunes[Index]->CanUnsocket())
	{
		return false;
	}
	
	float Remaining = ActiveRunes[Index]->GetCooldownRemaining();
	
	UE_LOG(LogItem, Error, TEXT("룬의 남은 쿨타임 : %.1f"), Remaining);
	
	if (Remaining > 0.f)
	{
		RuneCooldownEndTimeMap.Add(ActiveRunes[Index]->GetClass(), GetWorld()->GetTimeSeconds() + Remaining);
	}
	
	ActiveRunes[Index]->OnUnsocketed(OwnerCharacter);
	
	SocketedIDs.RemoveAt(Index);
	ActiveRunes.RemoveAt(Index);
		
	UT3InventoryComponent* Inventory = GetOwner()->FindComponentByClass<UT3InventoryComponent>();

	if (Inventory)
	{
		Inventory->AddRuneItemByCount(RuneID, 1);
	}

	OnRuneSocketChanged.Broadcast();
	
	return true;
}

bool UT3PlayerEquipmentComponent::SocketRuneAuto(FName RuneID)
{
	UT3InventoryComponent* Inventory = GetOwner()->FindComponentByClass<UT3InventoryComponent>();

	if (!Inventory || !Inventory->RuneTable)
	{
		return false;
	}

	const FT3RuneItemData* RuneRow = Inventory->RuneTable->FindRow<FT3RuneItemData>(RuneID, TEXT("SocketRuneAuto"));

	if (!RuneRow)
	{
		return false;
	}

	return SocketRune(RuneID, RuneRow->EquipmentType);
}

void UT3PlayerEquipmentComponent::ResetRunesForNewRun()
{
	auto ResetArray = [](TArray<TObjectPtr<UT3RuneBase>>& Runes)
	{
		for (TObjectPtr<UT3RuneBase>& Rune : Runes)
		{
			if (Rune)
			{
				Rune->ResetCooldown();
			}
		}
	};

	ResetArray(WeaponActiveRunes);
	ResetArray(ArmorActiveRunes);
	ResetArray(ClassActiveRunes);
}

void UT3PlayerEquipmentComponent::RestoreRunes(const TArray<FName>& RuneIDs, TArray<TObjectPtr<UT3RuneBase>>& OutActiveRunes)
{
	OutActiveRunes.Empty();

	UT3InventoryComponent* Inventory = GetOwner()->FindComponentByClass<UT3InventoryComponent>();

	if (!Inventory || !Inventory->RuneTable)
	{
		return;
	}

	for (const FName& RuneID : RuneIDs)
	{
		const FT3RuneItemData* RuneRow = Inventory->RuneTable->FindRow<FT3RuneItemData>(RuneID, TEXT("RestoreRunes"));

		if (!RuneRow || !RuneRow->RuneLogicClass)
		{
			OutActiveRunes.Add(nullptr);
			continue;
		}

		UT3RuneBase* NewRune = NewObject<UT3RuneBase>(this, RuneRow->RuneLogicClass);
		NewRune->SetGrade(RuneRow->RuneGrade);
		OutActiveRunes.Add(NewRune);
		NewRune->OnSocketed(OwnerCharacter);
	}
}

void UT3PlayerEquipmentComponent::EquipAccessory(UT3TestItemInstance* NewItem)
{
	if (!IsValid(NewItem))
	{
		UE_LOG(LogTemp, Error, TEXT("EquipAccessory: NewItem이 유효하지 않음"));
		return;
	}

	if (!IsValid(AccessoryTable))
	{
		UE_LOG(LogTemp, Error, TEXT("EquipAccessory: AccessoryTable이 할당되지 않음"));
		return;
	}

	if (IsValid(AccessoryInstance))
	{
		UnequipAccessory();
	}

	AccessoryInstance = NewItem;

	FT3AccessoryDataRow* ItemRow =
		AccessoryTable->FindRow<FT3AccessoryDataRow>(AccessoryInstance->ItemID, TEXT("EquipAccessory"));
	
	if (!ItemRow)
	{
		AccessoryInstance = nullptr;
		return;
	}
	
	ApplyAccessoryStatPointBonus(ItemRow, AccessoryInstance->CurrentLevel, false);
	
	if (IsValid(ItemRow->EffectClass))
	{
		ActiveAccessoryEffect = NewObject<UT3AccessoryEffectBase>(this, ItemRow->EffectClass);
		ActiveAccessoryEffect->OnEquipped(OwnerCharacter);
	}
}

void UT3PlayerEquipmentComponent::UnequipAccessory()
{
	if (!IsValid(AccessoryInstance))
	{
		return;
	}
	
	FT3AccessoryDataRow* ItemRow =
		AccessoryTable->FindRow<FT3AccessoryDataRow>(AccessoryInstance->ItemID, TEXT("EquipAccessory"));
	
	if (!ItemRow)
	{
		return;
	}
	
	ApplyAccessoryStatPointBonus(ItemRow, AccessoryInstance->CurrentLevel, true);
	
	if (IsValid(ActiveAccessoryEffect))
	{
		ActiveAccessoryEffect->OnUnequipped(OwnerCharacter);
	}
	
	ActiveAccessoryEffect = nullptr;
	
	AccessoryInstance = nullptr;
}

bool UT3PlayerEquipmentComponent::TryUpgradeAccessory(int32 MaxAllowedLevel)
{
	if (!IsValid(AccessoryInstance))
	{
		return false;
	}
	
	FT3AccessoryDataRow* ItemRow =
		AccessoryTable->FindRow<FT3AccessoryDataRow>(AccessoryInstance->ItemID, TEXT("EquipAccessory"));
	
	if (!ItemRow)
	{
		return false;
	}
	
	if (ItemRow->LevelStats.Num() == 0)
	{
		return false;
	}
	
	if (AccessoryInstance->CurrentLevel >= ItemRow->LevelStats.Num())
	{
		return false;
	}
	
	if ((AccessoryInstance->CurrentLevel + 1) > MaxAllowedLevel)
	{
		return false;
	}
	
	UT3TestItemInstance* TempInstance = AccessoryInstance;

	UnequipAccessory();
	
	TempInstance->CurrentLevel++;

	EquipAccessory(TempInstance);

	return true;
}

void UT3PlayerEquipmentComponent::LoadAccessoryFromSave(const FT3AccessorySaveData& AccessoryData)
{
	if (AccessoryData.ItemID == NAME_None)
	{
		return;
	}

	UT3TestItemInstance* NewInstance = NewObject<UT3TestItemInstance>(this);
	NewInstance->Init(AccessoryData.ItemID, AccessoryData.Level, ET3EquipmentType::Accessory);

	EquipAccessory(NewInstance);
}

void UT3PlayerEquipmentComponent::GetAccessorySaveData(FT3AccessorySaveData& OutData) const
{
	if (!IsValid(AccessoryInstance))
	{
		OutData.ItemID = NAME_None;
		OutData.Level = 0;
		return;
	}

	OutData.ItemID = AccessoryInstance->ItemID;
	OutData.Level = AccessoryInstance->CurrentLevel;
}

void UT3PlayerEquipmentComponent::SetOniAccessoryEquipped(bool IsEquipped)
{
	bOniAccessoryEquipped = IsEquipped;
}

bool UT3PlayerEquipmentComponent::GetOniAccessoryEquipped() const
{
	return bOniAccessoryEquipped;
}

void UT3PlayerEquipmentComponent::ApplyAccessoryStatPointBonus(const FT3AccessoryDataRow* Row, int32 Level, bool bRemove)
{
	if (!IsValid(OwnerCharacter) || !Row)
	{
		return;
	}

	int32 StatPointBonus = Row->BaseStatBonus;

	if (Level > 0 && Row->LevelStats.IsValidIndex(Level - 1))
	{
		StatPointBonus = Row->LevelStats[Level - 1].StatBonus;
	}
	
	if (bRemove)
	{
		StatPointBonus = -StatPointBonus;
	}

	switch (Row->AccessoryType)
	{
	case ET3AccessoryType::Vigor:
		OwnerCharacter->SetVigor(OwnerCharacter->GetVigor() + StatPointBonus);
		break;

	case ET3AccessoryType::Endurance:
		OwnerCharacter->SetEndurance(OwnerCharacter->GetEndurance() + StatPointBonus);
		break;

	case ET3AccessoryType::Mind:
		OwnerCharacter->SetMind(OwnerCharacter->GetMind() + StatPointBonus);
		break;

	case ET3AccessoryType::Strength:
		OwnerCharacter->SetStrength(OwnerCharacter->GetStrength() + StatPointBonus);
		break;

	case ET3AccessoryType::Intelligence:
		OwnerCharacter->SetIntelligence(OwnerCharacter->GetIntelligence() + StatPointBonus);
		break;

	case ET3AccessoryType::Balrog:
		OwnerCharacter->SetVigor(OwnerCharacter->GetVigor() + StatPointBonus);
		break;
		
	case ET3AccessoryType::Oni:
		OwnerCharacter->SetEndurance(OwnerCharacter->GetEndurance() + StatPointBonus);
		break;
		
	case ET3AccessoryType::FallenAngel:
		OwnerCharacter->SetStrength(OwnerCharacter->GetStrength() + StatPointBonus);
		OwnerCharacter->SetIntelligence(OwnerCharacter->GetIntelligence() + StatPointBonus);
		break;
		
	case ET3AccessoryType::Dragon:
		OwnerCharacter->SetVigor(OwnerCharacter->GetVigor() + StatPointBonus);
		OwnerCharacter->SetEndurance(OwnerCharacter->GetEndurance() + StatPointBonus);
		OwnerCharacter->SetMind(OwnerCharacter->GetMind() + StatPointBonus);
		OwnerCharacter->SetStrength(OwnerCharacter->GetStrength() + StatPointBonus);
		OwnerCharacter->SetIntelligence(OwnerCharacter->GetIntelligence() + StatPointBonus);
		break;
		
	default:
		break;
	}

	// 장착 시에만 캐시 저장
	if (!bRemove)
	{
		CachedAccessoryStatPointBonus = StatPointBonus;
	}
}