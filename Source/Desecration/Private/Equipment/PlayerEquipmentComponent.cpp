// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Equipment/PlayerEquipmentComponent.h"
#include "GameFramework/Character.h"
#include "Engine/AssetManager.h" // 로딩을 위해
#include "Engine/StaticMeshActor.h" 

UPlayerEquipmentComponent::UPlayerEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 포인터 변수는 반드시 nullptr로 초기화해야 안전합니다.
    WeaponBaseTable = nullptr;
    WeaponGrowthTable = nullptr;
    ArmorBaseTable = nullptr;
    ArmorGrowthTable = nullptr;
    
    WeaponInstance = nullptr;
    ArmorInstance = nullptr;
    SpawnedWeaponActor = nullptr;

    // FName은 기본값이 NAME_None이지만 명시적으로 해주는 게 좋습니다.
    DefaultWeaponID = NAME_None;
    DefaultArmorID = NAME_None;

    CurrentAttackPower = 0.0f;
    CurrentDefensePower = 0.0f;
}

void UPlayerEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    // (나중에는 세이브 파일에서 로드하거나 인벤토리에서 가져오는 로직으로 대체됨)
    // 1. 기본 무기 생성 및 장착
    if (DefaultWeaponID != NAME_None)
    {
        UTestItemInstance* NewWeapon = NewObject<UTestItemInstance>(this);
        NewWeapon->Init(DefaultWeaponID, 0, EEquipmentType::Weapon);
        EquipWeapon(NewWeapon);
    }

    // 2. 기본 방어구 생성 및 장착
    if (DefaultArmorID != NAME_None)
    {
        UTestItemInstance* NewArmor = NewObject<UTestItemInstance>(this);
        NewArmor->Init(DefaultArmorID, 0, EEquipmentType::Armor); // 0강으로 시작
        EquipArmor(NewArmor); // 방어구 장착 함수 호출!
    }
}

void UPlayerEquipmentComponent::EquipWeapon(UTestItemInstance* NewItem)
{
    if (!NewItem) return;

    // 1. 슬롯에 아이템 등록
    WeaponInstance = NewItem;

    // 2. 비주얼 갱신
    UpdateWeaponVisuals();
    RefreshStats(); 
    
    // 로그 출력
    UE_LOG(LogTemp, Log, TEXT("Equipped Weapon: %s (Lv.%d), Power: %f"), 
        *NewItem->ItemID.ToString(), 
        NewItem->CurrentLevel, 
        GetCurrentAttackPower());
}

void UPlayerEquipmentComponent::EquipArmor(UTestItemInstance* NewItem)
{
    if (!NewItem) return;

    // 1. 방어구 인스턴스 교체
    ArmorInstance = NewItem;

    // 2. 상태 갱신 (비주얼/스탯)
    UpdateArmorVisuals();
    
    RefreshStats(); 
    
    // 로그 확인
    UE_LOG(LogTemp, Log, TEXT("방어구 장착 완료: %s (Lv.%d) - 방어력: %f"), 
        *NewItem->ItemID.ToString(), 
        NewItem->CurrentLevel,
        GetCurrentDefensePower());
}

bool UPlayerEquipmentComponent::TrySocketRune(UTestItemInstance* TargetItem, FName RuneID)
{
    if (!TargetItem || !RuneTable) return false;

    // (소켓 개수 제한 로직 추가)

    // 1. 데이터 테이블 조회
    const FRuneDataRow* Row = RuneTable->FindRow<FRuneDataRow>(RuneID, TEXT("SocketRune"));
    if (!Row || !Row->RuneLogicClass) 
    {
        UE_LOG(LogTemp, Warning, TEXT("룬 데이터가 없거나 로직 클래스가 없습니다."));
        return false;
    }

    // 2. 룬 객체 생성 (NewObject)
    // Outer를 'this(Component)'나 'TargetItem'으로 설정
    URuneLogicBase* NewRune = NewObject<URuneLogicBase>(this, Row->RuneLogicClass);

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

    UE_LOG(LogTemp, Log, TEXT("룬 장착: %s (Type: %d, Val: %f)"), *RuneID.ToString(), (int32)Row->StatType, Row->StatValue);
    return true;
}

void UPlayerEquipmentComponent::UpdateWeaponVisuals()
{
    if (!WeaponInstance || !WeaponBaseTable || !WeaponGrowthTable) return;

    // -------------------------------------------------------
    // 1. 데이터 조회
    // -------------------------------------------------------
    // (1) 베이스 데이터 찾기
    const FWeaponBaseRow* BaseRow = WeaponBaseTable->FindRow<FWeaponBaseRow>(WeaponInstance->ItemID, TEXT("BaseLookup"));
    if (!BaseRow) return;

    // (2) 성장 데이터 찾기 (RowName을 "0", "1", "2"... 숫자로 관리한다고 가정)
    FName LevelRowName = FName(*FString::FromInt(WeaponInstance->CurrentLevel));

    // -------------------------------------------------------
    // 2. 액터 스폰 및 메쉬 설정
    // -------------------------------------------------------
    UWorld* World = GetWorld();
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!World || !OwnerCharacter) return;

    // 기존 무기 제거
    if (SpawnedWeaponActor)
    {
        SpawnedWeaponActor->Destroy();
        SpawnedWeaponActor = nullptr;
    }

    // A. 무기 메쉬 로딩 (SoftPtr -> HardPtr)
    UStaticMesh* LoadedMesh = BaseRow->Mesh.LoadSynchronous();
    if (!LoadedMesh) return;

    // B. 이펙트 액터 결정 (강화 레벨에 이펙트가 있으면 그걸로, 없으면 기본 액터)
    // 여기서는 간단하게 AStaticMeshActor를 기본으로 쓰고, 이펙트가 있으면 그걸 스폰합니다.
    
    // 5. 이펙트 클래스 결정
    UClass* ActorClassToSpawn = AStaticMeshActor::StaticClass(); // 기본값

    // 성장 테이블에서 내 무기 ID로 검색
    const FWeaponGrowthRow* GrowthRow = WeaponGrowthTable->FindRow<FWeaponGrowthRow>(WeaponInstance->ItemID, TEXT("VisualUpdate"));

    if (GrowthRow && WeaponInstance->CurrentLevel > 0)
    {
        int32 ArrayIndex = WeaponInstance->CurrentLevel - 1;
        
        // 해당 레벨 데이터가 있고, 이펙트가 설정되어 있다면
        if (GrowthRow->LevelStats.IsValidIndex(ArrayIndex))
        {
            const FWeaponLevelData& LevelData = GrowthRow->LevelStats[ArrayIndex];
            if (!LevelData.VisualEffectClass.IsNull())
            {
                ActorClassToSpawn = LevelData.VisualEffectClass.LoadSynchronous();
            }
        }
    }

    // 6. 스폰
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerCharacter;
    
    SpawnedWeaponActor = World->SpawnActor<AActor>(ActorClassToSpawn, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

    if (SpawnedWeaponActor)
    {
        // [수정 포인트] AStaticMeshActor인 경우 무브빌리티를 'Movable'로 변경해야 함
        if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(SpawnedWeaponActor))
        {
            // 1. "이제 너는 움직일 수 있다"고 선언 (이게 없으면 에러 발생)
            MeshActor->SetMobility(EComponentMobility::Movable); 
            MeshActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);

            // 2. 메쉬 및 충돌 설정
            MeshActor->GetStaticMeshComponent()->SetStaticMesh(LoadedMesh);
            MeshActor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("NoCollision"));
        }
        
        // 3. 캐릭터 손 소켓에 부착
        SpawnedWeaponActor->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("WeaponSocket"));
    }
}

void UPlayerEquipmentComponent::UpdateArmorVisuals()
{
    // 데이터 테이블이 없거나 장착된 게 없으면 패스
    if (!ArmorInstance || !ArmorBaseTable) return;

    // 1. 방어구 데이터 찾기 (FArmorBaseRow 사용)
    const FArmorBaseRow* ArmorRow = ArmorBaseTable->FindRow<FArmorBaseRow>(ArmorInstance->ItemID, TEXT("ArmorVisual"));
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
    
    UE_LOG(LogTemp, Log, TEXT("[Visual] 방어구 외형 정보를 갱신했습니다."));
}

float UPlayerEquipmentComponent::CalculateWeaponPower() const
{
    // (기존 GetCurrentAttackPower 내용 그대로 복사)
    if (!WeaponInstance || !WeaponBaseTable || !WeaponGrowthTable) return 0.0f;

    const FWeaponBaseRow* BaseRow = WeaponBaseTable->FindRow<FWeaponBaseRow>(WeaponInstance->ItemID, TEXT("Calc"));
    if (!BaseRow) return 0.0f;

    if (WeaponInstance->CurrentLevel == 0) return BaseRow->BaseAttackPower;

    const FWeaponGrowthRow* GrowthRow = WeaponGrowthTable->FindRow<FWeaponGrowthRow>(WeaponInstance->ItemID, TEXT("Calc"));
    if (GrowthRow && WeaponInstance->CurrentLevel > 0)
    {
        int32 Idx = WeaponInstance->CurrentLevel - 1;
        if (GrowthRow->LevelStats.IsValidIndex(Idx))
        {
            return GrowthRow->LevelStats[Idx].FixedAttackPower;
        }
    }
    return BaseRow->BaseAttackPower;
}

float UPlayerEquipmentComponent::CalculateArmorPower() const
{
    // (기존 GetCurrentDefensePower 내용 그대로 복사)
    if (!ArmorInstance || !ArmorBaseTable || !ArmorGrowthTable) return 0.0f;

    const FArmorBaseRow* BaseRow = ArmorBaseTable->FindRow<FArmorBaseRow>(ArmorInstance->ItemID, TEXT("Calc"));
    if (!BaseRow) return 0.0f;

    if (ArmorInstance->CurrentLevel == 0) return BaseRow->BaseDefensePower;

    const FArmorGrowthRow* GrowthRow = ArmorGrowthTable->FindRow<FArmorGrowthRow>(ArmorInstance->ItemID, TEXT("Calc"));
    if (GrowthRow && ArmorInstance->CurrentLevel > 0)
    {
        int32 Idx = ArmorInstance->CurrentLevel - 1;
        if (GrowthRow->LevelStats.IsValidIndex(Idx))
        {
            return GrowthRow->LevelStats[Idx].FixedDefensePower;
        }
    }
    return BaseRow->BaseDefensePower;
}

float UPlayerEquipmentComponent::CalculateRuneTotalBonus(const UTestItemInstance* Item, ERuneStatType StatType) const
{
    if (!Item) return 0.0f;
    
    float Bonus = 0.0f;
    for (URuneLogicBase* Rune : Item->ActiveRunes)
    {
        if (Rune)
        {
            // 룬 객체에게 "너 이 스탯 올려줘?" 하고 물어봄
            Bonus += Rune->GetStatBonus(StatType);
        }
    }
    return Bonus;
}

void UPlayerEquipmentComponent::RefreshStats()
{
    // A. 기본 장비 스탯 (Intrinsic)
    float BaseAtk = CalculateWeaponPower(); // 기존 함수 (Base + Growth)
    float BaseDef = CalculateArmorPower();  // 기존 함수

    // B. 룬 보너스 스탯
    float RuneAtk = CalculateRuneTotalBonus(WeaponInstance, ERuneStatType::Attack);
    float RuneDef = CalculateRuneTotalBonus(ArmorInstance, ERuneStatType::Defense);
    
    // C. 최종 합산 (Final)
    CurrentAttackPower = BaseAtk + RuneAtk;
    CurrentDefensePower = BaseDef + RuneDef;

    UE_LOG(LogTemp, Log, TEXT("Stats Updated -> Atk: %.1f (Rune+%.1f), Def: %.1f (Rune+%.1f)"), 
        CurrentAttackPower, RuneAtk, CurrentDefensePower, RuneDef);
}

bool UPlayerEquipmentComponent::TryUpgrade(EEquipmentType TargetType, int32 MaxAllowedLevel)
{
    // 1. 대상 식별
    UTestItemInstance* TargetItem = (TargetType == EEquipmentType::Weapon) ? WeaponInstance : ArmorInstance;
    
    if (!TargetItem)
    {
        UE_LOG(LogTemp, Warning, TEXT("강화할 아이템이 없습니다."));
        return false;
    }

    // 2. 레벨 제한 확인
    int32 NextLevel = TargetItem->CurrentLevel + 1;
    if (NextLevel > MaxAllowedLevel) return false;

    // 3. 데이터 테이블 확인 (구조체가 다르므로 분기 필요!)
    bool bCanUpgrade = false;

    if (TargetType == EEquipmentType::Weapon)
    {
        if (WeaponGrowthTable)
        {
            // 무기 테이블 조회
            const FWeaponGrowthRow* Row = WeaponGrowthTable->FindRow<FWeaponGrowthRow>(TargetItem->ItemID, TEXT("Upgrade"));
            // 배열 인덱스 체크
            if (Row && Row->LevelStats.IsValidIndex(NextLevel - 1))
            {
                bCanUpgrade = true;
            }
        }
    }
    else // Armor
    {
        if (ArmorGrowthTable)
        {
            // 방어구 테이블 조회 (구조체가 다름!)
            const FArmorGrowthRow* Row = ArmorGrowthTable->FindRow<FArmorGrowthRow>(TargetItem->ItemID, TEXT("Upgrade"));
            // 배열 인덱스 체크
            if (Row && Row->LevelStats.IsValidIndex(NextLevel - 1))
            {
                bCanUpgrade = true;
            }
        }
    }

    if (!bCanUpgrade)
    {
        UE_LOG(LogTemp, Warning, TEXT("최고 레벨이거나 성장 데이터를 찾을 수 없습니다."));
        return false;
    }

    // 4. 적용
    TargetItem->CurrentLevel = NextLevel;

    // 5. 비주얼 갱신
    if (TargetType == EEquipmentType::Weapon)
    {
        UpdateWeaponVisuals();
    }
    else
    {
        // UpdateArmorVisuals(); // 나중에 구현
        UE_LOG(LogTemp, Log, TEXT("방어구 강화 성공! (현재 방어력: %f)"), GetCurrentDefensePower());
    }
    
    RefreshStats();

    return true;
}

/*
// 저장할 때 호출 (UItemInstance -> FItemSaveData 변환)
FItemSaveData UPlayerEquipmentComponent::GetWeaponSaveData() const
{
    FItemSaveData Data;
    if (WeaponInstance)
    {
        Data.ItemID = WeaponInstance->ItemID;
        Data.Level = WeaponInstance->CurrentLevel;
        Data.Type = EEquipmentType::Weapon;
    }
    return Data;
}

// 로드할 때 호출 (FItemSaveData -> UItemInstance 변환)
void UPlayerEquipmentComponent::LoadWeaponFromSave(FItemSaveData LoadData)
{
    UItemInstance* NewItem = NewObject<UItemInstance>(this);
    NewItem->Init(LoadData.ItemID, LoadData.Level, LoadData.Type);
    EquipWeapon(NewItem);
}
*/