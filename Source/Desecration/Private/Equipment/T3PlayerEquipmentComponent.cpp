// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/T3PlayerEquipmentComponent.h"
#include "Desecration.h"
#include "GameFramework/Character.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMeshActor.h" 

UT3PlayerEquipmentComponent::UT3PlayerEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 포인터 변수는 반드시 nullptr로 초기화해야 안전합니다.
    WeaponTable = nullptr;
    ArmorTable = nullptr;

    WeaponInstance = nullptr;
    ArmorInstance = nullptr;
    SpawnedWeaponActor = nullptr;

    // FName은 기본값이 NAME_None이지만 명시적으로 해주는 게 좋습니다.
    DefaultWeaponID = NAME_None;
    DefaultArmorID = NAME_None;

    CurrentAttackPower = 0.0f;
    CurrentDefensePower = 0.0f;
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

bool UT3PlayerEquipmentComponent::TrySocketRune(UT3TestItemInstance* TargetItem, FName RuneID)
{
    if (!TargetItem || !RuneTable) return false;

    // 소켓 개수 제한 체크
    if (TargetItem->SocketedRuneIDs.Num() >= MaxRuneSockets)
    {
        UE_LOG(LogDesecration, Warning, TEXT("룬 소켓이 가득 찼습니다. (최대: %d)"), MaxRuneSockets);
        return false;
    }

    // 1. 데이터 테이블 조회
    const FT3RuneDataRow* Row = RuneTable->FindRow<FT3RuneDataRow>(RuneID, TEXT("SocketRune"));
    if (!Row || !Row->RuneLogicClass)
    {
        UE_LOG(LogDesecration, Warning, TEXT("룬 데이터가 없거나 로직 클래스가 없습니다."));
        return false;
    }

    // 2. 룬 객체 생성 (NewObject)
    // Outer를 'this(Component)'나 'TargetItem'으로 설정
    UT3RuneLogicBase* NewRune = NewObject<UT3RuneLogicBase>(this, Row->RuneLogicClass);

    // 3. 데이터 주입 (Inject)
    // 테이블에 적힌 공격력/방어력 타입과 수치를 객체에 심어줍니다.
    NewRune->Init(Row->StatType, Row->StatValue);

    // 4. 로직 발동 (OnEquip)
    NewRune->OnEquip(Cast<ACharacter>(GetOwner()));

    // 5. 저장
    TargetItem->ActiveRunes.Add(NewRune);
    TargetItem->SocketedRuneIDs.Add(RuneID);

    // 6. 스탯 재계산
    RefreshStats();

    UE_LOG(LogDesecration, Log, TEXT("룬 장착: %s (Type: %d, Val: %f)"), *RuneID.ToString(), (int32)Row->StatType, Row->StatValue);
    return true;
}

void UT3PlayerEquipmentComponent::UpdateWeaponVisuals()
{
    if (!WeaponInstance || !WeaponTable) return;

    // 1. 통합 테이블에서 데이터 조회
    const FT3WeaponDataRow* WeaponRow = WeaponTable->FindRow<FT3WeaponDataRow>(WeaponInstance->ItemID, TEXT("VisualUpdate"));
    if (!WeaponRow) return;

    // 2. 액터 스폰 및 메쉬 설정
    UWorld* World = GetWorld();
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!World || !OwnerCharacter) return;

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
    SpawnParams.Owner = OwnerCharacter;

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
        SpawnedWeaponActor->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("WeaponSocket"));
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

float UT3PlayerEquipmentComponent::CalculateRuneTotalBonus(const UT3TestItemInstance* Item, ET3RuneStatType StatType) const
{
    if (!Item) return 0.0f;
    
    float Bonus = 0.0f;
    for (UT3RuneLogicBase* Rune : Item->ActiveRunes)
    {
        if (Rune)
        {
            // 룬 객체에게 "너 이 스탯 올려줘?" 하고 물어봄
            Bonus += Rune->GetStatBonus(StatType);
        }
    }
    return Bonus;
}

void UT3PlayerEquipmentComponent::RefreshStats()
{
    // A. 기본 장비 스탯 (Intrinsic)
    float BaseAtk = CalculateWeaponPower(); // 기존 함수 (Base + Growth)
    float BaseDef = CalculateArmorPower();  // 기존 함수

    // B. 룬 보너스 스탯
    float RuneAtk = CalculateRuneTotalBonus(WeaponInstance, ET3RuneStatType::Attack);
    float RuneDef = CalculateRuneTotalBonus(ArmorInstance, ET3RuneStatType::Defense);
    
    // C. 최종 합산 (Final)
    CurrentAttackPower = BaseAtk + RuneAtk;
    CurrentDefensePower = BaseDef + RuneDef;

    UE_LOG(LogDesecration, Log, TEXT("Stats Updated -> Atk: %.1f (Rune+%.1f), Def: %.1f (Rune+%.1f)"),
        CurrentAttackPower, RuneAtk, CurrentDefensePower, RuneDef);

    // 캐릭터팀에 스탯 변경 알림
    OnEquipmentStatsChanged.Broadcast(CurrentAttackPower, CurrentDefensePower);
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
    else // Armor
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
